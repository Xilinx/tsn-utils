#Copyright (c) 2020-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

#!/bin/bash
source /usr/sbin/OOB_scripts/common.sh
if [ $switch_present == 0 ]; then
echo "FRER is not supported for ep only design"
exit
fi
frer=1
send=0
mac="e0:e0:e0:e0:e0:e0"
init_ptp
switch_cam -d $mac 10
switch_cam -d $mac 20
switch_cam -a $mac 10 swp0 7
switch_cam -a $mac 20 swp0 8
for i in 10 20
do                                    
tmp=`switch_cam -r $mac $i | grep "swp0"`
if [ "$tmp" == "" ]; then                                   
echo "Switch cam not set in properly for $mac and vlan $i"
exit
fi  
done
#brctl addbr brt
#brctl addif brt eth0
#brctl addif brt ep
#ifconfig brt up
echo "FRER configured as follows:"
echo "ingressPort  MaxSeqId  SeqRecHistoryLen EgressPort VLANID"
echo "   swp1          0           32           noport     0"
echo "RemainingTicks GateId MemberId SeqReset RecvTimoutValid Gatestate FrerValid"
echo "       0          7       7       0            0            1         1"
echo "ingressPort  MaxSeqId  SeqRecHistoryLen EgressPort VLANID"
echo "   swp2          0           32           noport     0"
echo "RemainingTicks GateId MemberId SeqReset RecvTimoutValid Gatestate FrerValid"
echo "    0            8        7       0            0            1         1"
cb_prog -a swp1 0 32 noport 0 0 7 7 0 0 1 1 
cb_prog -a swp2 0 32 noport 0 0 8 7 0 0 1 1
#tcpdump -i ep -e -s0 -w frer_cap.pcapng 
tcpdump -i ep -e 
