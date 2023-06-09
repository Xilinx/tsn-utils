#!/bin/bash
################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

SCRIPTPATH="$( cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 ; pwd -P )"

ip=192.168.1.40
datadir="/tftpboot"
tftpd_conf="/etc/default/tftpd-hpa"
tftp_options="--secure --create"
ptp_conf_file="/usr/bin/ptp4l_master.conf"
parserdir=${SCRIPTPATH}
# keep the ptp_interface empty to use the default EMAC0
ptp_interface=
