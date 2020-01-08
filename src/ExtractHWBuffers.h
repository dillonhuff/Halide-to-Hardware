#ifndef HALIDE_EXTRACT_HW_BUFFERS_H
#define HALIDE_EXTRACT_HW_BUFFERS_H

/** \file
 *
 * Defines a hardware buffer extraction pass, which finds the parameters
 * for a general buffer that can then be outputted to a hardware backend.
 * These parameters provide specification for line buffers and double
 * buffers.
 * 
 */

#include <map>
#include <set>
#include <vector>
#include <iostream>

#include "IR.h"
#include "ExtractHWKernelDAG.h"
#include "HWUtils.h"
#include "SlidingWindow.h"

namespace Halide {
namespace Internal {

//struct Stride {
  //int stride;
  //bool is_inverse;
  //Stride(int stride=0, bool is_inverse=false) :
    //stride(stride), is_inverse(is_inverse) {}

//};

struct MergedDimSize {
  std::string loop_name;
  Expr logical_size;
  Expr logical_min;

  Expr input_chunk;
  Expr input_block;

  Expr output_stencil;
  Expr output_block;
  Expr output_min_pos;
  Expr output_max_pos;
};

struct InputDimSize {
  std::string loop_name;
  Expr input_chunk;
  Expr input_block;
};

struct InputStream {
  std::string name;
  std::vector<InputDimSize> idims;
  Stmt input_access_pattern;
  std::map<std::string, Stride> stride_map;
};

struct OutputDimSize {
  std::string loop_name;
  Expr output_stencil;
  Expr output_block;
  Expr output_min_pos;
  Expr output_max_pos;
};

struct HWBuffer; // forward declare
struct OutputStream {
  std::string name;
  std::vector<OutputDimSize> odims;
  Stmt output_access_pattern;
  std::map<std::string, Stride> stride_map;
  //std::shared_ptr<HWBuffer> hwref;
  HWBuffer* hwref;
};

struct RMWStream {
  std::string name;
  std::vector<InputDimSize> idims;
  std::vector<OutputDimSize> mdims;
  std::vector<OutputDimSize> odims;
  Stmt input_access_pattern;
  Stmt modify_access_pattern;
  Stmt output_access_pattern;
  std::map<std::string, Stride> stride_map;
  //std::shared_ptr<HWBuffer> ohwref;
  HWBuffer* ohwref;
};

struct InOutDimSize {
  std::string loop_name;

  Expr input_chunk;    // replace stencilfor, dispatch, need_hwbuffer, add_hwbuffer,
                       // transform_hwkernel, xcel inserthwbuffers
  Expr input_block;    // add_hwbuffer

  Expr output_stencil; // add_hwbuffer, dispatch
  Expr output_block;   // add_hwbuffer
  //Expr output_min_pos; // in provide+call shifts
  //Expr output_max_pos; // not used yet?
};

struct LogicalDimSize {
  Expr var_name;
  Expr logical_size;
  Expr logical_min;
};

struct AccessDimSize {
  Expr range;
  Expr stride;
  Expr dim_ref;
};

std::vector<MergedDimSize> create_hwbuffer_sizes(std::vector<int> logical_size,
                                                 std::vector<int> output_stencil,
                                                 std::vector<int> output_block,
                                                 std::vector<int> input_chunk,
                                                 std::vector<int> input_block);

std::vector<AccessDimSize> create_linear_addr(std::vector<int> range,
                                              std::vector<int> stride,
                                              std::vector<int> dim_ref);

struct HWBuffer {
  std::string name;
  std::string store_level;
  std::string compute_level;
  std::vector<std::string> streaming_loops;
  //LoopLevel store_looplevel;
  //LoopLevel compute_looplevel = LoopLevel::inlined();

  Stmt my_stmt;
  Function func;
  bool is_inlined = false;
  bool is_output = false;
  std::vector<Expr> output_kernel_min_pos;
  int num_accum_iters = 0;
  
  // old parameters for the HWBuffer
  std::shared_ptr<SlidingStencils> input_stencil;
  std::vector<InOutDimSize> dims;
  Stmt input_access_pattern;
  Stmt output_access_pattern;
  std::map<std::string, HWBuffer*> producer_buffers;
  //std::map<std::string, std::shared_ptr<HWBuffer>> producer_buffers;
  std::map<std::string, HWBuffer*> consumer_buffers;   // used for transforming call nodes and inserting dispatch calls
  //std::map<std::string, std::shared_ptr<HWBuffer>> consumer_buffers;   // used for transforming call nodes and inserting dispatch calls
  std::vector<std::string> input_streams;  // used when inserting read_stream calls; should make a set?
  std::map<std::string, Stride> stride_map;
  std::vector<AccessDimSize> linear_addr;

  // dimensions for the hwbuffer
  std::vector<LogicalDimSize> ldims;
  std::map<std::string, InputStream> istreams;
  std::map<std::string, OutputStream> ostreams;
  
  // Constructors
  HWBuffer() : input_stencil(nullptr) { }

  //HWBuffer(const HWBuffer &b) = delete;

  HWBuffer(std::string name, std::vector<MergedDimSize> mdims, std::vector<AccessDimSize> linear_addr,
           std::vector<std::string> loops, int store_index, int compute_index, bool is_inlined, bool is_output,
           std::string iname="input", std::string oname="output");
  
};

std::ostream& operator<<(std::ostream& os, const std::vector<Expr>& vec);
std::ostream& operator<<(std::ostream& os, const HWBuffer& buffer);

std::map<std::string, HWBuffer> extract_hw_buffers(Stmt s, const std::map<std::string, Function> &env,
                                                   const std::vector<std::string> &streaming_loop_names);


struct HWXcel {
  std::string name;
  Function func;
  LoopLevel store_level;
  LoopLevel compute_level;
  
  std::vector<std::string> streaming_loop_levels;  // store (exclusive) to compute (inclusive)
  std::set<std::string> input_streams; // might be wrong?
  std::map<std::string, HWBuffer> hwbuffers; // delete default copy constructor?
  std::map<std::string, HWBuffer*> consumer_buffers; // used for transforming call nodes and inserting dispatch calls

  //std::map<std::string, HWTap> input_taps;
};

std::vector<HWXcel> extract_hw_accelerators(Stmt s, const std::map<std::string, Function> &env,
                                            const std::vector<BoundsInference_Stage> &inlined_stages);


std::ostream& operator<<(std::ostream& os, const std::vector<string>& vec);
int id_const_value(const Expr e);
std::vector<std::string> get_tokens(const std::string &line, const std::string &delimiter);
int to_int(Expr expr);

class IdentifyAddressing : public IRVisitor {
  Function func;
  //int stream_dim_idx;
  const Scope<Expr> &scope;
  
  using IRVisitor::visit;
  
  void visit(const For *op);

public:
  const vector<string> &storage_names;
  const map<string,Stride> &stride_map;
  vector<string> varnames;

  map<string, int> dim_map;
  
  vector<int> ranges;
  vector<int> dim_refs;
  vector<int> strides_in_dim;
  IdentifyAddressing(const Function& func, const Scope<Expr> &scope, const map<string,Stride> &stride_map);
};

}  // namespace Internal
}  // namespace Halide



#endif
