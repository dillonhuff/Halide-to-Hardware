#include "UBufferRewrites.h"

#include "RemoveTrivialForLoops.h"
#include "UnrollLoops.h"
#include "CoreIR_Libs.h"

#include "coreir.h"
#include "coreir/libs/commonlib.h"

using namespace CoreIR;
using namespace std;

namespace Halide {
  namespace Internal {

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

  void synthesize_ubuffer(CoreIR::ModuleDef* def, MemoryConstraints& buffer) {
    if (buffer.read_ports.size() == 0) {
      // Buffer is write only, so it has no effect
      return;
    }

    auto self = def->sel("self");
    if (buffer.read_loop_levels().size() == 1) {
      if (buffer.write_loop_levels().size() == 1) {
        if (buffer.read_loop_levels()[0] == buffer.write_loop_levels()[0]) {
          set<string> ports = buffer.port_names();
          vector<string> port_list(begin(ports), end(ports));
          sort(begin(port_list), end(port_list), [buffer](const std::string& pta, const std::string& ptb) {
              return buffer.lt(buffer.schedule(pta).back(), buffer.schedule(ptb).back());
          });

          cout << "Buffer: " << buffer.name << " is all wires!" << endl;
          string last_pt = port_list[0];
          internal_assert(buffer.is_write(last_pt)) << "Buffer: " << buffer.name << " is read before it is written?\n";

          for (size_t i = 1; i < port_list.size(); i++) {
            string next_pt = port_list[i];

            if (buffer.is_write(next_pt)) {
              auto last_cntrl = def->sel("self." + last_pt + "_" + (buffer.is_write(last_pt) ? "en" : "valid"));
              auto last_data = def->sel("self." + last_pt);

              auto next_cntrl = def->sel("self." + next_pt + "_" + (buffer.is_write(next_pt) ? "en" : "valid"));
              auto next_data = def->sel("self." + next_pt);

              def->connect(last_cntrl, next_cntrl);
              def->connect(last_data, next_data);
            }

            last_pt = next_pt;
          }
          return;
        } else {
          internal_assert(buffer.write_ports.size() == 1);

          string wp = begin(buffer.write_ports)->first;

          coreir_builder_set_context(def->getContext());
          coreir_builder_set_def(def);

          auto wen = self->sel(wp + "_en");
          // TODO: Populate counter with real values
          Instance* en_cnt_last = build_counter(def, 16, 0, 16, 1);
          def->connect(en_cnt_last->sel("en"), self->sel(wp + "_en"));
          def->connect(en_cnt_last->sel("reset"), self->sel("reset"));

          auto row_cnt = udiv(en_cnt_last->sel("out"), 4);
          auto col_cnt = sub(en_cnt_last->sel("out"), mul(row_cnt, 4));

          auto context = def->getContext();

          Wireable* inside_out_row = geq(row_cnt, 2);
          Wireable* inside_out_col = geq(col_cnt, 2);
          Wireable* started = andList(def,
              {geq(en_cnt_last->sel("out"), 1),
              inside_out_row,
              inside_out_col,
              wen});

          // TODO: Set to real max value
          int min_row_offset = 10000;
          int min_col_offset = 10000;
          int max_row_offset = -1;
          int max_col_offset = -1;
          vector<Expr> baseArgs = map_find(string("read_port_0"), buffer.read_ports)->args;
          for (auto rp : buffer.read_ports) {
            vector<Expr> args = rp.second->args;
            vector<int> offsets;
            internal_assert(args.size() == 2);
            for (size_t i = 0; i < baseArgs.size(); i++) {
              offsets.push_back(id_const_value(simplify(args[i] - baseArgs[i])));
            }

            if (offsets[1] < min_row_offset) {
              min_row_offset = offsets[1];
            }
            if (offsets[0] < min_col_offset) {
              min_col_offset = offsets[0];
            }


            if (offsets[1] > max_row_offset) {
              max_row_offset = offsets[1];
            }

            if (offsets[0] > max_col_offset) {
              max_col_offset = offsets[0];
            }
          }

          max_row_offset += -min_row_offset;
          max_col_offset += -min_col_offset;

          internal_assert(max_col_offset >= 0);
          internal_assert(max_row_offset >= 0);

          vector<Wireable*> rowDelays{self->sel(wp)};
          vector<Wireable*> rowDelayValids{wen};
          for (int i = 1; i < max_row_offset + 1; i++) {
            auto lastD = rowDelays[i - 1];
            auto lastEn = rowDelayValids[i - 1];
            // Create delays
            auto r0Delay =
              def->addInstance("r" + to_string(i) + "_delay" + context->getUnique(),
                  "memory.rowbuffer",
                  {{"width", COREMK(context, 16)}, {"depth", COREMK(context, 4)}});
            def->connect(r0Delay->sel("wdata"), lastD);
            def->connect(r0Delay->sel("wen"), lastEn);
            def->connect(r0Delay->sel("flush"), self->sel("reset"));

            rowDelays.push_back(r0Delay->sel("rdata"));
            rowDelayValids.push_back(r0Delay->sel("valid"));
          }

          vector<vector<Wireable*> > colDelays;
          for (size_t i = 0; i < rowDelays.size(); i++) {
            cout << "\ti = " << i << endl;
            auto d = rowDelays.at(i);
            auto dValid = rowDelayValids.at(i);
            Wireable* lastReg = d;
            Wireable* lastEn = dValid;

            internal_assert(dValid != nullptr);
            vector<Wireable*> cds{d};

            for (int c = 1; c < max_col_offset + 1; c++) {
              auto d0 = def->addInstance("d0" + context->getUnique(),"mantle.reg",{{"width",Const::make(context,16)},{"has_en",Const::make(context,true)}});
              def->connect(d0->sel("in"), lastReg);
              def->connect(d0->sel("en"), lastEn);

              auto delayedEn = def->addInstance("delayed_en" + context->getUnique(),"corebit.reg");
              def->connect(dValid, delayedEn->sel("in"));

              lastReg = d0->sel("out");
              lastEn = delayedEn->sel("out");

              cds.push_back(d0->sel("out"));
            }
            colDelays.push_back(cds);
          }

          for (auto rp : buffer.read_ports) {
            vector<Expr> args = rp.second->args;
            vector<int> offsets;
            internal_assert(args.size() == 2);
            for (size_t i = 0; i < baseArgs.size(); i++) {
              offsets.push_back(id_const_value(simplify(args[i] - baseArgs[i])));
            }

            int rowOffset = max_row_offset - offsets[1] + min_row_offset;
            int colOffset = max_col_offset - offsets[0] + min_col_offset;

            internal_assert(rowOffset >= 0);
            internal_assert(colOffset >= 0);
            internal_assert(rowOffset < (int) colDelays.size());

            def->connect(started, self->sel(rp.first + "_valid"));
            auto read_data = self->sel(rp.first);
            internal_assert(rowOffset < (int) colDelays.size());
            def->connect(colDelays.at(rowOffset).at(colOffset), read_data);
          }

          return;
        }
      }
    }

    // Dummy definition
    internal_assert(buffer.write_ports.size() > 0);

    string wp = begin(buffer.write_ports)->first;
    auto write_en = self->sel(wp + "_en");
    auto write_data = self->sel(wp);
    for (auto rp : buffer.read_ports) {
      auto read_valid = self->sel(rp.first + "_valid");
      auto read_data = self->sel(rp.first);

      def->connect(write_en, read_valid);
      def->connect(write_data, read_data);
    }

    //internal_assert(false) << "Cannot classify buffer: " << buffer.name << "\n";
  }

map<string, CoreIR::Module*>
synthesize_hwbuffers(const Stmt& stmt, const std::map<std::string, Function>& env, std::vector<HWXcel>& xcels) {
  Stmt simple = simplify(remove_trivial_for_loops(simplify(unroll_loops(simplify(stmt)))));

  cout << "Doing rewrites for" << endl;
  cout << simple << endl;

  FuncOpCollector mic;
  simple.accept(&mic);
  auto buffers = mic.hwbuffers();

  CoreIR::Context* context = newContext();
  CoreIRLoadLibrary_commonlib(context);
  auto ns = context->getNamespace("global");

  for (auto f : env) {
    if (f.second.schedule().is_accelerated() ||
        f.second.schedule().is_accelerator_input()) {
        //f.second.schedule().is_accelerator_input() ||
        //f.second.schedule().is_hw_kernel()) {

      cout << "Buffer for " << f.first << endl;
      internal_assert(contains_key(f.first, buffers)) << f.first << " was not found in memory analysis\n";
      MemoryConstraints buf = map_find(f.first, buffers);
      // Add hwbuffer field to memory constraints wrapper
      for (auto xcel : xcels) {
        for (auto buffer : xcel.hwbuffers) {
          if (buffer.first == buf.name) {
            buf.ubuf = buffer.second;
          }
        }
      }
      vector<pair<string, CoreIR::Type*> > ubuffer_fields{{"clk", context->Named("coreir.clkIn")}, {"reset", context->BitIn()}};
      cout << "\t\tReads..." << endl;
      for (auto rd : buf.read_ports) {
        cout << "\t\t\t" << rd.first << " : " << buf.port_schedule(rd.first) << " " << buf.port_address_stream(rd.first) << endl;
        ubuffer_fields.push_back({rd.first + "_valid", context->Bit()});
        ubuffer_fields.push_back({rd.first, context->Bit()->Arr(16)});
      }
      cout << "\t\tWrites..." << endl;
      for (auto rd : buf.write_ports) {
        cout << "\t\t\t" << rd.first << " : " << buf.port_schedule(rd.first) << buf.port_address_stream(rd.first) << endl;
        ubuffer_fields.push_back({rd.first + "_en", context->BitIn()});
        ubuffer_fields.push_back({rd.first, context->BitIn()->Arr(16)});
      }

      RecordType* utp = context->Record(ubuffer_fields);
      auto ub = ns->newModuleDecl(f.first + "_ubuffer", utp);
      auto def = ub->newModuleDef();
      synthesize_ubuffer(def, buf);
      ub->setDef(def);
    } else {
      cout << f.first << " is not accelerated" << endl;
      //internal_assert(!f.second.schedule().is_hw_kernel());
    }
  }

  // Save ubuffers and finish
  if (!saveToFile(ns, "ubuffers.json")) {
    cout << "Could not save ubuffers" << endl;
    context->die();
  }
  deleteContext(context);

  return {};

}

  }
}
