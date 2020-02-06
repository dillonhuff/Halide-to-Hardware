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

      string sanitize_c(const string& n) {
        return print_name(n);
      }

      void compileStmt(const std::string& n, const Stmt& s,
          const map<pair<string, int>, Interval>& bounds) {

        string name = n;
        cout << "Generating code for: " << s << endl;

        FuncOpCollector collector;
        s.accept(&collector);
        vector<string> arg_strings;
        vector<string> local_buf_strings;
        for (auto bn : collector.hwbuffers()) {
          auto b = bn.second;
          if (b.write_loop_levels().size() == 0 ||
              b.read_loop_levels().size() == 0) {
            arg_strings.push_back("hwbuffer& " + print_name(b.name));
          } else {
            local_buf_strings.push_back("hwbuffer " + print_name(b.name));
            Box bt = box_touched(s, b.name);
            stream << "// Box of " << b.name << " touched: " << bt << endl;
          }
        }

        //stream << "Bounds:" << endl;
        //for (auto b : bounds) {
          //stream << "\tBound: " << b.first.first << "[" << b.first.second << "] = " << b.second.min << " to " << b.second.max << endl;
        //}
        stream << "class hwbuffer { public: int read() { return 0; } void write(const int value) { } };" << endl << endl;
        //do_indent();
        stream << "void " << name << "(" << comma_list(arg_strings) << ") {" << endl;
        for (auto s : local_buf_strings) {
          do_indent();
          stream << s << ";" << endl;
        }
        stream << endl << endl;
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
        if (op->is_intrinsic(Call::likely_if_innermost)) {
          op->args[0].accept(this);
        } else if (op->call_type == Call::CallType::Image ||
            op->call_type == Call::CallType::Halide) {
          vector<string> args;
          ostringstream rhs;
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
        //string vs = print_assignment(op->values[0].type(), val);
        do_indent();
        stream << print_name(op->name) << ".write(" << val << ");" << endl;
        cache.clear();
      }
  };

  void streaming_codegen(Stmt& stmt, 
      const map<string, Function>& env,
      const map<pair<string, int>, Interval>& bounds) {

    for (const auto &p : env) {
      Function func = p.second;
      if(!func.schedule().is_accelerated()) {
        continue;
      }

      Stmt simple = simplify(remove_trivial_for_loops(simplify(unroll_loops(simplify(stmt)))));
      PCFinder rf(p.first);
      simple.accept(&rf);
      internal_assert(rf.r != nullptr) << "No realize for: " << p.first << " in \n " << simple << "\n";

      Target target;
      ofstream out(p.first + "_accel.cpp");
      CodeGen_Streaming stream_codegen(out, target);
      stream_codegen.compileStmt(stream_codegen.sanitize_c(p.first), rf.r->body, bounds);
      out.close();
    }
  }

}
}
