#include "Halide.h"

namespace {

using namespace Halide;
using namespace std;

class PointwiseMultiplication : public Halide::Generator<PointwiseMultiplication> {
public:
    Input<Buffer<uint16_t>>  input{"input", 2};
    Output<Buffer<uint16_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func hw_input, hw_output;
        Func mult("mult");
        hw_input(x, y) = input(x, y);
        mult(x, y) = hw_input(x,y) * 2;
        hw_output(x, y) = cast<uint16_t>(mult(x, y));
        output(x, y) = hw_output(x, y);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR) || get_target().has_feature(Target::HLS)) {
          Var xi,yi, xo,yo;

          //mult.compute_at(hw_output, x);
          mult.compute_root();
          hw_input.compute_root();
          hw_output.compute_root();

          mult.tile(x, y, xo, yo, xi, yi, 2, 2).accelerate({hw_input}, xi, xo);

          cout << "Loop nest" << endl;
          hw_output.print_loop_nest();
                    
          // hw_output.tile(x,y, xo,yo, xi,yi, 64, 64-2)
          //   .hw_accelerate(xi, xo);

          hw_input.stream_to_accelerator();

        } else {
          mult.compute_root();
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(PointwiseMultiplication, pointwise)
