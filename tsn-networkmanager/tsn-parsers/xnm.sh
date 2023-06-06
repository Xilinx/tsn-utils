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
ptp_conf_file="/usr/bin/ptp4l_master.conf"
datadir="/tftpboot"
parserdir=${cwd}
ip=192.168.1.99
tftpd_conf="/etc/default/tftpd-hpa"
tftp_options="--secure --create"

if [ ! -f $config_file ]; then
        echo "$config_file not found, proceeding with default configuration"
else
        source $config_file
fi

while [ "$1" != "" ]; do
	case $1 in
        -f | --file )
		conffile=$ptp_conf_file
		;;
	-m | --master )
                conffile="/usr/bin/ptp4l_master.conf"
		;;
	-s | --slave )
                conffile="/usr/bin/ptp4l_slave.conf"
		;;
	*)
		echo "invalid argument '$1'"
                exit
		;;
	esac
	shift
done

if [ ! -d $parserdir ]; then
        echo "$parserdir not found"
        exit
fi

if [ ! -d $datadir ]; then
        $SUDO mkdir $datadir
        $SUDO chown tftp:tftp $datadir
        $SUDO sed -i '/TFTP_DIRECTORY/d' "$tftpd_conf" && $SUDO sh -c "echo 'TFTP_DIRECTORY=\"$datadir\"' >> $tftpd_conf"
        $SUDO sed -i '/TFTP_OPTIONS/d' "$tftpd_conf" && $SUDO sh -c "echo 'TFTP_OPTIONS=\"$tftp_options\"' >> $tftpd_conf"
        $SUDO systemctl restart tftpd-hpa
fi

# Check TFTP Server Status
status=$($SUDO systemctl status tftpd-hpa | grep -o "running")
echo "TFTP Server status:" $status
if [ "$status" != "running" ]; then
       echo "TFTP server is not running"
       exit
fi

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
