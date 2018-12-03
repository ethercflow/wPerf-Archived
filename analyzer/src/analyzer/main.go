package main

import (
    "flag"

    "analyzer/lib/events"
    "analyzer/lib/process"
)

var (
    cpuFreqFile      string
    pidsFile         string
    switchEventFile  string
    softirqEventFile string

    statFile         string
    waitForGraphFile string

    pidList    []int
    switchList []events.Switch
    softList   []events.Soft
)

func init() {
    flag.StringVar(&cpuFreqFile, "cpufreq", "cpufreq", "cpufreq file")
    flag.StringVar(&pidsFile, "pids", "pids", "pids file")
    flag.StringVar(&switchEventFile, "switch", "switch", "switch event file")
    flag.StringVar(&softirqEventFile, "softirq", "soft", "softirq event file")
    flag.StringVar(&waitForGraphFile, "waitfor", "waitfor", "wait-for graph file")
}

func main() {
    flag.Parse()

    process.ReadCPUFreq(cpuFreqFile)
    pidList = process.Pids(pidsFile)
    switchList = events.LoadSwitch(switchEventFile)
    softList = events.LoadSoft(softirqEventFile)

    process.InitPrevStates(pidList, switchList)
    events.InitSoftContainer(softList)

    for _, pid := range pidList {
        process.BreakIntoSegments(pid, switchList)
    }

    for _, pid := range pidList {
        segs := process.GetSegments(pid)
        segs.ForEach(func(_, v interface{}) {
            process.Cascade(v.(*process.Segment))
        })
    }

    process.OutputWaitForGraph(waitForGraphFile)
}
