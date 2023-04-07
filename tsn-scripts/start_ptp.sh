#!/bin/bash
#
#*****************************************************************************
#Copyright (c) 2022 Xilinx, Inc. All rights reserved.
#Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
#SPDX-License-Identifier: MIT
#*****************************************************************************
#
# Establish  Precision Time Protocol (PTP) to synchronize clocks throughout a computer network
#

# Usage and help function
function print_help() {
	echo -e "Usage:"
	echo -e "  $0 [-h|--help]"
	echo -e "Options:"
	echo -e "  -h, --help       Display this help and exit"
	echo -e "  -m, --master     Start PTP as master"
	echo -e "  -s, --slave      Start PTP as slave"
}

if [ "$#" == 0 ]; then 
	print_help
	return 
fi

while [ "$1" != "" ]; do
	case $1 in
	-h | --help )
		print_help $0
		return 0
		;;
	-m | --master )
	    MASTER=1	
		;;
	-s | --slave )
		MASTER=0
		;;
	*)
		echo "invalid argument '$1'"
		return 1
		;;
	esac
	shift
done

if [ "$EUID" -ne 0 ]
	then echo "Please enter the password for sudo access"
	SUDO=sudo
else
	SUDO=
fi

# Check to ensure TSN Overlay is loaded
if [ "$HOSTNAME" = "kria" ]; then
	OVERLAY=$($SUDO xmutil listapps | grep tsn-rs485pmod | cut -d ')' -f2 | tr ',' ' ' | tr -d ' ')
	if [ "$OVERLAY" == "-1" ]; then
		echo "Please load tsn-rs485pmod overlay using xmutil and try again"
		return 1
	fi
fi

# Setting master|slave config files
if [ "$MASTER" == "1" ]; then
	conffile="-m -f /usr/bin/ptp4l_master.conf"
	successstring="grand master"
else 
	conffile="-s -m -f /usr/bin/ptp4l_slave.conf"
	successstring=rms
fi


# Sanity test on interfaces  
EP=${EP:-ep}
FND_EMAC=($(find /sys/devices/platform/axi/. -iname  *tsn_emac_* | grep -i endpoint))
EMAC_BASE0=`echo ${FND_EMAC[0]} | cut -d '.' -f 3 | cut -d '/' -f 2`
EMAC0=`echo $(ip -d link show | grep ${EMAC_BASE0} -B1) | cut -d ':' -f2 | cut -d ' ' -f2`

for i in $EP $EMAC0
do
	if grep -q "down" /sys/class/net/$i/operstate > /dev/null                                           
	then                                                                                                
		echo "$i link is not up,failed to setup PTP"                                                                                 
		return 1
	fi                                                                                                  
done              


# Clean up 
${SUDO} killall ptp4l
logdir=~/.local/log
mkdir -p $logdir
if [ -f $logdir/ptplog ]; then                                                                         
	rm $logdir/ptplog   
fi                

# Start PTP & validate 
$SUDO ptp4l -P -2 -H -i $EMAC0  $conffile >& $logdir/ptplog &
echo "invoking ptp4l wait till complete .........."
echo "  "
tail -f ~/.local/log/ptplog &
PID=$!
sleep 30
temp=`grep -i "$successstring" $logdir/ptplog`
if [ "$temp" == "" ]; then
	echo "PTP not configured properly, log failure is at $logdir/ptplog"
	$SUDO killall ptp4l
	return 1
else 
	if [ "$MASTER" == "1" ]; then 
		$SUDO kill -9 $PID
		echo "  "
		echo "PTP started on master, PTP continues to run in background"
	else 
		$SUDO kill -9 $PID
		echo "  "
		echo "PTP sync success with master clock, PTP continues to run in background"
	fi
fi
