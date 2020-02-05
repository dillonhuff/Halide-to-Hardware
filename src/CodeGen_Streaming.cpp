#include "CodeGen_Streaming.h"

#include "CodeGen_C.h"

#include <fstream>

using namespace std;

namespace Halide {
namespace Internal {


  void streaming_codegen(Stmt& stmt, 
      const map<string, Function>& env) {
    for (const auto &p : env) {
      Function func = p.second;
      if(!func.schedule().is_accelerated()) {
        continue;
      }

      Target target;
      ofstream out(p.first + "_accel.cpp");
      CodeGen_C(out, target, CodeGen_C::OutputKind::CPlusPlusImplementation);
      out.close();
    }
  }

}
}
