#
#SPDX-License-Identifier: GPL-2.0
#
#!/bin/bash
#interface bringup
source /usr/sbin/OOB_scripts/common.sh
ifconfig ep 192.168.1.99 up
ifconfig eth1 up
sleep 5
temp=`grep "up" /sys/class/net/eth1/operstate`
if [ "$temp" == "" ]; then
echo "eth1 not up"
exit
fi
temp=$((switch_present & hw_addr))
if [ $temp != 0 ]; then
switch_prog pst -s swp0 -t 4
switch_prog pst -s swp1 -t 4
for i in swp0 swp1
do
temp=`switch_prog pst -g $i | grep "Forwarding State"`
if [ "$temp" == "" ]; then
echo "Switch $i not set in forwarding state"
exit
fi
done
fi
#PTP
ptp4l -P -2 -H -i eth1 -p /dev/ptp0 -s -m -f /usr/sbin/ptp4l_slave.conf
