#!/bin/bash
#
#*****************************************************************************
#Copyright (C) 2023 Advanced Micro Devices, Inc.
#SPDX-License-Identifier: MIT
#*****************************************************************************

if [ "$EUID" -ne 0 ]
	then echo "Please enter the password for sudo access"
	SUDO=sudo
else
	SUDO=
fi

# Obtain TSN MAC interface
FND_EMAC=($(find /sys/devices/platform/ -iname  *tsn_emac_* | grep -i endpoint))
EMAC0=`echo $(ls ${FND_EMAC[0]}/net/)`
EMAC1=`echo $(ls ${FND_EMAC[1]}/net/)`
EP=${EP:-ep}

# Obtain MAC Address
HW_ADDR_EMAC0=`echo $($SUDO xmutil boardid | grep PL.MAC.ID.0:) | awk -F'0: ' '{print $2}'`
if ! echo $HW_ADDR_EMAC0 | grep -q ':'
then   
    HW_ADDR_EMAC0=$(echo "$HW_ADDR_EMAC0" | sed 's/../&:/g;s/:$//')
fi
echo "eth2 MAC:" $HW_ADDR_EMAC0

HW_ADDR_EMAC1=`echo $($SUDO xmutil boardid | grep PL.MAC.ID.1:) | awk -F'1: ' '{print $2}'`
if ! echo $HW_ADDR_EMAC1 | grep -q ':'
then   
   HW_ADDR_EMAC1=$(echo "$HW_ADDR_EMAC1" | sed 's/../&:/g;s/:$//')
fi
echo "eth3 MAC:" $HW_ADDR_EMAC1

EP_MAC=$(echo $HW_ADDR_EMAC1 | tr -d ':')
MAC_ADD=$(( 0x$EP_MAC + 1 ))
HW_ADDR_EP=$(printf "%012x" $MAC_ADD | sed 's/../&:/g;s/:$//')

# Check if first 11 nibles of MAC addresses match
HW_ADDR_EMAC0_NIBBLES=$(echo $HW_ADDR_EMAC0 | tr -d ':' | cut -c1-11)
HW_ADDR_EMAC1_NIBBLES=$(echo $HW_ADDR_EMAC1 | tr -d ':' | cut -c1-11)
HW_ADDR_EP_NIBBLES=$(echo $HW_ADDR_EP | tr -d ':' | cut -c1-11)
if [ "$HW_ADDR_EMAC1_NIBBLES" != "$HW_ADDR_EP_NIBBLES" ]; then 
    echo "Warning: First 11 Nibbles of MAC Addresses do not match"
    MAC_ADD=$(( 0x$EP_MAC - 4 ))
    HW_ADDR_EP=$(printf "%012x" $MAC_ADD | sed 's/../&:/g;s/:$//')
fi
echo "ep MAC:" $HW_ADDR_EP

# Set MAC addresses
$SUDO ip link set $EP down
$SUDO ip link set $EMAC0 down
$SUDO ip link set $EMAC1 down

$SUDO ip link set $EP address $HW_ADDR_EP
$SUDO ip link set $EMAC0 address $HW_ADDR_EMAC0
$SUDO ip link set $EMAC1 address $HW_ADDR_EMAC1

$SUDO ip link set $EP up
$SUDO ip link set $EMAC0 up
$SUDO ip link set $EMAC1 up

$SUDO switch_prog pst -s swp0 -t 4
$SUDO switch_prog pst -s swp1 -t 4
$SUDO switch_prog pst -s swp2 -t 4
