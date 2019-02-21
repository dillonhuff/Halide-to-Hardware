#include "Halide.h"
#include <stdio.h>

using namespace Halide;

const unsigned char gaussian2d[5][5] = {
    {1,     3,     6,     3,     1},
    {3,    15,    25,    15,     3},
    {6,    25,    44,    25,     6},
    {3,    15,    25,    15,     3},
    {1,     3,     6,     3,     1}
};

Var x("x"), y("y"), c("c");
Var xo("xo"), xi("xi"), yi("yi"), yo("yo");

class MyPipeline {
public:
    ImageParam input;
    ImageParam weight;
    Param<uint16_t> bias;
    Func kernel;
    Func clamped;
    Func conv1;
    Func output;
    Func hw_output;
    std::vector<Argument> args;

    RDom win;

    MyPipeline() : input(UInt(8), 3, "input"), weight(UInt(8), 2, "weight"), bias("bias"),
                   kernel("kernel"), conv1("conv1"),
                   output("output"), hw_output("hw_output"),
                   win(-2, 5, -2, 5) {
        // Define a 9x9 Gaussian Blur with a repeat-edge boundary condition.
        float sigma = 1.5f;

        kernel(x, y) = cast<uint16_t>(exp(-(x*x + y*y)/(2*sigma*sigma)) / (float)(2*M_PI*sigma*sigma));

        // define the algorithm
        clamped = BoundaryConditions::repeat_edge(input);
        //conv1 = clamped;
        conv1(x, y, c) += clamped(x+win.x, y+win.y, c) * kernel(win.x, win.y);

        // unroll the reduction
        conv1.update(0).unroll(c).unroll(win.x).unroll(win.y);

        hw_output = convolve55_rd(conv1);
        output(x, y, c) = hw_output(x, y, c);

        // constraints
        output.bound(c, 0, 3);

        // weight.set_bounds(0, 0, 5);
        // weight.set_bounds(1, 0, 5);
        // weight.set_stride(0, 1);
        // weight.set_stride(1, 5);

        args.push_back(input);
        args.push_back(weight);
        args.push_back(bias);

        kernel.compute_root();
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        output.tile(x, y, xo, yo, xi, yi, 256, 256);
        output.fuse(xo, yo, xo).parallel(xo);

        output.vectorize(xi, 8);
        conv1.compute_at(output, xo).vectorize(x, 8);

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    // void compile_gpu() {
    //     std::cout << "\ncompiling gpu code..." << std::endl;

    //     output.compute_root().reorder(x, y, c).gpu_tile(x, y, c, 16, 16, 1);
    //     conv1.compute_root().reorder(x, y, c).gpu_tile(x, y, c, 16, 16, 1);
    //     //conv1.compute_at(output, Var::gpu_blocks()).gpu_threads(x, y, c);
    //     //output.print_loop_nest();

    //     Target target = get_target_from_environment();
    //     target.set_feature(Target::CUDA);
    //     output.compile_to_lowered_stmt("pipeline_cuda.ir.html", args, HTML, target);
    //     output.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
    //     output.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);
    // }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        clamped.compute_root(); // prepare the input for the whole image

        // HLS schedule: make a hw pipeline producing 'hw_output', taking
        // inputs of 'clamped', buffering intermediates at (output, xo) loop
        // level
        hw_output.compute_root();
        //hw_output.tile(x, y, xo, yo, xi, yi, 1920, 1080).reorder(c, xi, yi, xo, yo);
        hw_output.tile(x, y, xo, yo, xi, yi, 256, 256).reorder(c, xi, yi, xo, yo);
        //hw_output.unroll(xi, 2);
        hw_output.accelerate({clamped}, xi, xo, {kernel});  // define the inputs and the output
        conv1.linebuffer();
	//        conv1.unroll(c).unroll(x).unroll(y);
        hw_output.unroll(c);

        //output.print_loop_nest();
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_vhls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);
    }

