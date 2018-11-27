package events

import (
	"encoding/json"
	"io/ioutil"
	"sort"
	"strings"
)

type Switch struct {
	Core          int    `json:"core"`
	Time          uint64 `json:"time"`
	Type          int    `json:"type"`
	Prev_pid      int    `json:"prev_pid"`
	Next_pid      int    `json:"next_pid"`
	Prev_state    uint64 `json:"prev_state"`
	Next_state    uint64 `json:"next_state"`
	In_whitch_ctx int    `json:"in_whitch_ctx"`
}

type ByTime []Switch

func (b ByTime) Len() int           { return len(b) }
func (b ByTime) Swap(i, j int)      { b[i], b[j] = b[j], b[i] }
func (b ByTime) Less(i, j int) bool { return b[i].Time < b[j].Time }

func LoadSwitch(file string) ([]Switch, error) {
	bs, err := ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	es := make([]Switch, 0)
	ls := strings.Split(string(bs), "\n")
	for _, l := range ls {
		var e Switch
		err := json.Unmarshal([]byte(l), &e)
		if err != nil {
			return nil, err
		}
		es = append(es, e)
	}
	sort.Sort(ByTime(es))

	return es, nil
}
