################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

launch_parser (){
	echo $1
	if [[ $1 == *"qbv"* ]]; then
                python $parserdir/qbv-parser.py $1 &
        fi
        if [[ $1 == *"fdb"* ]]; then
                python $parserdir/fdb_parser.py $1 &
        fi
        if [[ $1 == *"frer"* ]]; then
                python $parserdir/frer_parser.py $1 &
        fi
        if [[ $1 == *"talker_status"* ]]; then
                python $parserdir/ipic-parser.py $1 &
        fi
        if [[ $1 == *"listener_status"* ]]; then
                python $parserdir/ep_vlan.py $1 &
        fi
}
cwd=$(pwd)
config_file="$cwd/board_config.sh"
datadir="/home/root/"
parserdir="/home/root/"
conffile="/usr/sbin/ptp4l_slave.conf"
epmac=00:0a:35:00:01:10
eth1mac=00:0a:35:00:01:1e
eth2mac=00:0a:35:00:01:1f
ip=192.168.1.99
ptp_interface=eth1
ptp_clock="/dev/ptp0"
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

#Setup tftp server
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
ifconfig ep $ip up
ifconfig $EMAC0 up
ifconfig $EMAC1 up
sleep 5
for i in ep $EMAC0 $EMAC1
do
       if grep -q "down" /sys/class/net/$i/operstate > /dev/null
       then
               echo "$i is not up"
       fi
done
switch_prog pst -s swp0 -t 4
switch_prog pst -s swp1 -t 4
switch_prog pst -s swp2 -t 4
for i in swp0 swp1 swp2
do
       tmp=`switch_prog pst -g $i | grep "Forwarding State"`
       if [ "$tmp" == "" ]; then
               echo "Switch $i not set in forwarding state"
       fi
done

if [ -f .ptplog ]; then
       rm .ptplog
fi
pids=`pidof ptp4l`
if [ "$pids" != "" ]; then
       for pid in $pids
       do
               kill $pid
       done
fi
ptp4l -P -2 -H -i $ptp_interface -m -f $conffile >& .ptplog &
sleep 60
tag=$( tail -n 3 .ptplog )
if [ "$tag" == "" ]; then
       echo "PTP not configured properly"
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

