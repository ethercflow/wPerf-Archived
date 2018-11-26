package main

import (
	"encoding/json"
	"flag"
	"io/ioutil"
	"sort"
	"strconv"
	"strings"
)

var (
	CPUFreqFile      string
	PidsFile         string
	SwitchEventFile  string
	SoftirqEventFile string

	CPUFreq    float64
	PidList    []pid_t
	SwitchList []SwitchEvent
	SoftList   []SoftEvent

	SegMap  SegmentMap
	PPSM    PidPrevStateMap
	SoftMap SoftEventMap
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

func (s *SegmentMap) Init(pidList []pid_t) {
	for _, pid := range pidList {
		(*s)[pid] = make(Segments)
	}
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

type ByTime []SwitchEvent

func (b ByTime) Len() int           { return len(b) }
func (b ByTime) Swap(i, j int)      { b[i], b[j] = b[j], b[i] }
func (b ByTime) Less(i, j int) bool { return b[i].Time < b[j].Time }

type SoftEvent struct {
	Core  int    `json:"core"`
	Type  int    `json:"type"`
	Stime uint64 `json:"stime"`
	Etime uint64 `json:"etime"`
}

type SoftEvents map[uint64]SoftEvent

// key: core id
type SoftEventMap map[int]SoftEvents

func (s *SoftEventMap) Init(sl []SoftEvent) {
	for _, v := range sl {
		_, ok := (*s)[v.Core]
		if !ok {
			(*s)[v.Core] = make(SoftEvents)
		}
		(*s)[v.Core][v.Etime] = v
	}
}

type PidPrevState struct {
	Timestamp uint64
	State     int
}

type PidPrevStateMap map[pid_t]PidPrevState

func (p *PidPrevStateMap) Init(pl []pid_t, sl []SwitchEvent) {
	createTimeList := make(map[pid_t]uint64)
	exitTimeList := make(map[pid_t]uint64)

	traceBeginTime := sl[0].Time
	traceEndTime := sl[len(sl)-1].Time

	for _, v := range sl {
		switch v.Type {
		case 2:
			createTimeList[v.Next_pid] = v.Time // TODO: may exist bug?
		case 3:
			exitTimeList[v.Prev_pid] = v.Time // TODO: may exist bug?
		}
	}

	for _, v := range pl {
		t, ok := createTimeList[v]
		if ok {
			(*p)[v] = PidPrevState{t, -1}
		} else {
			(*p)[v] = PidPrevState{traceBeginTime, -1}
		}

		t, ok = exitTimeList[v]
		if ok {
			(*p)[v] = PidPrevState{t, -1}
		} else {
			(*p)[v] = PidPrevState{traceEndTime, -1}
		}
	}
}

type Loader interface {
	CpuFreq(file string) (float64, error)
	Pids(file string) ([]pid_t, error)
	SwitchEvents(file string) ([]SwitchEvent, error)
	SoftirqEvents(file string) ([]SoftEvent, error)
}

type loader struct{}

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
	return cpuFreq * 1e9, nil
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

	loader := &loader{}

	CPUFreq, _ = loader.CPUFreq(CPUFreqFile)
	PidList, _ = loader.Pids(CPUFreqFile)
	SwitchList, _ = loader.SwitchEvents(CPUFreqFile)
	SoftList, _ = loader.SoftirqEvents(CPUFreqFile)

	SegMap.Init(PidList)
	sort.Sort(ByTime(SwitchList))
	PPSM.Init(PidList, SwitchList)
	SoftMap.Init(SoftList)
}
