#include "CodeGen_Streaming.h"

#include "Closure.h"
#include "CodeGen_C.h"
#include "HWUtils.h"
#include "RemoveTrivialForLoops.h"
#include "Simplify.h"
#include "UnrollLoops.h"

#include <fstream>

using namespace std;

namespace Halide {
namespace Internal {

  std::string comma_list(const std::vector<string>& strs) {
    if (strs.size() == 0) {
      return "";
    }

    string s = "";
    for (size_t i = 0; i < strs.size(); i++) {
      s += strs.at(i);
      if (i < strs.size() - 1) {
        s += ", ";
      }
    }
    return s;
  }

  class CodeGen_Streaming : public CodeGen_C {
    public:

      CodeGen_Streaming(std::ostream& out,
          Target& t) :
        CodeGen_C(out, t, CodeGen_C::OutputKind::CPlusPlusImplementation) {
        }

      void compileStmt(const std::string& name, const Stmt& s) {
        //do_indent();
        stream << "void " << name << "() {" << endl;
        cout << "Compiling stmt: " << s << endl;
        s.accept(this);
        cout << "Done Compiling stmt" << endl;
        stream << "}" << endl;
      }

      void visit(const Realize* op) override {
        stream << "// Realize: " << op->name << endl;
        op->body.accept(this);
      }

      void visit(const Call* op) override {
        if (op->call_type == Call::CallType::Image ||
            op->call_type == Call::CallType::Halide) {
          vector<string> args;
          //for (auto a : op->args) {
            //do_indent();
            //string arg = print_expr(a);
            //args.push_back(arg);
          //}
          ostringstream rhs;
          //rhs << print_name(op->name) << "(" << comma_list(args) << ")";
          rhs << print_name(op->name) << ".read()";
          print_assignment(op->type, rhs.str());
        } else {
          CodeGen_C::visit(op);
        }
      }

      void visit(const Provide* op) override {
        internal_assert(op->values.size() == 1);
        do_indent();
        string val = print_expr(op->values[0]);
        do_indent();
        stream << print_name(op->name) << ".write(" << val << ");" << endl;
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
      stream_codegen.compileStmt(p.first, rf.r->body);
      out.close();
      //assert(false);
    }
  }

}
}
