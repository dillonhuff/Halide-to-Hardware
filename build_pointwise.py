import os

def run_cmd(cmd):
    print('Running ', cmd)

    res = os.system(cmd)

    assert(res == 0)

run_cmd('cd ./apps/hardware_benchmarks/apps/pointwise/; make design-vhls')

# Generate ll file for examination
run_cmd('clang++ -std=c++11 -O1 -c -S -emit-llvm ./apps/hardware_benchmarks/apps/pointwise/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/pointwise/')

# Generate bc file for optimization
run_cmd('clang++ -std=c++11 -O0 -c -emit-llvm ./apps/hardware_benchmarks/apps/pointwise/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/pointwise/')

# Compile C++ testbench
run_cmd('clang++ -std=c++11 -O1 ./apps/hardware_benchmarks/apps/pointwise/target_tb.cpp ./apps/hardware_benchmarks/apps/pointwise/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/pointwise/')
# Run C++ testbench
run_cmd('./a.out')
