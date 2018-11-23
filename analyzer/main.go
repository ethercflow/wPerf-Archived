package main

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
	Core          int
	Time          uint64 // tsc
	Type          int
	Prev_pid      pid_t
	Next_pid      pid_t
	Prev_state    uint64
	Next_state    uint64
	In_whitch_ctx int
}

type SoftEvent struct {
	Core  int
	Type  int
	stime uint64
	etime uint64
}

// key: core id
type SoftEventMap map[int]SoftEvent

func (s *SoftEventMap) Init() {

}

type Loader interface {
	CPUFreq() float64
	Pids() []pid_t
	SwitchEvents() []SwitchEvent
	SoftirqEvents() []SoftEvent
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

}
