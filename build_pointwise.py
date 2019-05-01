import os

def stencil_name(prefix, typename, e0, e1):
    s_name = '{0}_{1}_{2}_{3}_'.format(prefix, typename, e0, e1)
    return s_name

def axi_stencil_name(typename, e0, e1):
    return stencil_name('AxiPackedStencil', typename, e0, e1)

def packed_stencil_name(typename, e0, e1):
    return stencil_name('PackedStencil', typename, e0, e1)

def stream_name(typename):
    return 'hls_stream_' + typename + '_'

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

#app_name = 'harris'
app_name = 'cascade'
run_cmd('cd ./apps/hardware_benchmarks/apps/{0}/; make design-vhls'.format(app_name))

# Generate auto-gen header files
f = open('./apps/hardware_benchmarks/apps/{0}/gen_classes.h'.format(app_name), 'w');

axi = decl_axi_packed_stencil('uint16_t', 1, 1)
f.write(axi)
axi = gen_axi_packed_stencil('uint16_t', 1, 1)
f.write(axi)

axi = decl_axi_packed_stencil('int32_t', 1, 1)
f.write(axi)
axi = gen_axi_packed_stencil('uint16_t', 3, 3)
f.write(axi)

axi = gen_axi_packed_stencil('int32_t', 1, 1)
f.write(axi)

axi = gen_axi_packed_stencil('int32_t', 3, 3)
f.write(axi)

axi = decl_axi_packed_stencil('int16_t', 1, 1)
f.write(axi)

axi = gen_axi_packed_stencil('int16_t', 1, 1)
f.write(axi)

axi = gen_axi_packed_stencil('int16_t', 3, 3)
f.write(axi)

f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'uint16_t', 1, 1), stencil_name('PackedStencil', 'uint16_t', 1, 1)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'uint16_t', 1, 1), stencil_name('Stencil', 'uint16_t', 1, 1)))

f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int32_t', 1, 1), stencil_name('PackedStencil', 'int32_t', 1, 1)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int32_t', 1, 1), stencil_name('Stencil', 'int32_t', 1, 1)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int32_t', 3, 3), stencil_name('Stencil', 'int32_t', 3, 3)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int32_t', 3, 3), stencil_name('PackedStencil', 'int32_t', 3, 3)))

f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'uint16_t', 3, 3), stencil_name('PackedStencil', 'uint16_t', 3, 3)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'uint16_t', 3, 3), stencil_name('Stencil', 'uint16_t', 3, 3)))

f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int16_t', 1, 1), stencil_name('PackedStencil', 'int16_t', 1, 1)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int16_t', 1, 1), stencil_name('Stencil', 'int16_t', 1, 1)))

f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int16_t', 1, 1), stencil_name('PackedStencil', 'int16_t', 3, 3)))
f.write('typedef {0} {1};\n'.format(stencil_name('AxiPackedStencil', 'int16_t', 1, 1), stencil_name('Stencil', 'int16_t', 3, 3)))

ast = gen_stream(axi_stencil_name('uint16_t', 1, 1))
f.write(ast)

ast = gen_stream(axi_stencil_name('int16_t', 1, 1))
f.write(ast)

ast = gen_stream(axi_stencil_name('uint16_t', 3, 3))
f.write(ast)

ast = gen_stream(axi_stencil_name('int16_t', 3, 3))
f.write(ast)

ast = gen_stream(axi_stencil_name('int32_t', 1, 1))
f.write(ast)

ast = gen_stream(axi_stencil_name('int32_t', 3, 3))
f.write(ast)

# Packed stencils
ast = gen_stream(packed_stencil_name('uint16_t', 1, 1))
f.write(ast)

ast = gen_stream(packed_stencil_name('int16_t', 1, 1))
f.write(ast)

ast = gen_stream(packed_stencil_name('uint16_t', 3, 3))
f.write(ast)

ast = gen_stream(packed_stencil_name('int16_t', 3, 3))
f.write(ast)

ast = gen_stream(packed_stencil_name('int32_t', 1, 1))
f.write(ast)

ast = gen_stream(packed_stencil_name('int32_t', 3, 3))
f.write(ast)

def declare_linebuffer(s0Name, s1Name, ext0, ext1):
    fullLbName = 'linebuffer_' + stream_name(s0Name) + '_to_' + stream_name(s1Name) + '_bnds_' + str(ext0) + '_' + str(ext1)
    return 'class ' + fullLbName + ' {' + '};\n'

def declare_ram(typename, depth):
    ramName = 'ram_' + typename + '_' + str(depth)
    return 'class ' + ramName + ' {' + '};\n'

s0Name = packed_stencil_name('int32_t', 1, 1)
s1Name = packed_stencil_name('int32_t', 3, 3)
lb0 = declare_linebuffer(s0Name, s1Name, 62, 62)
f.write(lb0)

s0Name = axi_stencil_name('uint16_t', 1, 1)
s1Name = packed_stencil_name('uint16_t', 3, 3)
lb0 = declare_linebuffer(s0Name, s1Name, 64, 64)
f.write(lb0)

ramName = declare_ram('int32_t', 9)
f.write(ramName)

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
