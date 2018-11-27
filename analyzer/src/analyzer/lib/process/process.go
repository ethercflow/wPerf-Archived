package process

import (
	"io/ioutil"
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
	NONE    = 0
	UNKNOWN = -100 // TODO: use a better way?
)

var (
	CreateTimeList map[int]uint64
	ExitTimeList   map[int]uint64

	prevStateMap PrevStateMap
	segMap       SegmentMap

	statMap StatMap
)

func init() {
	CreateTimeList = make(map[int]uint64)
	ExitTimeList = make(map[int]uint64)

	prevStateMap = make(PrevStateMap)
	segMap = make(SegmentMap)

	statMap = make(StatMap)
}

func InitSegMap(pl []int) {
	segMap.Init(pl)
}

func InitPrevStateMap(pl []int, sl []events.Switch) {
	prevStateMap.Init(pl, sl)
}

func Pids(file string) ([]int, error) {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	pidList := make([]int, 0)
	lines := strings.Split(string(data), "\n")
	for _, line := range lines {
		pid, err := strconv.Atoi(line)
		if err != nil {
			return nil, err
		}
		pidList = append(pidList, pid)
	}

	return pidList, nil
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
			CreateTimeList[v.Next_pid] = v.Time // TODO: may exist bug?
		case 3: // do_exit
			ExitTimeList[v.Prev_pid] = v.Time // TODO: may exist bug?
		}
	}

	for _, v := range pl {
		stime := traceSTime
		etime := traceETime

		t, ok := CreateTimeList[v]
		if ok {
			(*p)[v] = PrevState{t, -1}
			stime = t
		} else {
			(*p)[v] = PrevState{traceSTime, -1}
		}

		if t, ok := ExitTimeList[v]; ok {
			etime = t
		}

		statMap[v] = new(Stat)
		statMap[v].Total = etime - stime
	}
}

func GetPrevState(pid int) PrevState {
	return prevStateMap[pid]
}

func HasExitTime(pid int) (uint64, bool) {
	t, ok := ExitTimeList[pid]
	return t, ok
}

type Segment struct {
	Pid     int
	State   int
	STime   uint64
	ETime   uint64
	WaitFor int
}

type Segments struct {
	tm *treemap.Map
}

func (s *Segments) Put(pid, state int, stime, etime uint64, waitFor int) {
	s.tm.Put(stime, &Segment{pid, state, stime, etime, waitFor})
}

func (s *Segments) Floor(stime uint64) *Segment {
	_, seg := s.tm.Floor(stime)
	return seg.(*Segment)
}

type SegmentMap map[int]Segments

func (s *SegmentMap) Init(pidList []int) {
	for _, pid := range pidList {
		(*s)[pid] = Segments{treemap.NewWith(utils.UInt64Comparator)}
	}
}

func GetSegments(pid int) Segments {
	return segMap[pid]
}

type Stat struct {
	Running  uint64
	Runnable uint64
	Blocked  uint64
	HardIRQ  uint64
	SoftIRQ  uint64
	Unknown  uint64
	Total    uint64
}

type StatMap map[int]*Stat

// Matching of wait and wakeup events can naturally break a thread’s time into
// multiple segments, in either “running/runnable” or “waiting” state. The ana-
// lyzer treats running and runnable segments in the same way in this step and
// separates them later.
func BreakIntoSegments(pid int, swl []events.Switch) {
	ps := GetPrevState(pid)
	segs := GetSegments(pid)
	stat := statMap[pid]

	for _, sw := range swl {
		pt := ps.Timestamp
		switch sw.Type {
		case 0: // switch_to
			if sw.Prev_pid == pid { // switch out
				if Runnable(sw.Prev_state) {
					ps.UpdateToRunnable(sw.Time)
				} else {
					ps.UpdateToStopped(sw.Time)
				}

				sf := events.GetCeilingSoft(sw.Core, pt)

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

					stat.Running += stime - pt
					stat.SoftIRQ += etime - stime
					stat.Running += sw.Time - etime
				} else { // pid not preempt by softirq
					segs.Put(pid, RUNNING, pt, sw.Time, NONE)

					stat.Running += sw.Time - pt
				}
				break
			}

			if sw.Next_pid == pid { // switch in
				segs.Put(pid, RUNNABLE, pt, sw.Time, NONE)
				ps.UpdateToRunning(sw.Time)

				stat.Running += sw.Time - pt
			}
		case 1: // try_to_wake_up: blocked => runnable
			if sw.Next_pid == pid {
				if ps.State != BLOCKED { // we need to check this because try_to_wake_up may be the first event
					continue
				}

				ps.UpdateToRunnable(sw.Time)
				if sw.In_whitch_ctx == 0 {
					segs.Put(pid, BLOCKED, pt, sw.Time, sw.Prev_pid)

					stat.Blocked += sw.Time - pt
				} else {
					segs.Put(pid, BLOCKED, pt, sw.Time, -sw.In_whitch_ctx)

					// FIXME: swith case In_whitch_ctx to handle stat info
				}
			}
		}
	}

	traceEtime := swl[len(swl)-1].Time
	ps = GetPrevState(pid)
	switch ps.State {
	case RUNNING:
		segs.Put(pid, RUNNING, ps.Timestamp, traceEtime, NONE)

		stat.Running += traceEtime - ps.Timestamp
	case RUNNABLE:
		segs.Put(pid, RUNNABLE, ps.Timestamp, traceEtime, NONE)

		stat.Running += traceEtime - ps.Timestamp
	default:
		if t, ok := HasExitTime(pid); ok {
			seg := segs.Floor(t)
			seg.ETime = t
		} else {
			segs.Put(pid, BLOCKED, ps.Timestamp, traceEtime, UNKNOWN)

			stat.Unknown += traceEtime - ps.Timestamp
		}
	}
}
