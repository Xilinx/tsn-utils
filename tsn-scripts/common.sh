#
#SPDX-License-Identifier: GPL-2.0
#
dt_dir=/proc/device-tree/
switch_dir=`find $dt_dir -iname tsn_switch*`
if [ "$switch_dir" == "" ]; then
echo "switch not found in the design"
switch_present=0
else
switch_present=1
hw_addr=0
[ -f $switch_dir/xlnx,has-hwaddr-learning ] && hw_addr=1
fi

send=1
frer=0
destmac=e0:e0:e0:e0:e0:e0
st_pcp=4

get_st_pcp(){
st_str="xilinx_tsn_ep.st_pcp"
v=`grep $st_str /proc/device-tree/chosen/bootargs`
new=${v#*$st_str}
st_pcp=${new:1:1}
echo "ST PCP:$st_pcp"
}

init_ptp(){
if [ $send -eq 1 ]; then
	epmac=00:0a:35:00:01:10
	eth1mac=00:0a:35:00:01:1e
	eth2mac=00:0a:35:00:01:1f
	conffile="-s -m -f /usr/sbin/ptp4l_slave.conf"
	successstring=rms
	ip=192.168.1.99
else
	epmac=00:0a:35:00:01:20
	eth1mac=00:0a:35:00:01:2e
	eth2mac=00:0a:35:00:01:2f
	conffile="-m -f /usr/sbin/ptp4l_master.conf"
	successstring="grand master"
	ip=192.168.1.100
fi
echo "IP address of ep is being configured to $ip, Make sure the other device is on the same subnet"
ifconfig ep hw ether $epmac
ifconfig eth1 hw ether $eth1mac
ifconfig ep $ip up
ifconfig eth1 up
if [ $frer -eq 1 ]; then
	ifconfig eth2 hw ether $eth2mac
	ifconfig eth2 up
fi
sleep 5
for i in ep eth1
do
	if grep -q "down" /sys/class/net/$i/operstate > /dev/null                                           
	then                                                                                                
		echo "$i is not up"                                                                                 
		#exit                                                                                                
	fi                                                                                                  
done                                                                                                
if [ $frer -eq 1 ]; then                                                                            
	if grep -q "down" /sys/class/net/eth2/operstate > /dev/null                                         
	then                                                                                                
		echo "eth2 is not up"                                                                               
		#exit                                                                                                
	fi                                                         
fi                                                         
temp=$((switch_present & hw_addr))
if [ $temp != 0 ]; then
	switch_prog pst -s swp0 -t 4                                                                        
	switch_prog pst -s swp1 -t 4                                                                        
	switch_prog pst -s swp2 -t 4                                                                        
	for i in swp0 swp1 swp2                                    
	do                                                         
		tmp=`switch_prog pst -g $i | grep "Forwarding State"`      
		if [ "$tmp" == "" ]; then                                    
			echo "Switch $i not set in forwarding state"                                                        
			exit                                                                                                
		fi                                                                                                  
	done                                                                                                
fi                                                                                                  
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
ptp4l -P -2 -H -i eth1 -p /dev/ptp0 $conffile >& .ptplog &
sleep 60
temp=`grep -i "$successstring" .ptplog`
if [ "$temp" == "" ]; then
	echo "PTP not configured properly"
	pids=`pidof ptp4l`
	kill $pids
	exit
fi                                                                                  
}                                                                  

set_qbv_tadma_sched(){
file=tadma_sched.cfg
if [ -f  "$file" ]; then
echo "file tadma_sched.cfg already exists, remove the file and try again"
exit
fi
file=qbv_sched.cfg
if [ -f  "$file" ]; then
echo "file qbv_sched.cfg already exists, remove the file and try again"
exit
fi
cat > tadma_sched.cfg << EOF1
streams =
(
        {
                dest      = "$destmac";
                vid       = 10;
                trigger   = 100000;
                count     = 1; // fetch 1 frame at this time
        }
);
EOF1

cat > qbv_sched.cfg << EOF1
qbv =
{
    ep =
    {
        start_sec = 0;
        start_ns = 0;

        cycle_time = 1000000;
        gate_list_length = 2;
        gate_list =
        (
            {

                state = 4;
                time = 1000000;
            }
        );
    };
};
EOF1
tadma_prog -c ep tadma_sched.cfg
qbv_sched -c ep qbv_sched.cfg -f
} 
