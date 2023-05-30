################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

ip=192.168.1.40
cwd="$( cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 ; pwd -P )"
conffile="/usr/bin/ptp4l_master.conf"
datadir="/tftpboot"
parserdir=${cwd}
tftpd_conf="/etc/default/tftpd-hpa"
tftp_options="--secure --create"
