# -*- coding: utf-8 -*-
"""
Created on Tue Dec 13 18:12:24 2022

@author: pranavis
"""
import json
import sys
import string
import yaml
import re
import tftp_nm


def add_fdb_main(mac,fdb_file_path,self=None):
    num_entries = 0
    while (True):
        fdb_json = open(fdb_file_path, 'r+')
        data = json.load(fdb_json)
        # fdb_json.close()
        ret = add_fdb(data)
        if ret == -1:
            if num_entries > 0:
                print(str(num_entries) + " valid entries will be sent to the device")
                tftp_nm.program_device(mac,fdb_file_path)
                fdb_json.close()
            return
        else:
            num_entries = num_entries + 1
            fdb_json.close()
            fdb_json = open(fdb_file_path, "w+")
            json.dump(ret, fdb_json, indent=11)
            fdb_json.close()
            val = input(
                "Static CAM entry added locally, Want to add more entries [Y/N]? If you enter N/n, device will be "
                "programmed immediately\n")
            if val.lower() in ["y", "yes"]:
                continue
            else:
                tftp_nm.program_device(mac,fdb_file_path)
                return


def add_fdb(fdb_dict):
    mac = input("Please enter a destination MAC address for the CAM entry\n")
    if not re.match("[0-9a-f]{2}([-:]?)[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$", mac.lower()):
        print("Invalid MAC address\n")
        return -1
    vlan = input("Please enter a vlan ID(s)/range of vlan's for the CAM Entry\n")
    dest_ports = input(
        "Please specify destination switch port/s:\n swp0: 3, swp1: 1, swp2: 2\n Multiple ports can be "
        "specified using a comma separated string\n")
    port_list = dest_ports.split(",", 2)
    port_list_int = []
    for port in port_list:
        if int(port) not in [1, 2, 3]:
            print("Invalid name for destination switch port: " + port)
            return -1
        else:
            port_list_int.append(int(port))
    forwarding_entry = dict.fromkeys(["destination-mac-address", "vids", "port-map"])
    forwarding_entry["destination-mac-address"] = mac
    forwarding_entry["vids"] = vlan
    port_map_list = []
    port_map_dict = dict.fromkeys(["port-dest", "port-ref"])
    port_map_dict["port-ref"] = 3
    port_map_dict["port-dest"] = port_list_int
    port_map_list.append(port_map_dict)
    forwarding_entry["port-map"] = port_map_list
    fdb_dict["ieee802-dot1q-bridge:bridges"]["bridge"][0]["component"][0]["ge-fdb:forwarding-table"][
        "forwarding-entry"].append(forwarding_entry)
    return fdb_dict
