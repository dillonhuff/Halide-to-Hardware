import os

def stencil_name(prefix, typename, e0, e1):
    s_name = '{0}_{1}_{2}_{3}_'.format(prefix, typename, e0, e1)
    return s_name
    
def gen_axi_packed_stencil(typename, e0, e1):
    axi_pre = 'AxiPackedStencil'
    # axi_name = 'AxiPackedStencil_{0}_{1}_'.format(typename, e0, e1)
    cdef = 'class ' + stencil_name(axi_pre, typename, e0, e1)  + ' {' + '\n'
    cdef += '  public:\n'
    cdef += '\tvoid set_last(const int);\n'
    cdef += '\t{0}& read();\n'.format(typename)
    cdef += '\tvoid write({0}&);\n'.format(typename)
    cdef += '};\n'
    return cdef

def run_cmd(cmd):
    print('Running ', cmd)

    res = os.system(cmd)

    assert(res == 0)

run_cmd('cd ./apps/hardware_benchmarks/apps/pointwise/; make design-vhls')

# Generate auto-gen header files
f = open('./apps/hardware_benchmarks/apps/pointwise/gen_classes.h', 'w');
axi = gen_axi_packed_stencil('uint16_t', 1, 1)
f.write(axi)
f.write('\n')
f.close()


# Generate ll file for examination
run_cmd('clang++ -std=c++11 -O1 -c -S -emit-llvm ./apps/hardware_benchmarks/apps/pointwise/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/pointwise/')

# Generate bc file for optimization
run_cmd('clang++ -std=c++11 -O0 -c -emit-llvm ./apps/hardware_benchmarks/apps/pointwise/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/pointwise/')

# Compile C++ testbench
run_cmd('clang++ -std=c++11 -O1 ./apps/hardware_benchmarks/apps/pointwise/target_tb.cpp ./apps/hardware_benchmarks/apps/pointwise/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/pointwise/')
# Run C++ testbench
run_cmd('./a.out')
