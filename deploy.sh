#!/usr/bin/env bash

if [ -d "bin" ]; then
  rm -rf bin
fi

mkdir bin

make -C module
if [ "`lsmod | grep trace_wperf_events.ko`" != "\n" ]; then
  sudo rmmod trace_wperf_events.ko
fi
cd module && sudo insmod trace_wperf_events.ko

cd ..

make -C recorder
mv recorder/recorder bin/

backup=`echo $GOPATH`
export GOPATH=`pwd`/analyzer
cd ./analyzer/src/analyzer && go build
mv analyzer ../../../bin
cd ../../../
export GOPATH=$backup

cp scripts/*.py bin/
