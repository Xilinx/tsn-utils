#Copyright (c) 2020-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

#!/bin/bash
source /usr/bin/OOB_scripts/common.sh
if [ $switch_present == 0 ]; then
echo "FRER is not supported for ep only design"
exit
fi
send=1
frer=1
destmac="e0:e0:e0:e0:e0:e0"
init_ptp
set_qbv_tadma_sched
sleep 1
tmp=`qbv_sched -g ep | grep "Gate State: 4"`
if [ "$tmp" == "" ]; then
echo "Qbv not scheduled properly"
exit
fi
switch_cam -d $destmac 10
switch_cam -a $destmac 10 swp1,swp2 35
tmp=`switch_cam -r $destmac 10 | grep "Gate ID: 35"`
if [ "$tmp" == "" ]; then
echo "Switch cam not set in properly for $destmac and vlan id 10"
exit
fi
echo "ingressPort  MaxSeqId  SeqRecHistoryLen EgressPort VLANID"
echo "    noport    65535          0            swp2       20"
echo "RemainingTicks GateId MemberId SeqReset RecvTimoutValid Gatestate FrerValid"
echo "     0           35      24       0            0            1         1"
cb_prog -a noport 65535 0 swp2 20 0 35 24 0 0 1 1
get_st_pcp
pids=`pidof tsn_talker`
if [ "$pids" != "" ]; then
        for pid in $pids
        do
                kill $pid
        done
fi
echo "talker configured to generate 50 packets with length 1500, st pcp and vlan id 10"
echo "50 packets per second can be seen on the other side, some with vlan 10 and some with vlan 20, if both eth1 and eth2 are up"
tsn_talker ep $destmac a0:a0:a0:a0:a0:a0 10 $st_pcp 1500 0 50 1000000000 1 &