private:
    Func convolve55_rd(Func in) {
        Func local_sum, res;
        RDom r(-2, 5, -2, 5);

        local_sum(x, y, c) = bias;

        local_sum(x, y, c) += cast<uint16_t>(in(x+r.x, y+r.y, c)) * weight(r.x+2, r.y+2);
        res(x, y, c) = cast<uint8_t>(local_sum(x, y, c) >> 8);

        // unroll the reduction
        local_sum.update(0).unroll(r.x).unroll(r.y);
        return res;
    }
};


// Var x("x"), y("y"), z("z"), c("c");
// Var x_grid("x_grid"), y_grid("y_grid"), xo("xo"), yo("yo"), x_in("x_in"), y_in("y_in");
// uint8_t r_sigma = 32;
// int s_sigma = 8;

// class MyPipelineOpt {
// public:
//     ImageParam input;
//     RDom r;
//     Func clamped;
//     Func input_shuffled, input2_shuffled, output_shuffled;
//     Func histogram, downsampled;
//     Func blurx, blury, blurz;
//     Func output;
//     std::vector<Argument> args;

//     MyPipelineOpt()
//         : input(UInt(8), 2), r(0, s_sigma, 0, s_sigma),
//           input_shuffled("input_shuffled"), input2_shuffled("input2_shuffled"),
//           output_shuffled("output_shuffled"),
//           histogram("histogram"), downsampled("downsampled"),
//           blurx("blurx"), blury("blury"), blurz("blurz"),
//           output("output")
//     {
//         // Add a boundary condition
//         //clamped = BoundaryConditions::repeat_edge(input);
//         clamped(x, y) = input(x+40, y+40);

//         // shuffle the input
//         input_shuffled(x_in, y_in, x_grid, y_grid)
//             = clamped(x_grid*s_sigma + x_in - s_sigma/2, y_grid*s_sigma + y_in - s_sigma/2);

//         // Construct the bilateral grid
//         Expr val = input_shuffled(r.x, r.y, x, y);
//         val = clamp(val, 0, 255);
//         Expr zi = cast<int>((val + r_sigma/2) / r_sigma);
//         histogram(x, y, z, c) = cast<uint16_t>(0);
//         histogram(x, y, zi, c) += select(c == 0, val/16, 64/16);

//         // Blur the grid using a five-tap filter
//         blurz(x, y, z, c) = cast<uint16_t>(histogram(x, y, z-2, c) +
//                              histogram(x, y, z-1, c)*4 +
//                              histogram(x, y, z  , c)*6 +
//                              histogram(x, y, z+1, c)*4 +
//                              histogram(x, y, z+2, c)) / 16;
//         blurx(x, y, z, c) = cast<uint16_t>(blurz(x-2, y, z, c) +
//                              blurz(x-1, y, z, c)*4 +
//                              blurz(x  , y, z, c)*6 +
//                              blurz(x+1, y, z, c)*4 +
//                              blurz(x+2, y, z, c)) / 16;
//         blury(x, y, z, c) = cast<uint16_t>(blurx(x, y-2, z, c) +
//                              blurx(x, y-1, z, c)*4 +
//                              blurx(x, y  , z, c)*6 +
//                              blurx(x, y+1, z, c)*4 +
//                              blurx(x, y+2, z, c)) / 16;


//         // Take trilinear samples to compute the output

//         // shuffle the input
//         input2_shuffled(x_in, y_in, x_grid, y_grid)
//             = input(x_grid*s_sigma + x_in, y_grid*s_sigma + y_in);

