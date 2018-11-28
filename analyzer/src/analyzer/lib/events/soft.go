package events

import (
	"encoding/json"
	"io/ioutil"
	"strings"

	"github.com/emirpasic/gods/maps/treemap"
	"github.com/emirpasic/gods/utils"
)

var (
	sm SoftMap
)

func init() {
	sm = make(SoftMap)
}

type Soft struct {
	Core  int    `json:"core"`
	Type  int    `json:"type"`
	STime uint64 `json:"stime"`
	ETime uint64 `json:"etime"`
}

type SoftMap map[int]*treemap.Map

func (s *SoftMap) Init(sl []Soft) {
	for _, v := range sl {
		_, ok := (*s)[v.Core]
		if !ok {
			(*s)[v.Core] = treemap.NewWith(utils.UInt64Comparator)
		}
		(*s)[v.Core].Put(v.ETime, v)
	}
}

func LoadSoft(file string) ([]Soft, error) {
	bs, err := ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	es := make([]Soft, 0)
	ls := strings.Split(string(bs), "\n")
	for _, l := range ls {
		var e Soft
		err := json.Unmarshal([]byte(l), &e)
		if err != nil {
			return nil, err
		}
		es = append(es, e)
	}

	return es, nil
}

func InitSoftContainer(sl []Soft) {
	sm.Init(sl)
}

func GetCeilingSoft(core int, time uint64) *Soft {
	_, v := sm[core].Ceiling(time)
	return v.(*Soft)
}
