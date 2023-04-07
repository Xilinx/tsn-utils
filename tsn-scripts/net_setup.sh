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
if [ "$HOSTNAME" = "kria" ]; then
	OVERLAY=$($SUDO xmutil listapps | grep tsn-rs485pmod | cut -d ')' -f2 | tr ',' ' ' | tr -d ' ')
	if [ "$OVERLAY" == "-1" ]; then
		echo "Please load tsn-rs485pmod overlay using xmutil and try again"
		return 1
	fi
fi

if [ ! -f /etc/modprobe.d/generic-uio.conf ]; then
	echo "options uio_pdrv_genirq of_id=generic-uio" | ${SUDO} tee /etc/modprobe.d/generic-uio.conf >> /dev/null
	${SUDO} rmmod uio_pdrv_genirq
	${SUDO} modprobe uio_pdrv_genirq of_id=generic-uio
fi

# Obtain TSN MAC interface 
FND_EMAC=($(find /sys/devices/platform/axi/. -iname  *tsn_emac_* | grep -i endpoint))
EMAC_BASE0=`echo ${FND_EMAC[0]} | cut -d '.' -f 3 | cut -d '/' -f 2`
EMAC_BASE1=`echo ${FND_EMAC[1]} | cut -d '.' -f 3 | cut -d '/' -f 2`
EP=${EP:-ep}
EMAC0=`echo $(ip -d link show | grep ${EMAC_BASE0} -B1) | cut -d ':' -f2 | cut -d ' ' -f2`
EMAC1=`echo $(ip -d link show | grep ${EMAC_BASE1} -B1) | cut -d ':' -f2 | cut -d ' ' -f2`

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

MST_HW_ADDR_EP="00:0a:35:00:01:10"
MST_HW_ADDR_EMAC0="00:0a:35:00:01:1e"
MST_HW_ADDR_EMAC1="00:0a:35:00:01:1f"
MST_IP_ADDR=111.222.0.10

SLV_HW_ADDR_EP="00:0a:35:00:01:20"
SLV_HW_ADDR_EMAC0="00:0a:35:00:01:2e"
SLV_HW_ADDR_EMAC1="00:0a:35:00:01:2f"
SLV_IP_ADDR=111.222.0.20

MAC_HIGH=0xF000a

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
	HW_ADDR_EP=${MST_HW_ADDR_EP}
	HW_ADDR_EMAC0=${MST_HW_ADDR_EMAC0}
	HW_ADDR_EMAC1=${MST_HW_ADDR_EMAC1}
	MAC_LOW=0x3500011e
	IP_ADDR=${MST_IP_ADDR}
	QOS_MAP="egress-qos-map 0:4"
else
	HW_ADDR_EP=$SLV_HW_ADDR_EP
	HW_ADDR_EMAC0=$SLV_HW_ADDR_EMAC0
	HW_ADDR_EMAC1=$SLV_HW_ADDR_EMAC1
	MAC_LOW=0x3500012e
	IP_ADDR=${SLV_IP_ADDR}
	QOS_MAP=
fi

${SUDO} ip link set $EP   down
${SUDO} ip link set $EMAC0 down
${SUDO} ip link set $EMAC1 down

${SUDO} ip link delete ep.${VLAN_ID} || echo "No previous config"

${SUDO} ip link set $EP   address $HW_ADDR_EP
${SUDO} ip link set $EMAC0 address $HW_ADDR_EMAC0
${SUDO} ip link set $EMAC1 address $HW_ADDR_EMAC1


if [ $switch_present != 0 ]; then
	${SUDO} switch_prog pst -s swp0 --state 4
	${SUDO} switch_prog pst -s swp1 --state 4
	${SUDO} switch_prog pst -s swp2 --state 4
fi 

# Set mac filter, when connecting TSN ethernet port in loop/mesh configuration to avoid duplicates
#${SUDO} busybox devmem 0x80078010 32 $MAC_HIGH
#${SUDO} busybox devmem 0x8007800c 32 $MAC_LOW

${SUDO} ip link set $EP   up
${SUDO} ip link set $EMAC0 up
${SUDO} ip link set $EMAC1 up

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
