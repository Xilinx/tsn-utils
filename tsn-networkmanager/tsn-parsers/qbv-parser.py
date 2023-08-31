################################################################
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
################################################################

from __future__ import division
from array import *
import json
import libconf
import io
import os
import sys
import signal
import pty
from datetime import datetime
import time
import math
from decimal import *
import shlex, subprocess
DELTA = 0.1

def qbv_sched():
	with open(sys.argv[1]) as data_file:
		data = json.load(data_file)
	#Creating a temporary qbv config file
	f = open('qbv_cfg_bridge_tmp', 'w')
	f.write('qbv =\n{\n')
	count = 0
	error = 0
	strings = []
	first_interface = 1
	now = 0
	for int_ll in data["ietf-interfaces:interfaces"]["interface"]:
		state = array('L')
		t = array('L')
		i = 0
		gate_list_length = int_ll["ge-qbv:gate-parameters"]["admin-control-list-length"]
		cycle_time = int_ll["ge-qbv:gate-parameters"]["admin-cycle-time"]
		if cycle_time > 1000000000:
			error = 1
			print("Admin cycle time cannot be more than 1 second\n")
			break
		if int_ll["name"] == "swp0":
			strings.append("ep")
		elif int_ll["name"] == "swp1":
			strings.append(swp1_ifname)		
		else:
			strings.append(swp2_ifname)
		total_time = 0
		for conf in int_ll["ge-qbv:gate-parameters"]["admin-control-list"]:
			state.insert(i,conf["sgs-params"]["gate-states-value"]) 
			t.insert(i,conf["sgs-params"]["time-interval-value"])
			total_time = total_time + conf["sgs-params"]["time-interval-value"]
			i = i + 1
			if i == 256:
				error = 1
				print("Number of gate entries cannot be more than 256\n")
				break
			if total_time > cycle_time and cycle_time != 0:
				error = 1
				print("Sum of times of gate entries cannot be more than cycle time\n")
				break
			if conf["sgs-params"]["time-interval-value"] > 8000000:
				error = 1
				print("Time of a gate entrie cannot be more than 8ms\n")
				break
			if conf["sgs-params"]["gate-states-value"] > 7:
				error = 1
				print("gate state value cannot be greater than 7\n")
				break
		if error == 1:
			break
		string = int_ll["ge-qbv:gate-parameters"]["admin-base-time"]
		time_ns = 0
		start_sec = 0
		if(string != "0"):
			time_tuple = time.strptime(string[0:19], "%Y-%m-%dT%H:%M:%S")
			j = 0
			if string[19+j:20+j] == '.':
				j = j+1
				temp = string[19+j:20+j]
				while temp.isdigit():
					decimal = int(temp)
					time_ns = (decimal * 10**(9-j)) + time_ns
					j = j+1
					temp = string[19+j:20+j]
			json_offset = 0
			if string[19+j:20+j] != 'Z':
				json_offset_sign = 1
				if string[19+j:20+j] == '-':
					json_offset_sign = -1
				temp = string[20+j:22+j]
				json_offset = int(temp)
				temp = string[23+j:25+j]
				json_offset = json_offset_sign * ((json_offset * 60 * 60) + (int(temp) * 60))
			timestamp = time.mktime(time_tuple)
			start_sec = int(timestamp) - json_offset
		cycle_time_sec = cycle_time / (10**9)
		start_time = Decimal(start_sec) + Decimal(time_ns/(10**9))
		if (first_interface == 1):
			out_r, out_w = pty.openpty()
			process = subprocess.Popen(["ptptime",strings[-1]], stdout=out_w, preexec_fn=os.setsid)
			os.close(out_w)
			line = os.read(out_r, 1000)
			os.killpg(os.getpgid(process.pid), signal.SIGTERM)
			words = line.decode('utf8').rstrip().split(' ')
			ptptime_s = 0
			ptptime_ns = 0
			now = Decimal(ptptime_s) + Decimal(ptptime_ns/(10**9))
			first_interface = 0
		if(now>=start_time and cycle_time != 0 and start_time > 0):
			N = int ((now - start_time) / Decimal(cycle_time_sec))
			start_time = start_time + Decimal( (N+1) * cycle_time_sec)
			if ((start_time-now) < DELTA):
				N = int (DELTA / cycle_time_sec)
				start_time = start_time + Decimal( (N+1) * cycle_time_sec)
		start_sec = int(start_time)
		start_ns = int((Decimal(start_time) - Decimal(start_sec)) * (10**9))
		f.write('%s =\n{\nstart_sec = %d;\nstart_ns = %d;\ncycle_time = %d;\ngate_list_length = %d;\ngate_list =\n(\n' %(strings[count], start_sec, start_ns, cycle_time, gate_list_length))
		for k in range(0, len(state)):
			if k+1 == len(state):
				f.write('{\nstate = %d;\ntime = %d;\n}\n' % (state[k], t[k]))
			else:
				f.write('{\nstate = %d;\ntime = %d;\n},\n' % (state[k], t[k]))
		f.write(');\n};')
		count = count + 1
		if count == 3:
			break
	f.write('};')
	f.close()
	if error == 0:
		
		with io.open('qbv_cfg_bridge_tmp') as f:
			config = libconf.load(f)
		f.close()
		k = 0
		for k in range(count):
                        cmd = "qbv_sched -c %s qbv_cfg_bridge_tmp -f" %(strings[k])
                        os.system(cmd)
                        k = k + 1

proc = subprocess.Popen(["find /sys/devices/platform/ -iname  *tsn_emac_* | grep -i endpoint"],shell=True,stdout=subprocess.PIPE, stderr=subprocess.PIPE)
outs, errs = proc.communicate()
outs = outs.decode().split("\n")
swp1_ifname = ""
swp2_ifname = ""
tsn_emac_0 = ""
tsn_emac_1 = ""
for i in outs:
    if "tsn_emac_0" in i:
        tsn_emac_0 = i
    if "tsn_emac_1" in i:
        tsn_emac_1 = i
if tsn_emac_0 == "" or tsn_emac_1 == "":
    print("Cannot find temac nodes")
    exit()
swp1_ifname = os.listdir(os.path.join(tsn_emac_0 , "net"))[0]
swp2_ifname = os.listdir(os.path.join(tsn_emac_1 , "net"))[0]

qbv_sched()
while(1):
	cmd = 'inotifywait -e modify %s' %(sys.argv[1])
	while(os.system(cmd)):
		pass
	qbv_sched()

