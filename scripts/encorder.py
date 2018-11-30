#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import json

def output(fname):
    def __output(objs):
        with open(fname, "w") as f:
            for obj in objs:
                f.write(obj + "\n")
    return __output

def load(fname, handle, output):
    lines = None

    with open(fname, "r") as f:
        lines = f.readlines()

    handle(lines, output)

def encode(lines, output):
    objs = []
    for line in lines:
        structed = {}

        comm, event, data = line.rsplit(":", 2)
        task_pid, cpu, sched, timestamp = ' '.join(comm.split()).split(" ")

        structed["comm"], structed["pid"] = task_pid.rsplit("-", 1)
        structed["cpu"] = int(cpu.strip("[").strip("]"))
        structed["irqs_off"] = sched[0]
        structed["need_resched"] = sched[1]
        structed["ctx"] = sched[2]
        structed["preempt_depth"] = sched[3]
        structed["timestamp"] = timestamp

        struct = data.strip("\n").split(",")
        for field in struct:
            field = field.replace(" ", "")
            k, v = field.split("=")
            structed[k] = v
        objs.append(json.JSONEncoder().encode(structed))

        output(objs)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "WPerf events encoder script")
    parser.add_argument("-o", "--output", action = "store", default = None,
                        help = "The file name that stores encoded events.")
    parser.add_argument("-i", "--input", action = "store", default = None,
                        help = "The file name that stores original events.")
    
    args = parser.parse_args()
    
    load(args.input, encode, output(args.output))
