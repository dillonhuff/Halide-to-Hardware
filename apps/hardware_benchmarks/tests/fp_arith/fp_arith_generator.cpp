#include "Halide.h"

namespace {

using namespace Halide;

class UnitTestFPArith : public Halide::Generator<UnitTestFPArith> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func hw_input("hw_input");
        hw_input(x, y) = cast<bfloat16_t>(input(x, y));

        Func mult, div, add, sub, mod, neg;
        neg(x,y) = -Expr(bfloat16_t(13.3));
        mult(x,y) = hw_input(x,y) * neg(x,y);
        div(x,y) = hw_input(x,y) / Expr(bfloat16_t(4.56));
        mod(x,y) = hw_input(x,y);// % 16;
        add(x,y) = div(x,y) + Expr(bfloat16_t(9.8));
        sub(x,y) = mult(x,y) - add(x,y);

        Func hw_output("hw_output");
        hw_output(x, y) = sub(x, y);
        output(x, y) = cast<uint8_t>(hw_output(x,y));

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;
          
          hw_input.compute_root();
          hw_output.compute_root();
          
          hw_output.tile(x,y, xo,yo, xi,yi, 64, 64)
            .hw_accelerate(xi, xo);

          hw_input.stream_to_accelerator();
          
        } else {  // schedule to CPU
          output.compute_root();
        }
        
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(UnitTestFPArith, fp_arith)
