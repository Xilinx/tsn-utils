#Copyright (c) 2020-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

#!/bin/bash
source /usr/bin/OOB_scripts/common.sh
[ "$1" == "" ] && echo "specify a destination mac address" && exit
init_ptp
destmac=$1
#edit qbv.cfg
sed -i 's/cycle_time = 0/cycle_time = 1000000/g' /etc/qbv.cfg
#CAM entries
if [ $switch_present != 0 ]; then
for i in 10 20
do
switch_cam -d $destmac $i
switch_cam -a $destmac $i swp1
tmp=`switch_cam -r $destmac $i | grep "swp1"`
if [ "$tmp" == "" ]; then
echo "Switch cam not set in properly for $destmac and vlan $i"
exit
fi
done
fi
qbv_sched ep off
qbv_sched ep
sleep 1
temp=`qbv_sched -g ep | grep "Gate State: 4"`
if [ "$temp" == "" ]; then
echo "Qbv not scheduled properly"
exit
fi
#getting traffic class to pcp mapping
get_st_pcp
pids=`pidof tsn_talker`
if [ "$pids" != "" ]; then
        for pid in $pids
        do
                kill $pid
        done
fi
#get MAC address
read srcmac < /sys/class/net/ep/address
be_pcp=1
echo "Two instances of talker configured, one to generate Scheduled traffic with Vlan 10"
echo "packet length 100 at full rate continuously and another to generate BE traffic with Vlan ID 20,"
echo "packet length 100 at full rate continuously."
tsn_talker ep $destmac $srcmac 10 $st_pcp 100 0 -1 0 0 &
tsn_talker ep $destmac $srcmac 20 $be_pcp 100 0 -1 0 0 &

