import os

def run_cmd(cmd):
    print('Running ', cmd)

    res = os.system(cmd)

    assert(res == 0)

run_cmd('cd ./apps/hardware_benchmarks/apps/pointwise/; make design-vhls')
run_cmd('clang++ -O1 -c -S -emit-llvm ./apps/hardware_benchmarks/apps/pointwise/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/pointwise/')
