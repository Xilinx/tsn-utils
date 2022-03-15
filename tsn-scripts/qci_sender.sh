#Copyright (c) 2020-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

#!/bin/bash
source /usr/sbin/OOB_scripts/common.sh
if [ $switch_present == 0 ]; then
echo "Qci is not supported for ep only design"
exit
fi
send=1
destmac=e0:e0:e0:e0:e0:e0
init_ptp
set_qbv_tadma_sched
sleep 1
tmp=`qbv_sched -g ep | grep "Gate State: 4"`
if [ "$tmp" == "" ]; then
echo "Qbv not scheduled properly"
exit
fi
switch_cam -d $destmac 10
switch_cam -a $destmac 10 swp1
tmp=`switch_cam -r $destmac 10 | grep  "swp1"`
if [ "$tmp" == "" ]; then
echo "Switch cam not set in properly for $destmac and vlan id 10"
exit
fi
get_st_pcp
pids=`pidof tsn_talker`
if [ "$pids" != "" ]; then
        for pid in $pids
        do
                kill $pid
        done
fi
echo "Two instances of talker configured, one to generate traffic with Vlan 10"
echo "packet length 100 and another to generate traffic with Vlan ID 20,"
echo "packet length 60."
tsn_talker ep e0:e0:e0:e0:e0:e0 a0:a0:a0:a0:a0:a0 10 $st_pcp 100 0 50 1000000000 1 &
tsn_talker ep e0:e0:e0:e0:e0:e0 a0:a0:a0:a0:a0:a0 10 $st_pcp 60 0 50 1000000000 1 &

