################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

from __future__ import division
from array import *
import json
import io
import os
import sys
import re

dmacs_all = []
vids_all = []
def add_del_fdb(dmac, vids, port_str, rest_str, add):
    vid_array = re.split(',',vids)
    length = len(vid_array)
    for i in range(0,length):
        vid_range = re.split('-',vid_array[i])
        l = len(vid_range)
        if l == 1:
            start = int(vid_range[0])
            end = start + 1
        else:
            start = int(vid_range[0])
            end = int(vid_range[1]) + 1
        for vid in range(start,end):
            if add == 1:
                cmd = "switch_cam2 -e "+  port_str + " -m " +  dmac + " -v " + str(vid) + rest_str
            else:
                cmd = "switch_cam2 -d -m " + dmac + " -v " + str(vid)
            os.system(cmd)
def fdb():
    num_entries = len(dmacs_all)
    if num_entries:
        num_entries = len(dmacs_all)
        for i in range(num_entries):
            add_del_fdb(dmacs_all[i],vids_all[i],"","",0)
    del dmacs_all [:]
    del vids_all [:]
    with open(sys.argv[1]) as data_file:
        data = json.load(data_file)
    rest_str = ""
    for bridge_ll in data["ieee802-dot1q-bridge:bridges"]["bridge"]:
        for component_ll in bridge_ll["component"]:
            for cam_ll in component_ll["ge-fdb:forwarding-table"]["forwarding-entry"]:
                dmac = cam_ll["destination-mac-address"]
                dmacs_all.append(dmac)
                port_str = ""
                rest_str = ""
                for port_ll in cam_ll["port-map"]:
                    j = 0
                    port_ref = port_ll["port-ref"]
                    for port_dest in port_ll["port-dest"]:
                        if port_dest == 3:
                            port = "--swp0"
                            if port_ref == 4:
                                rest_str = " -s swpex0"
                                swpex0 = 1
                        elif port_dest == 1:
                            port = "--swp1"
                        elif port_dest == 2:
                            port = "--swp2"
                        elif port_dest == 4:
                            port = "--swpex0"
                            if port_ref == 3:
                                rest_str = " -s swp0"	
                        else:
                            sys.exit("port-dest should contain a value between 1 and 3")
                        if "vlan-dest" in cam_ll["port-map"][0].keys():
                            vlan_dest = int(cam_ll["port-map"][0]["vlan-dest"])
                            if vlan_dest == 0:
                                port += "=u"
                        if j == 0:
                            port_str += port
                        else:
                            port_str = port_str + " " + port
                        j = j + 1
                vids = cam_ll["vids"]
                vids_all.append(vids)
                if "queue-dest" in cam_ll["port-map"][0].keys():
                            queue_dest = " -i " + str(pow(2,cam_ll["port-map"][0]["queue-dest"]))
                            rest_str += queue_dest
                add_del_fdb(dmac, vids, port_str,rest_str, 0)
                add_del_fdb(dmac, vids, port_str,rest_str, 1)

fdb()
while(1):
	cmd = 'inotifywait -e modify %s' %(sys.argv[1])
	while(os.system(cmd)):
		pass
	fdb()
    
