package main

import (
	"encoding/json"
	"flag"
	"io/ioutil"
	"strconv"
	"strings"
)

// FIXME: move to Config struct
var (
	CPUFreqFile      string
	PidsFile         string
	SwitchEventFile  string
	SoftirqEventFile string
)

func init() {
	flag.StringVar(&CPUFreqFile, "cpufreq file", "", "")
	flag.StringVar(&PidsFile, "pids file", "", "")
	flag.StringVar(&SwitchEventFile, "swtich event file", "", "")
	flag.StringVar(&SoftirqEventFile, "cpufreq file", "", "")
}

type pid_t int

type Segment struct {
	Pid     pid_t
	State   int
	STime   uint64
	ETime   uint64
	WaitFor int
	Core    int
}

// key: timestamp
type Segments map[uint64]Segment

type SegmentMap map[pid_t]Segments

func (s *SegmentMap) Init() {

}

type SwitchEvent struct {
	Core          int    `json:"core"`
	Time          uint64 `json:"time"`
	Type          int    `json:"type"`
	Prev_pid      pid_t  `json:"prev_pid"`
	Next_pid      pid_t  `json:"next_pid"`
	Prev_state    uint64 `json:"prev_state"`
	Next_state    uint64 `json:"next_state"`
	In_whitch_ctx int    `json:"in_whitch_ctx"`
}

type SoftEvent struct {
	Core  int    `json:"core"`
	Type  int    `json:"type"`
	Stime uint64 `json:"stime"`
	Etime uint64 `json:"etime"`
}

// key: core id
type SoftEventMap map[int]SoftEvent

func (s *SoftEventMap) Init() {

}

type Loader interface {
	CpuFreq(file string) (float64, error)
	Pids(file string) ([]pid_t, error)
	SwitchEvents(file string) ([]SwitchEvent, error)
	SoftirqEvents(file string) ([]SoftEvent, error)
}

type loader struct {
}

func (l *loader) CPUFreq(file string) (float64, error) {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		return 0.0, err
	}

	lines := strings.Split(string(data), "\n")
	cpuFreq, err := strconv.ParseFloat(lines[0], 64)
	if err != nil {
		return 0, err
	}
	return cpuFreq, nil
}

func (l *loader) Pids(file string) ([]pid_t, error) {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	pidList := make([]pid_t, 0)
	lines := strings.Split(string(data), "\n")
	for _, line := range lines {
		pid, err := strconv.Atoi(line)
		if err != nil {
			return nil, err
		}
		pidList = append(pidList, pid_t(pid))
	}

	return pidList, nil
}

func (l *loader) SwitchEvents(file string) ([]SwitchEvent, error) {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	events := make([]SwitchEvent, 0)
	lines := strings.Split(string(data), "\n")
	for _, line := range lines {
		var event SwitchEvent
		err := json.Unmarshal([]byte(line), &event)
		if err != nil {
			return nil, err
		}
		events = append(events, event)
	}

	return events, nil
}

func (l *loader) SoftirqEvents(file string) ([]SoftEvent, error) {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	events := make([]SoftEvent, 0)
	lines := strings.Split(string(data), "\n")
	for _, line := range lines {
		var event SoftEvent
		err := json.Unmarshal([]byte(line), &event)
		if err != nil {
			return nil, err
		}
		events = append(events, event)
	}

	return events, nil
}

// Matching of wait and wakeup events can naturally break a thread’s time into
// multiple segments, in either “running/runnable” or “waiting” state. The ana-
// lyzer treats running and runnable segments in the same way in this step and
// separates them later.
func BreakIntoSegments(pid pid_t, se []SwitchEvent, sm SoftEventMap) []Segment {
	return nil
}

func Cascade(wait Segment) {

}

func main() {
	flag.Parse()
}
