import os

def stencil_name(prefix, typename, e0, e1):
    s_name = '{0}_{1}_{2}_{3}_'.format(prefix, typename, e0, e1)
    return s_name

def axi_stencil_name(typename, e0, e1):
    return stencil_name('AxiPackedStencil', typename, e0, e1)

def gen_stream(typename):
    s_name = ''
    cdef = 'class ' + 'hls_stream_' + typename  + '_ {' + '\n'
    cdef += '\tint index;\n'
    cdef += '\t{0} elems[1000];\n'.format(typename)
    cdef += '  public:\n'
    cdef += '\t{0}();\n'.format('hls_stream_' + typename + '_')
    cdef += '\t{0} read();\n'.format(typename)
    cdef += '\tvoid write({0});\n'.format(typename)
    cdef += '};\n\n'
    return cdef

def decl_axi_packed_stencil(typename, e0, e1):
    axi_pre = 'AxiPackedStencil'
    cdef = 'class ' + stencil_name(axi_pre, typename, e0, e1)  + ';\n\n'
    return cdef;

def decl_packed_stencil(typename, e0, e1):
    axi_pre = 'PackedStencil'
    cdef = 'class ' + stencil_name(axi_pre, typename, e0, e1)  + ';\n\n'
    return cdef;

def decl_stencil(typename, e0, e1):
    axi_pre = 'Stencil'
    cdef = 'class ' + stencil_name(axi_pre, typename, e0, e1)  + ';\n\n'
    return cdef;

def gen_axi_packed_stencil(typename, e0, e1):
    axi_pre = 'AxiPackedStencil'
    sname = stencil_name(axi_pre, typename, e0, e1)
    cdef = 'class ' + sname  + ' {' + '\n'
    cdef += '\t{0} other_elems[{1}*{2} + 20];\n'.format(typename, e0, e1)
    cdef += '\t{0} elems[{1}*{2}];\n'.format(typename, e0, e1)    
    cdef += '  public:\n'
    cdef += '\tvoid set_last(const int);\n'
    cdef += '\tvoid set(const {0} c, const size_t e0=0, const size_t e1=0, const size_t e2=0);\n'.format(typename)
    cdef += '\t{0} get(const size_t e0=0, const size_t e1=0, const size_t e2=0);\n'.format(typename)
    cdef += '\t{0}();\n'.format(stencil_name('AxiPackedStencil', typename, e0, e1))
    cdef += '\t{0}(const {0}&);\n'.format(stencil_name('AxiPackedStencil', typename, e0, e1))    
    cdef += '\tvoid copy(const {0}& in);\n'.format(stencil_name('AxiPackedStencil', typename, e0, e1))

    cdef += '\t{0} operator=(const {0}&);\n'.format(stencil_name('AxiPackedStencil', typename, e0, e1))    
    #cdef += '\t{0} operator()(const size_t e0=0, const size_t e1=0, const size_t e2=0);\n'.format(typename)
    #cdef += '\t{0}({1}&);\n'.format(sname, stencil_name('PackedStencil', typename, e0, e1))    
    #cdef += '\toperator {0}();\n'.format(stencil_name('PackedStencil', typename, e0, e1))
    #cdef += '\toperator {0}();\n'.format(stencil_name('Stencil', typename, e0, e1))    
    cdef += '};\n\n'
    return cdef

def gen_packed_stencil(typename, e0, e1):
    axi_pre = 'PackedStencil'
    sname = stencil_name(axi_pre, typename, e0, e1)
    cdef = 'class ' + sname  + ' {' + '\n'
    cdef += '\t{0} other_elems[{1}*{2} + 20];\n'.format(typename, e0, e1)            
    cdef += '\t{0} elems[{1}*{2}];\n'.format(typename, e0, e1)        
    cdef += '  public:\n'
    cdef += '\t{0}();\n'.format(stencil_name('PackedStencil', typename, e0, e1))
    cdef += '\toperator {0}();\n'.format(stencil_name('AxiPackedStencil', typename, e0, e1))    
    cdef += '\tvoid set_last(const int);\n'
    cdef += '\t{0} operator()(const size_t e0=0, const size_t e1=0, const size_t e2=0);\n'.format(typename)
    cdef += '};\n\n'
    return cdef

