package main

import (
	"flag"
	"io/ioutil"
	"strconv"
	"strings"

	"analyzer/lib/events"
	"analyzer/lib/process"
)

var (
	CPUFreqFile      string
	PidsFile         string
	SwitchEventFile  string
	SoftirqEventFile string

	CPUFreq    float64
	PidList    []int
	SwitchList []events.Switch
	SoftList   []events.Soft
)

func init() {
	flag.StringVar(&CPUFreqFile, "cpufreq file", "", "")
	flag.StringVar(&PidsFile, "pids file", "", "")
	flag.StringVar(&SwitchEventFile, "swtich event file", "", "")
	flag.StringVar(&SoftirqEventFile, "cpufreq file", "", "")
}

func ReadCPUFreq(file string) (float64, error) {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		return 0.0, err
	}

	lines := strings.Split(string(data), "\n")
	cpuFreq, err := strconv.ParseFloat(lines[0], 64)
	if err != nil {
		return 0, err
	}
	return cpuFreq * 1e9, nil
}

func main() {
	flag.Parse()

	CPUFreq, _ = ReadCPUFreq(CPUFreqFile)
	PidList, _ = process.Pids(CPUFreqFile)
	SwitchList, _ = events.LoadSwitch(CPUFreqFile)
	SoftList, _ = events.LoadSoft(CPUFreqFile)

	process.InitPrevStates(PidList, SwitchList)
	events.InitSoftContainer(SoftList)

	for _, pid := range PidList {
		process.BreakIntoSegments(pid, SwitchList)
	}

	for _, pid := range PidList {
		segs := process.GetSegments(pid)
		segs.ForEach(func(_, v interface{}) {
			process.Cascade(v.(*process.Segment))
		})
	}
}
