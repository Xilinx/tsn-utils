#Copyright (c) 2020-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

#!/bin/bash
source /usr/bin/OOB_scripts/common.sh
read srcmac < /sys/class/net/ep/address
[ "$1" == "" ] && echo "specify a destination mac address" && exit
destmac=$1
get_st_pcp
init_ptp
set_qbv_tadma_sched
sleep 1
tmp=`qbv_sched -g ep | grep "Gate State: 4"`
if [ "$tmp" == "" ]; then
echo "Qbv not scheduled properly"
exit
fi
if [ $switch_present != 0 ]; then
switch_cam -d $destmac 10
switch_cam -a $destmac 10 swp1
tmp=`switch_cam -r $destmac 10 | grep "swp1"`
if [ "$tmp" == "" ]; then
echo "Switch cam not set in properly for $mac and vlan id 10"
exit
fi
fi
pids=`pidof tsn_talker`
if [ "$pids" != "" ]; then
        for pid in $pids
        do
                kill $pid
        done
fi
echo "talker configured to generate packets with length 100, st pcp and vlan id 10 at full rate continuously"
tsn_talker ep $destmac $srcmac 10 $st_pcp 100 0 -1 0 0 &

