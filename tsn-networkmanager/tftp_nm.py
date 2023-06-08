################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

import os
import sys
import subprocess
from subprocess import PIPE


def get_device_ip(macaddr):
    ip_addr = ""
    arp_command = "arp -n | grep " + macaddr
    try:
        proc = subprocess.Popen(arp_command, stdout=subprocess.PIPE, shell=True)
        arp_output = str(proc.communicate()[0],'UTF-8')
        print(arp_output)
        arp_output = arp_output.split(" ")
        ip_addr = arp_output[0]
    except Exception as e:
        print(e)
    return ip_addr

def program_device(mac, file_path):
    #ipaddr = input("Please enter the IP address of the device \n")  # temporary
    ipaddr = get_device_ip(mac)
    if(not ipaddr):
        print("Error obtaining IP address of " + mac + "\n")
        exit()
    proc = subprocess.run(["busybox"], stdout=PIPE, stderr=PIPE)
    errorstring = "Command not found"
    if errorstring in str(proc.stdout):
        print("Busybox not found, please install busybox with tftp\n")
        exit()
    if "tftp" not in str(proc.stdout):
        print("Please enable Busybox tftp\n")
        exit()
    if os.path.exists(file_path):
        remote_file_name = os.path.basename(file_path)
        print("Sending " + file_path + " \n")
        tftp_command = "curl -T " + file_path + " tftp://" + ip_addr
        print(tftp_command)
        try:
            os.system(tftp_command)
        except Exception as e:
            print(e)
    else:
        print("file doesnt exist" + file_path)
