#!/usr/bin/env bash

mkdir bin

make -C module
cd module && sudo insmod trace_wperf_events.ko

cd ..

make -C recorder
mv recorder/recorder bin/

cp scripts/*.py bin/
