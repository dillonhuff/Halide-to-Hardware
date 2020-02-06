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

  std::string sep_list(const std::vector<string>& strs,
      const string& ld, const string& rd, const string& mid) {
    if (strs.size() == 0) {
      return ld + rd;
    }

    string s = ld;
    for (size_t i = 0; i < strs.size(); i++) {
      s += strs.at(i);
      if (i < strs.size() - 1) {
        s += mid;
      }
    }
    return s + rd;
  }

  std::string comma_list(const std::vector<string>& strs) {
    return sep_list(strs, "", "", ", ");
  }
  class CodeGen_Streaming : public CodeGen_C {
    public:

      set<string> external_buffers;

      CodeGen_Streaming(std::ostream& out,
          Target& t) :
        CodeGen_C(out, t, CodeGen_C::OutputKind::CPlusPlusImplementation) {
        }

      string sanitize_c(const string& n) {
        return print_name(n);
      }

      bool external_buffer(const std::string& n) {
        return elem(n, external_buffers);
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
            arg_strings.push_back("hw_stream<int > & " + print_name(b.name));
            external_buffers.insert(b.name);
          } else {
            Box bt = box_touched(s, b.name);
            vector<string> bnd_strings;
            for (auto i : bt.bounds) {
              ostringstream oss;
              oss << "(" << i.max << " - " << i.min << " + 1)";
              bnd_strings.push_back(oss.str());
            }
            string size = comma_list(bnd_strings);
            local_buf_strings.push_back("hwbuffer<int, " + size +" >" + print_name(b.name));
            stream << "// Box of " << b.name << " touched: " << bt << endl;
          }
        }

        stream << "template<typename T, ";
        vector<string> dim_strings;
        for (int i = 0; i < 5; i++) {
          dim_strings.push_back("int extent_" + to_string(i) + " = 1");
        }
        stream << comma_list(dim_strings) << "> ";
        stream << "class hwbuffer {" << endl;
        stream << "\tpublic:" << endl;
        stream << "\tT buf[extent_0*extent_1*extent_2*extent_3*extent_4];" << endl;
        stream << "\tT read(const int e0=0, const int e1=0, const int e2=0, const int e3=0, const int e4=0) { return buf[e0*extent_0 + e1*extent_0*extent_1 + e2*extent_0*extent_1*extent_2 + e3*extent_0*extent_1*extent_2*extent_3 + e4*extent_0*extent_1*extent_2*extent_3*extent_4]; }" << endl;
        stream << "\tvoid write(const T& value, const int e0=0, const int e1=0, const int e2=0, const int e3=0, const int e4=0) { }" << endl;
        stream << "};" << endl << endl;

        stream << "template<typename T> class hw_stream { public:" << endl;
        stream << "\tT read() { return 0; }" << endl;
        stream << "\tvoid write(const T& value) { }" << endl;
        stream << "};" << endl << endl;

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
          vector<string> vs;
          if (!external_buffer(op->name)) {
            for (auto a : op->args) {
              string v = print_expr(a);
              vs.push_back(v);
            }
          }
          rhs << print_name(op->name) << ".read(" << comma_list(vs) << ")";
          print_assignment(op->type, rhs.str());
        } else {
          CodeGen_C::visit(op);
        }
      }

      void visit(const Provide* op) override {
        internal_assert(op->values.size() == 1);
        do_indent();
        string val = print_expr(op->values[0]);
        vector<string> vs{val};
        if (!external_buffer(op->name)) {
          for (auto a : op->args) {
            string v = print_expr(a);
            vs.push_back(v);
          }
        }
        //string vs = print_assignment(op->values[0].type(), val);
        do_indent();
        stream << print_name(op->name) << ".write(" << comma_list(vs) << ");" << endl;
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
