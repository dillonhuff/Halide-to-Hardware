#include "CodeGen_Streaming.h"

#include "CodeGen_C.h"
#include "HWUtils.h"
#include "RemoveTrivialForLoops.h"
#include "Simplify.h"
#include "UnrollLoops.h"

#include <fstream>

using namespace std;

namespace Halide {
namespace Internal {

  class CodeGen_Streaming : public CodeGen_C {
    public:

      CodeGen_Streaming(std::ostream& out,
          Target& t) :
        CodeGen_C(out, t, CodeGen_C::OutputKind::CPlusPlusImplementation) {
        }

      void compileStmt(const Stmt& s) {
        cout << "Compiling stmt: " << s << endl;
        s.accept(this);
        cout << "Done Compiling stmt" << endl;
      }

      //void visit(const For* op) override {
        //op->accept(this);
        //stream << "// For loop: " << op->name << endl;
      //}

      void visit(const Realize* op) override {
        do_indent();
        stream << "// Realize: " << op->name << endl;
        op->body.accept(this);
        //this->visit(op->body);
        //op->accept(this);
      }

      void visit(const Call* op) override {
        do_indent();
        //op->accept(this);
        stream << "// Call: " << op->name << endl;
      }

      void visit(const Provide*op) override {
        do_indent();
        //op->accept(this);
        stream << "// Provide: " << op->name << endl;
      }
  };

  void streaming_codegen(Stmt& stmt, 
      const map<string, Function>& env) {

    for (const auto &p : env) {
      Function func = p.second;
      if(!func.schedule().is_accelerated()) {
        continue;
      }

      //Stmt simple = simplify(remove_trivial_for_loops(simplify(unroll_loops(simplify(rf.r->body)))));
      Stmt simple = simplify(remove_trivial_for_loops(simplify(unroll_loops(simplify(stmt)))));
      PCFinder rf(p.first);
      simple.accept(&rf);
      internal_assert(rf.r != nullptr) << "No realize for: " << p.first << " in \n " << simple << "\n";

      Target target;
      ofstream out(p.first + "_accel.cpp");
      CodeGen_Streaming stream_codegen(out, target);
      stream_codegen.compileStmt(rf.r->body);
      out.close();
      //assert(false);
    }
  }

}
}
