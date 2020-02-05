#ifndef HALIDE_CODEGEN_STREAMING_H
#define HALIDE_CODEGEN_STREAMING_H

#include <sstream>

#include "IRVisitor.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {
namespace Internal {

  void streaming_codegen(Stmt& stmt, const std::map<std::string, Function>& env);

}
}

#endif

