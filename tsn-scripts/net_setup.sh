#!/bin/bash
#
#*****************************************************************************
#Copyright (c) 2022 Xilinx, Inc. All rights reserved.
#Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
#SPDX-License-Identifier: MIT
#*****************************************************************************
#
# Configure network for tsn
#

mac_setup="/usr/bin/network-manager/tsn-parsers/mac_setup.sh"

function print_help() {
	echo -e "Usage:"
	echo -e "  $0 <exactly one Option>"
	echo -e "Options:"
	echo -e "  -h, --help       Display this help"
	echo -e "  -b1, --master    Setup Network as Master Board"
	echo -e "  -b2, --slave     Setup Network as Slave Board"
}

if [ "$#" -ne 1 ];
then
	echo "Pass exactly 1 argument"
	print_help
	return
fi

if [ "$EUID" -ne 0 ]
	then echo "Please enter the password for sudo access"
	SUDO=sudo
else
	SUDO=
fi

# Check to ensure TSN Overlay is loaded
OVERLAY=$($SUDO xmutil listapps | grep -E 'tsn-rs485pmod|motor-ctrl-qei' | cut -d ')' -f2 | tr ',' ' ' | tr -d ' ')
if [[ "$OVERLAY" != *0* ]]; then
	echo "Please load TSN application overlay using xmutil and try again"
	return 1
fi

if [ ! -f /etc/modprobe.d/generic-uio.conf ]; then
	echo "options uio_pdrv_genirq of_id=generic-uio" | ${SUDO} tee /etc/modprobe.d/generic-uio.conf >> /dev/null
	${SUDO} rmmod uio_pdrv_genirq
	${SUDO} modprobe uio_pdrv_genirq of_id=generic-uio
fi

# Verify Switch in design
dt_dir=/proc/device-tree/
switch_dir=`find $dt_dir -iname tsn_switch*`
if [ "$switch_dir" == "" ]; then
	echo "switch not found in the design"
	switch_present=0
else
	switch_present=1
fi

VLAN_ID=${VLAN_ID:-20}

# SETUP mode is configured using the switch
# SETUP_MODE=${SETUP_MODE:-SLAVE}

MST_IP_ADDR=111.222.0.10
SLV_IP_ADDR=111.222.0.20

case $1 in
	-h | --help )
		print_help
		return 0
		;;
	-b1 | --master )
		SETUP_MODE=MASTER

		;;
	-b2 | --listener )
		SETUP_MODE=SLAVE
		;;
	* )
		echo "Invalid argument $1"
		print_help
		return 1;
		;;
esac

if [ "$SETUP_MODE" = "MASTER" ]; then
	IP_ADDR=${MST_IP_ADDR}
	QOS_MAP="egress-qos-map 0:4"
else
	IP_ADDR=${SLV_IP_ADDR}
	QOS_MAP=
fi

source $mac_setup

${SUDO} ip link delete ep.${VLAN_ID} || echo "No previous config"
${SUDO} ip link add link ep name ep.${VLAN_ID} type vlan id ${VLAN_ID} $QOS_MAP
${SUDO} ip addr add ${IP_ADDR}/24 broadcast + dev ep.${VLAN_ID}
${SUDO} ip link set dev ep.${VLAN_ID} up

# Add ep and eth2 to vlan 20 membership

if [ $switch_present != 0 ]; then
	${SUDO} switch_prog vlanm -a swp0 -v ${VLAN_ID}
	${SUDO} switch_prog vlanm -a swp1 -v ${VLAN_ID}
fi 

ip -d addr show ep.${VLAN_ID}
echo ""
echo "*** Board setup for $SETUP_MODE operations ***"
echo ""
