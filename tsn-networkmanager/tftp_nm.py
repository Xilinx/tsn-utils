import os
import sys
import subprocess
from subprocess import PIPE


# TODO: def get_device_ip(macaddr):


def program_device(mac, file_path):
    ipaddr = input("Please enter the IP address of the device \n")  # temporary
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
        tftp_command = "busybox tftp -p -r " + remote_file_name + " -l " + file_path + " " + ipaddr
        try:
            os.system(tftp_command)
        except Exception as e:
            print(e)
    else:
        print("file doesnt exist" + file_path)
