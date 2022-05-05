#Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

tsn-utils repo contains all tsn user space applications, out of box scripts and various config files related to TSN.
tsn-apps:
Contains all the tsn related utilities and their corresponding configuration data, if any.
Please refer to OOB scripts or Software user guide to understand the correct order in which to use the following utilities after boot.

-> switch_prog: Used for programming port states, VLAN membership and Learning/Aging/Flush
Example for forwarding:
switch_prog -s <swp number> 4

-> qbv_sched: Used for programming the scheduler gate time for each queue
https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/25034864/Xilinx+TSN+Solution#XilinxTSNSolution-RunningQbv%2FTimeAwareShaper%3A

-> tadma_prog: Used for programming ST streams DMA fetch time
https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/25034864/Xilinx+TSN+Solution#XilinxTSNSolution-TimeAwareDMA(TADMA)

-> qci_prog: Used for programming the filtering & policing parameters
Example usage on receiver side, to filter based on size
qci_prog -s <swp number> <mtu size> 1 0 1 1

-> cb_prog: Used for programming the redundancy feature parameters
To be used in conjunction with switch_cam entries, example to demonstrate redundant streams with vlan id 10 and 20.
(Receiver)
switch_cam -a <mac address> 10 swp0 7
switch_cam -a <mac address> swp0 8
cb_prog -a swp1 0 32 noport 0 0 7 7 0 0 1 1
cb_prog -a swp2 0 32 noport 0 0 8 7 0 0 1 1
(Sender)
switch_cam -a <destination mac address> 10 swp1,swp2 35
cb_prog -a noport 65535 0 swp2 20 0 35 24 0 0 1 1

-> switch_cam: Used for programming the switch CAM entries
To add
switch_cam -a <mac address> <vlan id> <swp number> <gate>
To delete,
switch_cam -d <mac address> <vlan id>

-> switch_cam2: Used for programming the switch CAM entries supersedes switch_cam (Please refer to TSN SW UG for more details)

-> br_prog: Used for programming the preemption parameters
br_prog <interface name> enable
br_prog <interface name> stats

tsn-conf:
Contains linuxptp configuration files for various profiles for PTP server and client
Example:
<Xilinx board># ptp4l -P -2 -H -i eth1 -p /dev/ptp0 –s -m -f /usr/sbin/ptp4l_slave.conf
<Link partner># ptp4l -P -2 -H -i <interface name> -p /dev/<phc of link partner> -m -f ptp4l_master.conf

tsn-scripts:
Provides default Out of Box scripts for demostrating TSN functionality
How to Run:
-> Boot Linux as usual and find the scripts in / usr sbin / folder of rootfs
-> For Qbv and tadma the scripts need the destination mac address as an argument. On the link partner, to see the mac address, use the following command:
ifconfig <interface>
run the script for the clause you want to run
#sh qbv_auto.sh <dest mac>
-> For FRER and Qci, receiver side script should be run first.
Receiver side
#sh <clause>_receive.sh
Sender side
#sh <clause>_sender.sh

For more information, please refer to TSN Software User Guide or
wiki page: https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/25034864/Xilinx+TSN+Solution
