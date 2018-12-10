# wPerf: Generic Off-CPU Analysis to Identify Bottleneck Waiting Events

wPerf is designed to identify bottlenecks caused by all kinds of waiting events.
To identify waiting events that limit the application’s throughput, wPerf uses
cascaded re-distribution to compute the local impact of a waiting event and uses
wait-for graph to compute whether such impact can reach other threads.

Check the paper for more details: [wPerf: Generic Off-CPU Analysis to Identify Bottleneck Waiting Events](https://www.usenix.org/system/files/osdi18-zhou.pdf), OSDI 2018.

## Requirements
- [Kernel](http://www.kernel.org/): Only tested on [CentOS Linux release 7.5.1804](https://www.centos.org)
  Please make sure the KProbe and CONFIG_SCHEDSTATS features are enabled.
- [libuv](https://github.com/libuv/libuv) Tested with v1.x

## Deploy

```shell
git clone https://github.com/ethercflow/wPerf
cd wPerf
./deploy.sh
```
## Tracing

Example: there's three worker threads tid1, tid2, tid3,
default the output dir is /tmp/wperf, record period is 90s.

Run

``` shell
cd bin
sudo ./recorder.py -p tid1,tid2,tid3
./encorder.py -i /tmp/wperf/softirq/output -o softirq
./encorder.py -i /tmp/wperf/switch/output -o switch
cp -a /tmp/wperf/cpufreq .
cp -a /tmp/wperf/pidlist .
./analyzer -cpufreq=cpufreq -pids=pidlist -switch=switch -softirq=softirq
```

After analyzer, you can get a waitfor file, open it to see if there is a knot.
In the future, I'll graphs the results. Currently, you can update waitfor to
[online graph explorer](https://osusyslab.github.io/wperf/) with your result
file and check out the bottleneck in that knot. (You'd better filter kworker
info before upload)

