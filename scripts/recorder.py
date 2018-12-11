#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import logging

from subprocess import Popen, PIPE

def run_cmd(cmd, shell=False, input=None):
    p = Popen(cmd, shell=shell, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    return p.communicate(input=input)

def getKsoftirqdList():
    stdout, stderr = run_cmd(["pgrep", "ksoftirqd"])
    if stderr:
        logging.fatal("pgrep ksoftirqd failed: %s" % stderr)
        exit(1)
    ksoftirqd = stdout.split("\n")
    return ksoftirqd[0:-1]
    
def getKworkerList():
    stdout, stderr = run_cmd(["pgrep", "kworker"])
    if stderr:
        logging.fatal("pgrep kworker failed: %s" % stderr)
        exit(1)
    kworker = stdout.split("\n")
    return kworker[0:-1]

def buildRecordTargetInfo(tlist, tnlist):
    tidlist = []
    tnameList = []
    
    if not tlist:
        tidlist.extend(tlist.split(","))
    if not tnlist:
        tnamelist.extend(tnlist.split(","))

    if not tidlist and not tnamelist:
        logging.fatal("no target tidlist or tnamelist")
        exit(1)
    
    return tidlist, tnamelist

def buildFilter(tidList, tnlist):
    filter = "prev_comm ~ ksoftirqd* || prev_comm ~ kworker*"

    for id in tidList:
        filter += " || common_pid == %s" % id
    for name in tnlist:
        filter += " || prev_comm ~ %s" % name

    return filter

def cleanup(output):
    _, stderr = run_cmd(["rm", "-rf", "%s" % output])
    if stderr:
        logging.fatal("cleanup failed: %s" % stderr)
        exit(1)

def output(fname, pids):
    with open(fname, "w") as f:
        for p in pids[:-1]:
            f.write(p + ",")
        f.write(pids[-1] + "\n")

def record_ksoftirqd(fname, ksoftirqd):
    output(fname, ksoftirqd)

def record_kworker(fname, kworker):
    output(fname, kworker)

def record_pids(fname, tidlist, tnlist, ksoftirqd, kworker):
    l = []

    l.extend(tidlist)
    l.extend(ksoftirqd)
    l.extend(kworker)

    if not tnlist:
        for tn in tnlist:
            cmd = ["pgrep", "%s" % tn]
            stdout, stderr = run_cmd(cmd)
            if stderr:
                logging.fatal("record pids failed: %s" % stderr)
                exit(1)
            tids = stdout.split("\n")
            l.extend(tids)

    output(fname, l)

def record_cpufreq(fname):
    stdout, stderr = run_cmd(["lscpu | grep \"GHz\" | awk '{print $NF}' | sed 's/GHz//'"], shell = True)
    if stderr:
        logging.fatal("record_cpufreq failed: %s" % stderr)
        exit(1)
    output(fname, [stdout.strip("\n")])

def record_events(filter, disklist, niclist, output, period):
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
    parser.add_argument("-t", "--tidlist", action = "store", default = None,
                        help = "The target process's worker thread list, seprated by ','")
    parser.add_argument("-T", "--tnamelist", action = "store", default = None,
                        help = "The target process's worker thread name list, seprated by ',', support *")
    parser.add_argument("-P", "--period", action = "store", default = "90000",
                        help = "The recorder run period.")
    parser.add_argument("-d", "--disklist", action = "store", default = None,
                        help = "Disk name list seprated by ','.")
    parser.add_argument("-n", "--niclist", action = "store", default = None,
                        help = "Nic name list seprated by ','.")
    parser.add_argument("-o", "--output", action = "store", default = "/tmp/wperf/",
                        help = "Output dir default: /tmp/wperf/.")

    args = parser.parse_args()

    ksoftirqd = getKsoftirqdList()
    kworker = getKworkerList()
    tidlist, tnamelist = buildRecordTargetInfo(args.tidlist, args.tnamelist)
    filter = buildFilter(tidlist, tnamelist)

    cleanup(args.output)

    record_ksoftirqd(args.output + "ksoftirqd", ksoftirqd)
    record_kworker(args.output + "kworker", kworker)
    record_pids(args.output + "pidlist", tidlist, tnamelist, ksoftirqd, kworker)
    record_cpufreq(args.output + "cpufreq")
    record_events(filter, args.disklist, args.niclist, args.output, args.period)
