#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import logging

from subprocess import Popen, PIPE

def run_cmd(cmd, shell=False, input=None):
    p = Popen(cmd, shell=shell, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    return p.communicate(input=input)

def buildRecordPidList(target):
    ret = []
    
    if not target:
        logging.fatal("no target pidlist")
        exit(1)
    
    ret.extend(target.split(","))
    stdout, stderr = run_cmd(["pgrep", "ksoftirqd"])
    if stderr:
        logging.fatal("pgrep ksoftirqd failed: %s" % stderr)
        exit(1)
    ksoftirqd = stdout.split("\n")
    ret.extend(ksoftirqd[0:-1])
    stdout, stderr = run_cmd(["pgrep", "kworker"])
    if stderr:
        logging.fatal("pgrep kworker failed: %s" % stderr)
        exit(1)
    kworker = stdout.split("\n")
    ret.extend(kworker[0:-1])

    return ret

def buildFilter(pidlist):
    filter = ""
    for i in pidlist[:-1]:
        filter += "common_pid == %s || " % i
    filter += "common_pid == %s" % pidlist[-1]
    return filter

def cleanup(output):
    _, stderr = run_cmd(["rm", "-rf", "%s" % output])
    if stderr:
        logging.fatal("cleanup failed: %s" % stderr)
        exit(1)

def run(filter, disklist, niclist, output, period):
    cmd = ["./recorder", "-p", "%s" % filter ]
    if args.disklist:
        cmd.append("-d")
        cmd.append(args.disklist)
    if args.niclist:
        cmd.append("-n")
        cmd.append(args.niclist)
    cmd.append("-o")
    cmd.append(args.output)
    cmd.append("-P")
    cmd.append(args.period)

    _, stderr = run_cmd(cmd)
    if stderr:
        logging.fatal("record failed: %s" % stderr)
        exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "WPerf events recorder script")
    parser.add_argument("-p", "--pidlist", action = "store", default = None,
                        help = "The target process's worker thread list, seprated by ','")
    parser.add_argument("-P", "--period", action = "store", default = 90000,
                        help = "The recorder run period.")
    parser.add_argument("-d", "--disklist", action = "store", default = None,
                        help = "Disk name list seprated by ','.")
    parser.add_argument("-n", "--niclist", action = "store", default = None,
                        help = "Nic name list seprated by ','.")
    parser.add_argument("-o", "--output", action = "store", default = "/tmp/wperf/",
                        help = "Output dir default: /tmp/wperf/.")

    args = parser.parse_args()

    pidlist = buildRecordPidList(args.pidlist)
    filter = buildFilter(pidlist)

    cleanup(args.output)

    run(filter, args.disklist, args.niclist, args.output, args.period)
