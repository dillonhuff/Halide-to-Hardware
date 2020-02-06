#ifndef HALIDE_CODEGEN_STREAMING_H
#define HALIDE_CODEGEN_STREAMING_H

#include <sstream>

#include "IRVisitor.h"
#include "Interval.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {
namespace Internal {

  void streaming_codegen(Stmt& stmt,
      const std::map<std::string, Function>& env,
      const std::map<std::pair<std::string, int>, Interval>& bounds);

}
}

#endif

