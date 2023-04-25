#!/bin/bash
################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

cwd="$( cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 ; pwd -P )"

launch_parser (){
	echo $1
	if [[ $1 == *"qbv"* ]]; then
                $SUDO python3 $parserdir/qbv-parser.py $1 &
        fi
        if [[ $1 == *"fdb"* ]]; then
                $SUDO python3 $parserdir/fdb_parser.py $1 &
        fi
        if [[ $1 == *"frer"* ]]; then
                $SUDO python3 $parserdir/frer_parser.py $1 &
        fi
        if [[ $1 == *"talker_status"* ]]; then
                $SUDO python3 $parserdir/ipic-parser.py $1 &
        fi
        if [[ $1 == *"listener_status"* ]]; then
                $SUDO python3 $parserdir/ep_vlan.py $1 &
        fi
}

if [ "$EUID" -ne 0 ]
	then echo "Please enter the password for sudo access"
	SUDO=sudo
else
	SUDO=
fi

config_file="${cwd}/board_config.sh"
conffile="/usr/bin/ptp4l_master.conf"
datadir="/home/ubuntu/datadir"
parserdir=${cwd}
ip=192.168.1.99

if [ ! -f $config_file ]; then
        echo "$config_file not found, proceeding with default configuration"
else
        source $config_file
fi

if [ ! -d $parserdir ]; then
        echo "$parserdir not found"
        exit
fi

if [ ! -d $datadir ]; then
        mkdir $datadir
fi

# Setup tftp server
inetd_conf="/etc/inetd.conf"
tftp_server_conf="69 dgram udp nowait root tftpd tftpd -c -l $datadir"

pids=`pidof inetd`
if [ "$pids" != "" ]; then
        for pid in $pids
        do
                kill $pid
        done
fi
echo $tftp_server_conf >> $inetd_conf
inetd

# Obtain TSN MAC interface
FND_EMAC=($(find /sys/devices/platform/ -iname  *tsn_emac_* | grep -i endpoint))
EMAC0=`echo $(ls ${FND_EMAC[0]}/net/)`
echo $EMAC0
EMAC1=`echo $(ls ${FND_EMAC[1]}/net/)`
echo $EMAC1
$SUDO ifconfig ep $ip up
$SUDO ifconfig $EMAC0 up
$SUDO ifconfig $EMAC1 up
sleep 5
for i in ep $EMAC0
do
       if grep -q "down" /sys/class/net/$i/operstate > /dev/null
       then
               echo "$i is not up, failed to setup PTP"
               exit
       fi
done
$SUDO switch_prog pst -s swp0 -t 4
$SUDO switch_prog pst -s swp1 -t 4
$SUDO switch_prog pst -s swp2 -t 4
for i in swp0 swp1 swp2
do
       tmp=`$SUDO switch_prog pst -g $i | grep "Forwarding State"`
       if [ "$tmp" == "" ]; then
               echo "Switch $i not set in forwarding state"
               exit
       fi
done

if [ -f .ptplog ]; then
       $SUDO rm .ptplog
fi
pids=`pidof ptp4l`
if [ "$pids" != "" ]; then
       for pid in $pids
       do
               $SUDO kill $pid
       done
fi
$SUDO ptp4l -P -2 -H -i $EMAC0 -m -f $conffile >& .ptplog &
sleep 30
tag=$( tail -n 3 .ptplog )
if [ "$tag" == "" ]; then
       echo "PTP not configured properly"
       exit
fi
echo "PTP log: "
echo $tag
for cf in $datadir/* ; do
       launch_parser $cf
done
inotifywait -m "$datadir" -e create -e moved_to --format "%w%f" |
    while read file; do
       launch_parser $file
    done
