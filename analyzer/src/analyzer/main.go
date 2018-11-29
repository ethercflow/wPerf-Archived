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
	flag.StringVar(&cpuFreqFile, "cpufreq file", "cpufreq", "cpufreq")
	flag.StringVar(&pidsFile, "pids file", "pids", "")
	flag.StringVar(&switchEventFile, "swtich event file", "switch", "switch")
	flag.StringVar(&softirqEventFile, "soft irq event file", "soft", "soft")
	flag.StringVar(&statFile, "stat file", "stat", "")
	flag.StringVar(&waitForGraphFile, "wait-for graph file", "waitfor", "waitfor")
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

	process.OutputStat(statFile)
	process.OutputWaitForGraph(waitForGraphFile)
}