//         Expr sample_val = cast<uint16_t>(input2_shuffled(x_in, y_in, x_grid, y_grid));
//         zi = cast<int>(sample_val / r_sigma);
//         Expr zf = cast<uint16_t>((sample_val % r_sigma) * (65536 / r_sigma));
//         Expr xf = cast<uint16_t>(x_in * (65536 / s_sigma));
//         Expr yf = cast<uint16_t>(y_in * (65536 / s_sigma));
//         Expr xi = x_grid;
//         Expr yi = y_grid;
//         Expr value, weight;
//         value =
//             lerp(lerp(lerp(blury(xi, yi, zi, 0), blury(xi+1, yi, zi, 0), xf),
//                       lerp(blury(xi, yi+1, zi, 0), blury(xi+1, yi+1, zi, 0), xf), yf),
//                  lerp(lerp(blury(xi, yi, zi+1, 0), blury(xi+1, yi, zi+1, 0), xf),
//                       lerp(blury(xi, yi+1, zi+1, 0), blury(xi+1, yi+1, zi+1, 0), xf), yf), zf);
//         weight =
//             lerp(lerp(lerp(blury(xi, yi, zi, 1), blury(xi+1, yi, zi, 1), xf),
//                       lerp(blury(xi, yi+1, zi, 1), blury(xi+1, yi+1, zi, 1), xf), yf),
//                  lerp(lerp(blury(xi, yi, zi+1, 1), blury(xi+1, yi, zi+1, 1), xf),
//                       lerp(blury(xi, yi+1, zi+1, 1), blury(xi+1, yi+1, zi+1, 1), xf), yf), zf);
//         weight = max(weight, 1); // to avoid underflow

//         // Normalize
//         output_shuffled(x_in, y_in, x_grid, y_grid) = cast<uint8_t>(clamp(value * 64 / weight, 0, 255));

//         // shuffle the output
//         output(x, y) = output_shuffled(x%s_sigma, y%s_sigma, x/s_sigma, y/s_sigma);

//         // The comment constraints and schedules.
//         Expr out_width = output.output_buffer().width();
//         Expr out_height = output.output_buffer().height();
//         output
//             .bound(x, 0, (out_width/480)*480)
//             .bound(y, 0, (out_height/640)*640);

//         // Arguments
//         args = {input};
//     }


//     void compile_hls() {
//         std::cout << "\ncompiling HLS code..." << std::endl;
//         output.tile(x, y, xo, yo, x_in, y_in, 480, 640);

//         output_shuffled.compute_at(output, xo);
//         input_shuffled.compute_at(output, xo);
//         input2_shuffled.compute_at(output, xo);

//         output_shuffled.tile(x_grid, y_grid, xo, yo, x_grid, y_grid, 60, 80);
//         output_shuffled.accelerate({input_shuffled, input2_shuffled}, x_grid, xo);

//         blury.linebuffer().reorder(x, y, z, c);
//         blurx.linebuffer().reorder(x, y, z, c);
//         blurz.linebuffer().reorder(z, x, y, c);
//         histogram.linebuffer().reorder(c, z, x, y).unroll(c).unroll(z);
//         histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);

//         //output.print_loop_nest();

//         // Create the target for HLS simulation
//         Target hls_target = get_target_from_environment();
//         hls_target.set_feature(Target::CPlusPlusMangling);
//         output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
//         output.compile_to_vhls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
//         output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);

//         // Create the Zynq platform target
//         // std::vector<Target::Feature> features({Target::Zynq});
//         // Target target(Target::Linux, Target::ARM, 32, features);

//         // output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
//         // output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);

//         // Vectorization and Parallelization Schedules (only work with LLVM codegen)
//         // input_shuffled.vectorize(x_in, 8);
//         // input2_shuffled.vectorize(x_in, 8);
//         // output.vectorize(x_in, 8);

//         // output.fuse(xo, yo, xo).parallel(xo);

//         // output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
//         // output.compile_to_object("pipeline_zynq.o", args, "pipeline_zynq", target);
//         // output.compile_to_assembly("pipeline_zynq.s", args, "pipeline_zynq", target);
//     }
// };

int main(int argc, char **argv) {
    MyPipeline p2;
    p2.compile_hls();
    return 0;
}
