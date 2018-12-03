#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

from subprocess import Popen, PIPE

def run_cmd(cmd, shell=False, input=None):
    p = Popen(cmd, shell=shell, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    return p.communicate(input=input)

def buildRecordPidList(pidlist):
    ps = pidlist.split(",")
    stdout, stderr = run_cmd("pgrep", "ksoftirq")
    print(stdout)
    stdout, stderr = run_cmd("pgrep", "kworker")
    print(stdout)
        

if __name == "__main__":
    parse = argparse.ArgumentParser(description = "WPerf events recorder script")
    parser.add_argument("-p", "--pidlist", action = "store", default = None,
                        help = "The target process's worker thread list, seprated by ','")
    parser.add_argument("-P", "--period", action = "store", default = 90000,
                        help = "The recorder run period.")
    parser.add_argument("-d", "--disklist", action = "store", default = None,
                        help = "Disk name list seprated by ','.")
    parser.add_argument("-n", "--niclist", action = "store", default = None,
                        help = "Nic name list seprated by ','.")
    
    args = parser.parse_args()

    buildRecordPidList(args.pidlist)
