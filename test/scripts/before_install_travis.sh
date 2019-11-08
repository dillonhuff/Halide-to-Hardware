#!/bin/bash -x

# Needed for new libstdc++ and gcc4.x
# Upgrade gcc here
export CXX=${CXX_}
export CC=${CC_}
export LLVM_CONFIG=${TRAVIS_BUILD_DIR}/llvm/bin/llvm-config
export CLANG=${TRAVIS_BUILD_DIR}/llvm/bin/clang
export COREIRCONFIG=${CXX_}
export COREIR_DIR=${TRAVIS_BUILD_DIR}/coreir
export FUNCBUF_DIR=${TRAVIS_BUILD_DIR}/BufferMapping/cfunc
export OUTPUT_REDIRECTION=""
