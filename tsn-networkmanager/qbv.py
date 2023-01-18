# -*- coding: utf-8 -*-
"""
Created on Tue Dec 13 18:12:24 2022

@author: pranavis
"""
import json
import string
import tftp_nm


def modify_schedule(mac,qbv_file_path,self=None):
    qbv_json = open(qbv_file_path, 'r+')
    data = json.load(qbv_json)
    port_num = 0
    val = input("Enter the switch port whose qbv schedule needs to be modified:\n")
    if val not in ["swp0","swp1", "swp2"]:
        print("Invalid Switch port name\n")
        qbv_json.close()
        return
    if val == "swp0":
        port_num = 0
    if val == "swp1":
        port_num = 1
    if val == "swp2":
        port_num = 2
    cycle_time = int(input("Enter cycle time in nanoseconds, Maximum cycle time is 1 sec\n"))
    if cycle_time > 1000000000:
        print("Invalid cycle time: " + cycle_time)
        qbv_json.close()
        return
    data["ietf-interfaces:interfaces"]["interface"][port_num]["ge-qbv:gate-parameters"]["admin-cycle-time"] = cycle_time
    base_time = input("Enter cycle start time (base time)\n")
    data["ietf-interfaces:interfaces"]["interface"][port_num]["ge-qbv:gate-parameters"]["admin-base-time"] = base_time
    list_length = int(input("Enter gate list length:\n"))
    if list_length > 256 or list_length < 1:
        print("Invalid list length: " + list_length)
        qbv_json.close()
        return
    data["ietf-interfaces:interfaces"]["interface"][port_num]["ge-qbv:gate-parameters"][
        "admin-control-list-length"] = list_length
    data["ietf-interfaces:interfaces"]["interface"][port_num]["ge-qbv:gate-parameters"]["admin-control-list"].clear()
    total_time = 0
    for index in range(1, list_length + 1):
        list_entry = dict.fromkeys(["index", "operation-name", "sgs-params"])
        list_entry["sgs-params"] = dict.fromkeys(["gate-states-value", "time-interval-value"])
        print("Entry " + str(index))
        list_entry["index"] = index
        list_entry["operation-name"] = "set-gate-states"
        print(
            "To open BE use state : 1\nTo open ST use state : 4\nTo open RES use state : 2\nOpen both ST & BE state: 5("
            "1+4) ")
        gate_states_value = int(input("Enter gate state value\n"))
        if gate_states_value not in [1, 2, 4, 5]:
            print("Invalid gate state value, aborting configuration")
            qbv_json.close()
            return
        list_entry["sgs-params"]["gate-states-value"] = gate_states_value
        window_size = int(input("Enter time for the gate list entry\n"))
        list_entry["sgs-params"]["time-interval-value"] = window_size
        total_time = total_time + window_size
        data["ietf-interfaces:interfaces"]["interface"][port_num]["ge-qbv:gate-parameters"]["admin-control-list"].append(
            list_entry)
    if total_time > cycle_time:
        val = input(
            "Sum of all gate list entry times is greater than cycle time, do you still want to proceed with this "
            "schedule?[y/n]").lower()
        if val in ["n", "no"]:
            qbv_json.close()
            return
    qbv_json.close()
    qbv_json = open(qbv_file_path, "w+")
    json.dump(data, qbv_json, indent=11)
    qbv_json.close()
    tftp_nm.program_device(mac,qbv_file_path)