def gen_stencil(typename, e0, e1):
    axi_pre = 'Stencil'
    cdef = 'class ' + stencil_name(axi_pre, typename, e0, e1)  + ' {' + '\n'
    cdef += '\t{0} other_elems[{1}*{2} + 20];\n'.format(typename, e0, e1)                
    cdef += '\t{0} elems[{1}*{2}];\n'.format(typename, e0, e1)            
    cdef += '  public:\n'
    cdef += '\t{0}();\n'.format(stencil_name('Stencil', typename, e0, e1))
    cdef += '\t{0}(const {0}&);\n'.format(stencil_name('Stencil', typename, e0, e1))
    cdef += '\t{0}(const {0}&&);\n'.format(stencil_name('Stencil', typename, e0, e1))
    cdef += '\t{0}& operator=(const {0}&);\n'.format(stencil_name('Stencil', typename, e0, e1))    
    cdef += '\tvoid set_last(const int);\n'
    cdef += '\t{0} operator()(const size_t e0=0, const size_t e1=0, const size_t e2=0);\n'.format(typename)
    cdef += '\tvoid write({0}, const size_t e0=0, const size_t e1=0, const size_t e2=0);\n'.format(typename)
    cdef += '\toperator {0}();\n'.format(stencil_name('AxiPackedStencil', typename, e0, e1))        
    cdef += '};\n\n'
    return cdef

def run_cmd(cmd):
    print('Running ', cmd)

    res = os.system(cmd)

    assert(res == 0)

app_name = 'cascade'    
run_cmd('cd ./apps/hardware_benchmarks/apps/{0}/; make design-vhls'.format(app_name))

# Generate auto-gen header files
f = open('./apps/hardware_benchmarks/apps/{0}/gen_classes.h'.format(app_name), 'w');

axi = decl_axi_packed_stencil('uint16_t', 1, 1)
f.write(axi)
axi = decl_axi_packed_stencil('int32_t', 1, 1)
f.write(axi)

axi = gen_axi_packed_stencil('uint16_t', 1, 1)
f.write(axi)
axi = gen_axi_packed_stencil('uint16_t', 3, 3)
f.write(axi)
axi = gen_axi_packed_stencil('int32_t', 1, 1)
f.write(axi)
axi = gen_axi_packed_stencil('int32_t', 3, 3)
f.write(axi)

f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'uint16_t', 1, 1), stencil_name('PackedStencil', 'uint16_t', 1, 1)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'uint16_t', 1, 1), stencil_name('Stencil', 'uint16_t', 1, 1)))

# axi = gen_packed_stencil('uint16_t', 1, 1)
# f.write(axi)

# axi = gen_stencil('uint16_t', 1, 1)
# f.write(axi)

ast = gen_stream(axi_stencil_name('uint16_t', 1, 1))
ast = gen_stream(axi_stencil_name('uint16_t', 3, 3))
ast = gen_stream(axi_stencil_name('int32_t', 1, 1))
ast = gen_stream(axi_stencil_name('int32_t', 3, 3))

f.write(ast)
f.write('\n')
f.close()


# Generate ll file for examination
run_cmd('clang++ -std=c++11 -O1 -c -S -emit-llvm ./apps/hardware_benchmarks/apps/{0}/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/{0}/'.format(app_name))

# Generate bc file for optimization
run_cmd('clang++ -std=c++11 -O0 -c -emit-llvm ./apps/hardware_benchmarks/apps/{0}/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/{0}/'.format(app_name))

# Compile C++ testbench
run_cmd('clang++ -std=c++11 -O1 ./apps/hardware_benchmarks/apps/{0}/target_tb.cpp ./apps/hardware_benchmarks/apps/{0}/bin/vhls_target.cpp  -I ./apps/hardware_benchmarks/apps/{0}/'.format(app_name))

# Run C++ testbench
run_cmd('./a.out')