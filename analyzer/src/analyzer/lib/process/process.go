package process

import (
	"bufio"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strconv"
	"strings"

	"analyzer/lib/events"
	"github.com/emirpasic/gods/maps/treemap"
	"github.com/emirpasic/gods/utils"
)

const (
	RUNNING = iota
	RUNNABLE
	BLOCKED
)

const (
	HI_SOFTIRQ = iota
	TIMER_SOFTIRQ
	NET_TX_SOFTIRQ
	NET_RX_SOFTIRQ
	BLOCK_SOFTIRQ
	IRQ_POLL_SOFTIRQ
	TASKLET_SOFTIRQ
	SCHED_SOFTIRQ
	HRTIMER_SOFTIRQ
	RCU_SOFTIRQ

	NR_SOFTIRQS

	HARDIRQ
	KSOFTIRQ
	KERNEL
	UNKNOWN
)

const (
	NONE = 0
)

var (
	cpuFreq float64

	createTimeList map[int]uint64
	exitTimeList   map[int]uint64

	prevStateMap PrevStateMap
	segMap       SegmentMap

	waitForGraph WaitForGraph
)

func init() {
	createTimeList = make(map[int]uint64)
	exitTimeList = make(map[int]uint64)

	prevStateMap = make(PrevStateMap)
	segMap = make(SegmentMap)

	waitForGraph = make(WaitForGraph)
}

func initSegMap(pl []int) {
	segMap.Init(pl)
}

func InitPrevStates(pl []int, sl []events.Switch) {
	prevStateMap.Init(pl, sl)
}

func ReadCPUFreq(file string) {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		log.Fatalln("ReadCPUFreq: ", err)
	}

	lines := strings.Split(string(data), "\n")
	cpuFreq, err = strconv.ParseFloat(lines[0], 64)
	if err != nil {
		log.Fatalln("ReadCPUFreq: ", err)
	}
}

func Pids(file string) []int {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		log.Fatalln("Pids: ", err)
	}

	pidList := make([]int, 0)
	lines := strings.Split(string(data), ",")
	for _, line := range lines {
		pid, err := strconv.Atoi(strings.TrimSuffix(line, "\n"))
		if err != nil {
			log.Fatalln("Pids: ", err)
		}
		pidList = append(pidList, pid)
	}

	initSegMap(pidList) // Is a good place to do init?

	return pidList
}

type PrevState struct {
	Timestamp uint64
	State     int
}

func (p *PrevState) UpdateToRunning(time uint64) {
	p.State = 0
	p.Timestamp = time
}

func (p *PrevState) UpdateToRunnable(time uint64) {
	p.State = 1
	p.Timestamp = time
}

func (p *PrevState) UpdateToStopped(time uint64) {
	p.State = 2
	p.Timestamp = time
}

func Runnable(state uint64) bool {
	// -1 unrunnable, 0 runnable, >0 stopped
	return state == 0
}

// key: pid
type PrevStateMap map[int]PrevState

func (p *PrevStateMap) Init(pl []int, sl []events.Switch) {

	traceSTime := sl[0].Time
	traceETime := sl[len(sl)-1].Time

	for _, v := range sl {
		switch v.Type {
		case 2: // wake_up_new_task
			createTimeList[v.NextPid] = v.Time // TODO: may exist bug?
		case 3: // do_exit
			exitTimeList[v.PrevPid] = v.Time // TODO: may exist bug?
		}
	}

	for _, v := range pl {
		t, ok := createTimeList[v]
		if ok {
			(*p)[v] = PrevState{t, -1}
		} else {
			(*p)[v] = PrevState{traceSTime, -1}
		}

		t, ok = exitTimeList[v]
		if ok {
			(*p)[v] = PrevState{t, -1}
		} else {
			(*p)[v] = PrevState{traceETime, -1}
		}
	}
}

func GetPrevState(pid int) PrevState {
	return prevStateMap[pid]
}

func HasExitTime(pid int) (uint64, bool) {
	t, ok := exitTimeList[pid]
	return t, ok
}

type Segment struct {
	Pid     int
	State   int
	STime   uint64
	ETime   uint64
	WaitFor int
	Overlap []int
}

type Segments struct {
	tm *treemap.Map
}

func (s *Segments) Put(pid, state int, stime, etime uint64, waitFor int) {
	s.tm.Put(stime, &Segment{pid, state, stime, etime, waitFor, make([]int, 0)})
}

func (s *Segments) Floor(stime uint64) (uint64, *Segment) {
	k, v := s.tm.Floor(stime)
	return k.(uint64), v.(*Segment)
}

func (s *Segments) Ceiling(stime uint64) (uint64, *Segment) {
	k, v := s.tm.Ceiling(stime)
	return k.(uint64), v.(*Segment)
}

func (s *Segments) ForEach(f func(k, v interface{})) {
	it := s.tm.Iterator()
	for it.Next() {
		k := it.Key()
		v := it.Value()
		f(k, v)
	}
}

type SegmentMap map[int]*Segments

func (s *SegmentMap) Init(pidList []int) {
	for _, pid := range pidList {
		(*s)[pid] = &Segments{treemap.NewWith(utils.UInt64Comparator)}
	}
}

func GetSegments(pid int) *Segments {
	return segMap[pid]
}

type WaitForGraph map[string]uint64

