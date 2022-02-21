#
#SPDX-License-Identifier: GPL-2.0
#
#!/bin/bash
source /usr/sbin/OOB_scripts/common.sh
if [ $switch_present == 0 ]; then
echo "Qci is not supported for ep only design"
exit
fi
send=0
destmac=e0:e0:e0:e0:e0:e0
echo "Make sure there's incoming traffic on eth1 with packets with length more than and less than 80"
switch_cam -d $destmac 10
switch_cam -a $destmac 10 swp0 1
tmp=`switch_cam -r $destmac 10 | grep "swp0"`
if [ "$tmp" == "" ]; then
echo "Switch cam not set in properly for $destmac and vlan 10"
exit
fi
#brctl addbr brt
#brctl addif brt eth0
#brctl addif brt ep
#ifconfig brt up
init_ptp #initialise
#programming qci
qci_prog -s swp1 80 1 0 1 1 
echo "packets with length>80 will be dropped by the switch"
echo "observe the incoming traffic on ep. All packets will be of size less than 80 bytes"
tcpdump -i ep -e
