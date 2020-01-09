#include "PreprocessHWLoops.h"

#include "Closure.h"
#include "IRMutator.h"
#include "Simplify.h"
#include "RemoveTrivialForLoops.h"
#include "HWUtils.h"
#include "UnrollLoops.h"

#include "coreir.h"

using namespace std;
using namespace CoreIR;

namespace Halide {

  namespace Internal {

    template<typename D, typename R>
      set<D> domain(const std::map<D, R>& m) {
        set<D> d;
        for (auto e : m) {
          d.insert(e.first);
        }
        return d;
      }

    // How do I assign final #s to statements?
    //  - New loop level only on for
    bool MemoryInfoCollector::isROM(const std::string& name) const {
      return CoreIR::elem(name, roms());
    }

    class InnermostLoopChecker : public IRGraphVisitor {
      public:
        bool foundSubLoop;

        InnermostLoopChecker() : foundSubLoop(false) {}

      protected:

        using IRGraphVisitor::visit;

        void visit(const For* f) override {
          foundSubLoop = true;
        }
    };

bool isInnermostLoop(const For* f) {
  InnermostLoopChecker c;
  f->body->accept(&c);
  return !c.foundSubLoop;
}

class LetPusher : public IRMutator {
  public:

    vector<const LetStmt*> letStack;

  protected:
    using IRMutator::visit;

    Stmt visit(const LetStmt* let) override {
      letStack.push_back(let);
      auto res = IRMutator::visit(let);
      letStack.pop_back();

      return res;
    }

    Stmt visit(const For* f) override {
      if (isInnermostLoop(f)) {
        cout << "Found innermost loop with var: " << f->name << endl;
        Stmt lBody = f->body;
        for (int i = ((int) letStack.size()) - 1; i >= 0; i--) {
          auto let = letStack[i];
          lBody = LetStmt::make(let->name, let->value, lBody);
        }

        return For::make(f->name, f->min, f->extent, f->for_type, f->device_api, lBody);
      } else {
        return IRMutator::visit(f);
      }
    }
};

class LetEraser : public IRMutator {

  public:

  protected:

    using IRMutator::visit;
    
    Stmt visit(const LetStmt* let) override {
      auto newBody = this->mutate(let->body);
      return newBody;
    }

    Stmt visit(const For* f) override {
      if (isInnermostLoop(f)) {
        return f;
      } else {
        return IRMutator::visit(f);
      }
    }
};

    int MemoryInfoCollector::romValue(const std::string& name, const int addr) {
      internal_assert(memOps.size() > 0);
      for (int i = ((int) memOps.size()) - 1; i >= 0; i--) {
        auto op = memOps[i];
        if (op.isStore() && is_const(op.store->index)) {
          int a = *as_const_int(op.store->index);
          if (a == addr) {
            internal_assert(is_const(op.store->value));
            return *as_const_int(op.store->value);
          }
        }
      }
      internal_assert(false);
      return 0;
    }

class ROMReadOptimizer : public IRMutator {
  public:

    MemoryInfoCollector mic;

    using IRMutator::visit;

    Expr visit(const Load* ld) {
      if (mic.isROM(ld->name) &&
          is_const(ld->index)) {
        int addr = *as_const_int(ld->index);
        int value = mic.romValue(ld->name, addr);
        return IntImm::make(Int(32), value);
      }
      
      return IRMutator::visit(ld);
    }
};


  Stmt preprocessHWLoops(const Stmt& stmt) {
    LetPusher pusher;
    Stmt pushed = pusher.mutate(stmt);
    LetEraser letEraser;
    Stmt r = letEraser.mutate(pushed);
    ROMReadOptimizer romReadOptimizer;

    MemoryInfoCollector mic;
    r.accept(&mic);
    romReadOptimizer.mic = mic;
    auto romOpt = romReadOptimizer.mutate(r);
    return romOpt;
  }

  class RealizeFinder : public IRGraphVisitor {
    public:

      using IRGraphVisitor::visit;

      const Realize* r;
      string target;

      RealizeFinder(const std::string& target_) : r(nullptr), target(target_) {}

      void visit(const Realize* rl) override {
        cout << "Searching realize: " << rl->name << " for " << target << endl;
        if (rl->name == target) {
          r = rl;
        } else {
          rl->body->accept(this);
        }
      }
  };

  class ComputeExtractor : public IRMutator {
    public:
      using IRMutator::visit;

      int ld_inst;
      int st_inst;

      map<const Provide*, int> provideNums;
      map<const Call*, int> callNums;

      ComputeExtractor() : ld_inst(0), st_inst(0) {}

      Expr visit(const Call* p) override {
        callNums[p] = ld_inst;
        auto fresh = Call::make(p->type,
            "compute_input",
            {Expr(ld_inst)},
            p->call_type);

        ld_inst++;
        return fresh;
      }

      Stmt visit(const Provide* p) override {
        provideNums[p] = st_inst;
        vector<Expr> vals;
        for (auto v : p->values) {
          vals.push_back(this->mutate(v));
        }
        auto fresh = Provide::make("compute_result", vals, {Expr(st_inst)});
        st_inst++;
        return fresh;
      }