// Matching of wait and wakeup events can naturally break a thread’s time into
// multiple segments, in either “running/runnable” or “waiting” state. The ana-
// lyzer treats running and runnable segments in the same way in this step and
// separates them later.
func BreakIntoSegments(pid int, swl []events.Switch) {
	ps := GetPrevState(pid)
	segs := GetSegments(pid)

	for _, sw := range swl {
		pt := ps.Timestamp
		switch sw.Type {
		case 0: // switch_to
			if sw.PrevPid == pid { // switch out
				if Runnable(sw.PrevState) {
					ps.UpdateToRunnable(sw.Time)
				} else {
					ps.UpdateToStopped(sw.Time)
				}

				sf := events.GetCeilingSoft(sw.CPU, pt)

				if sf == nil { // no softirq happen
					segs.Put(pid, RUNNING, pt, sw.Time, NONE)
					break
				}

				stime := sf.STime
				etime := sf.ETime
				if stime < sw.Time && etime < sw.Time { // pid preempt by softirq
					segs.Put(pid, RUNNING, pt, stime, NONE)
					segs.Put(pid, RUNNABLE, stime, etime, NONE)
					segs.Put(pid, RUNNING, etime, sw.Time, NONE)
				} else { // pid not preempt by softirq
					segs.Put(pid, RUNNING, pt, sw.Time, NONE)
				}
				break
			}

			if sw.NextPid == pid { // switch in
				segs.Put(pid, RUNNABLE, pt, sw.Time, NONE)
				ps.UpdateToRunning(sw.Time)
			}
		case 1: // try_to_wake_up: blocked => runnable
			if sw.NextPid == pid {
				if ps.State != BLOCKED { // we need to check this because try_to_wake_up may be the first event
					continue
				}

				ps.UpdateToRunnable(sw.Time)
				if sw.InWhichCtx == 0 {
					segs.Put(pid, BLOCKED, pt, sw.Time, sw.PrevPid)
				} else {
					segs.Put(pid, BLOCKED, pt, sw.Time, -sw.InWhichCtx)
				}
			}
		}
	}

	traceEtime := swl[len(swl)-1].Time
	ps = GetPrevState(pid)
	switch ps.State {
	case RUNNING:
		segs.Put(pid, RUNNING, ps.Timestamp, traceEtime, NONE)
	case RUNNABLE:
		segs.Put(pid, RUNNABLE, ps.Timestamp, traceEtime, NONE)
	default:
		if t, ok := HasExitTime(pid); ok {
			_, seg := segs.Floor(t)
			seg.ETime = t
		} else {
			segs.Put(pid, BLOCKED, ps.Timestamp, traceEtime, -UNKNOWN)
		}
	}
}

// TODO: maybe slow, change tail recursion to iteration
func Cascade(w *Segment) {
	if w.State != BLOCKED {
		return
	}

	w.Overlap = append(w.Overlap, w.Pid)

	addWeight(w.Pid, w.WaitFor, w.ETime, w.STime)
	segs := findAllWaitingSegments(w.WaitFor, w.ETime, w.STime)
	for _, seg := range segs {
		if seg.STime < w.STime {
			seg.STime = w.STime
		}
		if seg.ETime > w.ETime {
			seg.ETime = w.ETime
		}
		Cascade(seg)
	}
}

func addWeight(waitID, wakerID int, etime, stime uint64) {
	k := strconv.Itoa(waitID) + " " + strconv.Itoa(wakerID) + " "
	if _, ok := waitForGraph[k]; !ok {
		waitForGraph[k] = 0
	}
	waitForGraph[k] += etime - stime
}

// Find all waiting segments in wakerID that overlap with [w.start w.end]
func findAllWaitingSegments(wakerID int, etime, stime uint64) []*Segment {
	segs := GetSegments(wakerID)

	if segs == nil {
		return nil
	}

	_, seg := segs.Floor(stime)
	if seg == nil {
		return nil
	}

	waitSegs := make([]*Segment, 0)
	for {
		if seg.State == BLOCKED {
			// cascade will align waitSeg's stime with wID, I'm not sure the
			// affect to the waitSeg's cascade, so create a copy for align
			waitSeg := *seg
			copy(waitSeg.Overlap, seg.Overlap)
			contained := false
			for _, pid := range waitSeg.Overlap {
				if pid == wakerID {
					contained = true
					break
				}
			}
			if !contained {
				waitSeg.Overlap = append(waitSeg.Overlap, wakerID)
			}
			waitSegs = append(waitSegs, &waitSeg)
		}

		if seg.ETime >= etime {
			break
		}

		_, seg = segs.Ceiling(stime + 1) // TODO: make sure +1 is needed
		if seg == nil || seg.STime >= etime {
			break
		}
	}

	return nil
}

func OutputWaitForGraph(file string) {
	f, err := os.Create(file)
	if err != nil {
		log.Fatalln("Create %s failed: ", file, err)
	}
	defer f.Close()

	w := bufio.NewWriter(f)
	for k, v := range waitForGraph {
		if strings.Contains(k, strconv.Itoa(-UNKNOWN)) {
			continue
		}
		l := fmt.Sprintf("%s%f", k, float64(v)/cpuFreq)
		fmt.Fprintln(w, l)
	}
	w.Flush()
}
