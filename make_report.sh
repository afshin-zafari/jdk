#!/bin/bash
#make test CONF=debug LOG=info JOBS=20 TEST="micro:vm.runtime.NMTBenchmark"
tail -n 37 ./build/linux-x64-debug/test-support/micro_vm_runtime_NMTBenchmark/micro.log | sed s/\?/" "/g > BenchmarkResults_with_debug.txt
python3 plot_result.py with_debug

#make test CONF=linux-x64 LOG=info JOBS=20 TEST="micro:vm.runtime.NMTBenchmark"
tail -n 37 ./build/linux-x64/test-support/micro_vm_runtime_NMTBenchmark/micro.log | sed s/\?/" "/g > BenchmarkResults_no_debug.txt
python3 plot_result.py no_debug