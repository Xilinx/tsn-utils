#!/bin/bash
################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

SCRIPTPATH="$( cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 ; pwd -P )"

if [ "$EUID" -ne 0 ]
	then echo "Please enter the password for sudo access"
	SUDO=sudo
else
	SUDO=
fi

launch_parser (){
	echo $1
	if [[ $1 == *"qbv"* ]]; then
                $SUDO python3 $parserdir/qbv-parser.py $1 &
        fi
        if [[ $1 == *"fdb"* ]]; then
                $SUDO python3 $parserdir/fdb_parser.py $1 &
        fi
        if [[ $1 == *"frer"* ]]; then
                $SUDO python3 $parserdir/frer_parser.py $1 &
        fi
        if [[ $1 == *"talker_status"* ]]; then
                $SUDO python3 $parserdir/ipic-parser.py $1 &
        fi
        if [[ $1 == *"listener_status"* ]]; then
                $SUDO python3 $parserdir/ep_vlan.py $1 &
        fi
}

config_file="${SCRIPTPATH}/board_config.sh"
ptp_conf_file="/usr/bin/ptp4l_master.conf"
datadir="/tftpboot"
parserdir=${SCRIPTPATH}
ip=192.168.1.99
tftpd_conf="/etc/default/tftpd-hpa"
tftp_options="--secure --create"

if [ ! -f $config_file ]; then
        echo "$config_file not found, proceeding with default configuration"
else
        source $config_file
fi

while [ "$1" != "" ]; do
	case $1 in
        -f | --file )
		conffile=$ptp_conf_file
		;;
	-m | --master )
                conffile="/usr/bin/ptp4l_master.conf"
		;;
	-s | --slave )
                conffile="/usr/bin/ptp4l_slave.conf"
		;;
	*)
		echo "invalid argument '$1'"
                exit
		;;
	esac
	shift
done

if [ ! -d $parserdir ]; then
        echo "$parserdir not found"
        exit
fi

# Start (or restart) the TFTP server and report its status. Works both on a
# systemd host (managed unit) and in a non-systemd environment such as an OCI
# container, where in.tftpd-hpa is launched standalone in the foreground.
# systemd presence is detected via /run/systemd/system (the canonical
# sd_booted() check), not "command -v systemctl", because the systemctl binary
# can exist without a running systemd.
have_systemd() { [ -d /run/systemd/system ]; }

tftpd_running() {
        if have_systemd; then
                [ "$($SUDO systemctl is-active tftpd-hpa 2>/dev/null)" = "active" ]
        else
                pgrep -x in.tftpd-hpa >/dev/null 2>&1
        fi
}

start_tftpd() {
        if have_systemd; then
                $SUDO systemctl restart tftpd-hpa
        else
                # Non-systemd (container): run the daemon directly. Read the
                # same /etc/default/tftpd-hpa vars the systemd unit would use,
                # falling back to the parser defaults. -L keeps it in the
                # foreground (no inetd), so background it with &.
                tftpd_running && return 0
                # shellcheck disable=SC1090
                [ -f "$tftpd_conf" ] && . "$tftpd_conf"
                $SUDO in.tftpd-hpa -L ${TFTP_OPTIONS:-$tftp_options} \
                        -s "${TFTP_DIRECTORY:-$datadir}" &
                sleep 1
        fi
}

if [ ! -d $datadir ]; then
        $SUDO mkdir -p $datadir
        # in.tftpd-hpa drops privileges to its -u user before servicing a
        # write, so the data dir must be writable by that user or --create
        # uploads fail with "TFTP: Illegal operation". Distros that ship a
        # dedicated "tftp" user (and -u tftp) use it; otherwise the daemon
        # default is "nobody". Pick whichever exists so this works on both.
        # chown the OWNER only (no group): the nobody user's primary group is
        # "nogroup" (gid 65534), not "nobody", so "chown nobody:nobody" fails
        # with "invalid group", leaves the dir root:root, and the upload still
        # fails. Owning the dir (0755) is enough for the daemon to write.
        if id tftp >/dev/null 2>&1; then
                tftpd_user="tftp"
        else
                tftpd_user="nobody"
        fi
        if ! $SUDO chown "$tftpd_user" "$datadir" 2>/dev/null; then
                echo "Warning: unable to chown $datadir to $tftpd_user; falling back to 1777 permissions" >&2
                $SUDO chmod 1777 "$datadir"
        fi
fi

# Keep /etc/default/tftpd-hpa in sync with the parser's data dir / options so
# both the systemd unit and the standalone launch use the same settings.
if [ -f "$tftpd_conf" ]; then
        $SUDO sed -i '/TFTP_DIRECTORY/d' "$tftpd_conf"
        $SUDO sh -c "echo 'TFTP_DIRECTORY=\"$datadir\"' >> $tftpd_conf"
        $SUDO sed -i '/TFTP_OPTIONS/d' "$tftpd_conf"
        $SUDO sh -c "echo 'TFTP_OPTIONS=\"$tftp_options\"' >> $tftpd_conf"
fi

start_tftpd

# Check TFTP Server Status
if tftpd_running; then
        echo "TFTP Server status: running"
else
        echo "TFTP Server status: not running"
        echo "TFTP server is not running"
        exit 1
fi

# Obtain TSN MAC interface
FND_EMAC=($(find /sys/devices/platform/ -iname  *tsn_emac_* | grep -i endpoint))
EMAC0=`echo $(ls ${FND_EMAC[0]}/net/)`
echo $EMAC0
EMAC1=`echo $(ls ${FND_EMAC[1]}/net/)`
echo $EMAC1
$SUDO ifconfig ep $ip up
$SUDO ifconfig $EMAC0 up
$SUDO ifconfig $EMAC1 up
sleep 5
for i in ep $EMAC0
do
       if grep -q "down" /sys/class/net/$i/operstate > /dev/null
       then
               echo "$i is not up, failed to setup PTP"
               exit
       fi
done
$SUDO switch_prog pst -s swp0 -t 4
$SUDO switch_prog pst -s swp1 -t 4
$SUDO switch_prog pst -s swp2 -t 4
for i in swp0 swp1 swp2
do
       tmp=`$SUDO switch_prog pst -g $i | grep "Forwarding State"`
       if [ "$tmp" == "" ]; then
               echo "Switch $i not set in forwarding state"
               exit
       fi
done

if [ -f .ptplog ]; then
       $SUDO rm .ptplog
fi
pids=`pidof ptp4l`
if [ "$pids" != "" ]; then
       for pid in $pids
       do
               $SUDO kill $pid
       done
fi

$SUDO ptp4l -P -2 -H -i ${ptp_interface:-${EMAC0}} -m -f $conffile >& .ptplog &

sleep 30
tag=$( tail -n 3 .ptplog )
if [ "$tag" == "" ]; then
       echo "PTP not configured properly"
       exit
fi
echo "PTP log: "
echo $tag
for cf in $datadir/* ; do
       launch_parser $cf
done
inotifywait -m "$datadir" -e create -e moved_to --format "%w%f" |
    while read file; do
       launch_parser $file
    done
