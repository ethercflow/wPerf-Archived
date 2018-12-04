package events

import (
	"encoding/json"
	"io/ioutil"
	"log"
	"sort"
	"strings"
)

type Switch struct {
	CPU        int    `json:"cpu"`
	Time       uint64 `json:"tsc,string"`
	Type       int    `json:"type,string"`
	PrevPid    int    `json:"prev_pid,string"`
	NextPid    int    `json:"next_pid,string"`
	PrevState  uint64 `json:"prev_state,string"`
	NextState  uint64 `json:"next_state,string"`
	InWhichCtx int    `json:"in_which_ctx,string"`
}

type byTime []Switch

func (b byTime) Len() int           { return len(b) }
func (b byTime) Swap(i, j int)      { b[i], b[j] = b[j], b[i] }
func (b byTime) Less(i, j int) bool { return b[i].Time < b[j].Time }

func LoadSwitch(file string) []Switch {
	bs, err := ioutil.ReadFile(file)
	if err != nil {
		log.Fatalln("LoadSwitch: ", err)
	}

	es := make([]Switch, 0)
	ls := strings.Split(string(bs), "\n")
	ls = ls[:len(ls)-1]
	for i, l := range ls {
		var e Switch
		err := json.Unmarshal([]byte(strings.TrimSuffix(l, "\n")), &e)
		if err != nil {
			log.Println("LoadSwitch: ", err, "linum: ", i, "line: ", l)
			continue
		}
		es = append(es, e)
	}
	sort.Sort(byTime(es))

	return es
}
