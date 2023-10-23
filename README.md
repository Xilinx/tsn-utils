## TSN Utility Applications

tsn-utils repo contains all tsn user space applications, out of box scripts and various config files related to TSN.

* tsn-apps: Contains all the tsn related utilities switch_prog and other setup scritps and their corresponding configuration data, if any.

    * switch_prog: Used for programming port states, VLAN membership and Learning/Aging/Flush.

    * qbv_sched: Used for programming the scheduler gate time for each queue https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/25034864/Xilinx+TSN+Solution#XilinxTSNSolution-RunningQbv%2FTimeAwareShaper%3A

    * tadma_prog: Used for programming ST streams DMA fetch time https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/25034864/Xilinx+TSN+Solution#XilinxTSNSolution-TimeAwareDMA(TADMA)

    * qci_prog: Used for programming the filtering & policing parameters. 

    * cb_prog: Used for programming the redundancy feature parameters To be used in conjunction with switch_cam entries. 

    * switch_cam: Used for programming the switch CAM entries

    * switch_cam2: Used for programming the switch CAM entries supersedes switch_cam (Please refer to TSN SW UG for more details)

    * br_prog: Used for programming the preemption parameters

* tsn-conf: Contains linuxptp configuration files for various profiles for PTP server and client

* tsn-scripts: Provides default Out of Box scripts for demostrating TSN functionality

> For more information, please refer to TSN Software User Guide or wiki page: https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/25034864/Xilinx+TSN+Solution

## Build Instructions

- Install build pre-requisites

    ```bash
    sudo apt install -y build-essential bzip2 cmake libtool libconfig-dev libnl-3-dev libreadline-dev flex libnl-genl-3-dev libmnl-dev
    ```
- Clone repo

    ```bash
        git clone https://github.com/Xilinx/tsn-utils.git
        cd tsn-utils
    ```

- Build and Install

    ```bash
        sudo make install
    ```

## Run Instructions

1. Run interface setup on Board1 : `source /usr/bin/net_setup.sh -b1`
2. Run interface setup on Board2 : `source /usr/bin/net_setup.sh -b2`
3. Run PTP on Board1 in master mode : `source /usr/bin/start_ptp.sh -m`
4. Run PTP on Board2 in slave mode : `source /usr/bin/start_ptp.sh -s`
5. switch_prog : Example for forwarding
    ```
    switch_prog -s <swp number> 4
    ```
6. qci_prog : Example usage on receiver side, to filter based on size
    ```
    qci_prog -s <swp number> <mtu size> 1 0 1 1
    ```
7. cb_prog : Example to demonstrate redundant streams with vlan id 10 and 20
    ```
    (Receiver)
    switch_cam -a <mac address> 10 swp0 7
    switch_cam -a <mac address> swp0 8
    cb_prog -a swp1 0 32 noport 0 0 7 7 0 0 1 1
    cb_prog -a swp2 0 32 noport 0 0 8 7 0 0 1 1

    (Sender)
    switch_cam -a <destination mac address> 10 swp1,swp2 35
    cb_prog -a noport 65535 0 swp2 20 0 35 24 0 0 1 1
    ```
8. switch_cam : Example
    ```
    To add
    switch_cam -a <mac address> <vlan id> <swp number> <gate>
    
    To delete
    switch_cam -d <mac address> <vlan id>
    ```
9. br_prog : Example
    ```
    br_prog <interface name> enable
    br_prog <interface name> stats
    ```

## License

```
Copyright (c) 2016-2022 Xilinx Inc
Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```