      Stmt visit(const For* f) override {
        Stmt body = this->mutate(f->body);
        return body;
      }
      
      Stmt visit(const Realize* a) override {
        return this->mutate(a->body);
      }

      Stmt visit(const ProducerConsumer* a) override {
        return this->mutate(a->body);
      }

      Stmt visit(const AssertStmt* a) override {
        return Evaluate::make(0);
      }
  };

class VarSpec {
  public:
    std::string name;
    Expr min;
    Expr extent;
};

typedef std::vector<VarSpec> StmtSchedule;

std::ostream& operator<<(std::ostream& out, const VarSpec& e) {
  if (e.name != "") {
    out << e.name << " : [" << e.min << " " << simplify(e.min + e.extent - 1) << "]";
  } else {
    internal_assert(is_const(e.min));
    internal_assert(is_one(e.extent));
    out << e.min;
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const StmtSchedule& s) {
  for (auto e : s ) {
    out << e << ", ";
  }
  return out;
}

  class AbstractBuffer {
    public:
      string name;
      map<string, const Provide*> write_ports;
      map<string, const Call*> read_ports;

      // This is an ordinal schedule, it does not
      // describe cycle accurate timing of inputs and
      // outputs
      map<string, StmtSchedule> schedules;

      std::vector<Expr> port_address_stream(const std::string& str) const {
        vector<Expr> addrs;
        if (contains_key(str, write_ports)) {
          for (auto arg : map_find(str, write_ports)->args) {
            addrs.push_back(arg);
          }
        } else {
          internal_assert(contains_key(str, read_ports));
          for (auto arg : map_find(str, read_ports)->args) {
            addrs.push_back(arg);
          }
        }
        return addrs;
      }

      StmtSchedule port_schedule(const std::string& str) const {
        internal_assert(contains_key(str, schedules));
        return map_find(str, schedules);
      }
  };

  class FuncOpCollector : public IRGraphVisitor {
    public:
      using IRGraphVisitor::visit;

      vector<VarSpec> activeVars;
      int next_level;
      map<string, vector<const Provide*> > provides;
      map<string, vector<const Call*> > calls;
      map<const Provide*, StmtSchedule> write_scheds;
      map<const Call*, StmtSchedule> read_scheds;

      FuncOpCollector() : activeVars({{"", Expr(0), Expr(1)}}), next_level(1) {}

      map<string, AbstractBuffer> hwbuffers() const {
        map<string, AbstractBuffer> bufs;
        set<string> buffer_names = domain<string, vector<const Call*> >(calls);
        for (auto n : domain(provides)) {
          buffer_names.insert(n);
        }

        // Need to create port names for each read / write port

        for (auto b : buffer_names) {
          map<string, StmtSchedule> schedules;
          int rd_num = 0;
          map<string, const Call*> reads;
          for (auto rd : calls) {
            if (rd.first == b) {
              for (auto rdOp : rd.second) {
                reads["read_port_" + to_string(rd_num)] = rdOp;
                schedules["read_port_" + to_string(rd_num)] = map_find(rdOp, read_scheds);
                rd_num++;
              }
            }
          }
          int wr_num = 0;
          map<string, const Provide*> writes;
          for (auto rd : provides) {
            if (rd.first == b) {
              for (auto rdOp : rd.second) {
                writes["write_port_" + to_string(wr_num)] = rdOp;
                schedules["write_port_" + to_string(wr_num)] = map_find(rdOp, write_scheds);
                wr_num++;
              }
            }
          }
          bufs[b] = {b, writes, reads, schedules};
        }
        return bufs;
      }

      Expr last_provide_to(const std::string& name, const vector<Expr>& args) const {
        vector<const Provide*> ps = map_find(name, provides);
        CoreIR::reverse(ps);
        for (auto p : ps) {
          bool args_match = true;
          internal_assert(p->args.size() == args.size());
          for (size_t i = 0; i < p->args.size(); i++) {
            auto pa = p->args.at(i);
            auto a = args.at(i);
            if (!is_const(pa) || !is_const(a)) {
              args_match = false;
              break;
            }
            if (id_const_value(pa) != id_const_value(a)) {
              args_match = false;
              break;
            }
          }

          if (args_match) {
            internal_assert(p->values.size() == 1);
            return p->values.at(0);
          }
        }

        internal_assert(false) << "Could not find provide to location: " << name << "\n";
          //<< args << end;
        return args[0];
      }

      void visit(const For* f) override {
        activeVars.push_back({f->name, f->min, f->extent});
        push_level();
        f->body.accept(this);
        pop_level();
        activeVars.pop_back();
      }

      void inc_level() {
        auto back = activeVars.back();
        internal_assert(is_const(back.min)) << back.min << " is not const\n";
        activeVars[activeVars.size() - 1] = {"", simplify(back.min + 1), 1};
      }

      void push_level() {
        activeVars.push_back({"", Expr(0), Expr(1)});
        next_level = 1;
      }

      void pop_level() {
        activeVars.pop_back();
        next_level = 1;
      }
      void visit(const Provide* p) override {
        IRGraphVisitor::visit(p);

        inc_level();
        map_insert(provides, p->name, p);
        write_scheds[p] = activeVars;
      }
      
      void visit(const Call* p) override {
        if (p->call_type == Call::CallType::Halide) {
          inc_level();
          map_insert(calls, p->name, p);
          read_scheds[p] = activeVars;
        }

        IRGraphVisitor::visit(p);
      }

      bool is_rom(const std::string& func) const {
        if (!contains_key(func, provides)) {
          // If we have no information about this function assume it is
          // not going to be stored in a rom
          return false;
        }

        auto ps = map_find(func, provides);
        for (auto p : ps) {
          for (auto v : p->values) {
            if (!is_const(v)) {
              return false;
            }
          }
          for (auto a : p->args) {
            if (!is_const(a)) {
              return false;
            }
          }
        }
        return true;
      }
  };

  class ROMLoadFolder : public IRMutator {
    public:

      FuncOpCollector mic;

      using IRMutator::visit;

      Stmt visit(const AssertStmt* ld) override {
        return Evaluate::make(Expr(0));
      }

      Expr visit(const Call* ld) override {

        Expr newLd = IRMutator::visit(ld);

        if (ld->call_type != Call::CallType::Halide) {
          return newLd;
        }
        if (mic.is_rom(ld->name)) {
          cout << "found call to rom: " << ld->name << endl;
          //internal_assert(false) << ld->name << " is rom!\n";
          bool all_const_addr = true;
          for (auto a : ld->args) {
            if (!is_const(a)) {
              cout << "\targument: " << a << " is not const" << endl;
              all_const_addr = false;
              break;
            }
          }

          if (all_const_addr) {
            return mic.last_provide_to(ld->name, ld->args);
          }
        }

        return newLd;
      }

  };

  Stmt constant_fold_rom_buffers(const Stmt& stmt) {
    auto pre_simple = simplify(stmt);
    cout << "Pre simplification..." << endl;
    cout << pre_simple << endl;
    Stmt simple = simplify(remove_trivial_for_loops(simplify(unroll_loops(pre_simple))));
    cout << "Simplified code for stmt:" << endl;
    cout << simple << endl;

    FuncOpCollector mic;
    simple.accept(&mic);

    cout << "### New buffers in constant fold ROMs" << endl;
    for (auto b : mic.provides) {
      cout << "\t" << b.second.size() << " Provides to: " << b.first << endl;
      cout << "\t\tis rom " << mic.is_rom(b.first) << endl;
      if (contains_key(b.first, mic.calls)) {
        cout << "\t" << map_find(b.first, mic.calls).size() << " calls to: " << b.first << endl;
      }
    }

    ROMLoadFolder folder;
    folder.mic = mic;
    Stmt replaced = folder.mutate(simple);
    
    cout << "After ROM simplification..." << endl;
    cout << replaced << endl;
   
    {
      CoreIR::Context* context = newContext();
      auto ns = context->getNamespace("global");

      RecordType* mtp = context->Record({});
      auto m = ns->newModuleDecl("m", mtp);

      RealizeFinder rFinder("hw_output");
      replaced->accept(&rFinder);
      internal_assert(rFinder.r != nullptr);

      FuncOpCollector mic;
      replaced.accept(&mic);

      cout << "--- Hardware buffers" << endl;
      for (auto bufInfo : mic.hwbuffers()) {
        AbstractBuffer buf = bufInfo.second;
        cout << "\tFound buffer: " << buf.name << endl;
        cout << "\t\tReads..." << endl;
        for (auto rd : buf.read_ports) {
          cout << "\t\t\t" << rd.first << " : " << buf.port_schedule(rd.first) << " " << buf.port_address_stream(rd.first) << endl;
        }
        cout << "\t\tWrites..." << endl;
        for (auto rd : buf.write_ports) {
          cout << "\t\t\t" << rd.first << " : " << buf.port_schedule(rd.first) << buf.port_address_stream(rd.first) << endl;
        }

        // Classify the buffer?
        //  1. Pipeline
        //  2. Stencil
        //
        // What about boundary conditions? Inputs and outputs?

        vector<pair<string, CoreIR::Type*> > ubuffer_fields;
        RecordType* utp = context->Record(ubuffer_fields);
        CoreIR::Module* ubuffer = ns->newModuleDecl("unified_buffer_" + buf.name, utp);

        cout << "Unified buffer..." << endl;
        ubuffer->print();
      }


      //ComputeExtractor ce;
      //Stmt compute_only = simplify(ce.mutate(rFinder.r->body));
      //cout << "Compute logic..." << endl;
      //cout << compute_only << endl;

      //Closure interface;
      //rFinder.r->body.accept(&interface);
      //cout << "Interface..." << endl;
      //cout << "\tExternal vars..." << endl;
      //for (auto v : interface.vars) {
        //cout << "\t\t" << v.first << endl;
      //}

      //cout << "\t# external buffers = " << interface.buffers.size() << endl;
      //internal_assert(interface.buffers.size() == 0);

      cout << "Output module" << endl;
      m->print();

      deleteContext(context);
    }
    internal_assert(false) << "Stopping so dillon can view\n";
    return replaced;
  }


}
}
