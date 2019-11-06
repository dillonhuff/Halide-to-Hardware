#include "offset_soc_mini.h"

#include <iostream>

#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace std;

#include "test_utils.h"

// g++ lesson_10*run.cpp lesson_10_halide.a -o lesson_10_run -I ../include
int main() {
  Halide::Runtime::Buffer<uint16_t> input(15, 15), output(8, 8);
  for (int y = 0; y < input.height(); y++) {
    for (int x = 0; x < input.width(); x++) {
      input(x, y) = y*input.width() + x;
      //input(x, y) = 200;
      //y*input.width() + x;
    }
  }

  hw_output(input, output);

  cout << "C++ Output..." << endl;
  printBuffer(output, cout);
  cout << "Done" << endl;

  Halide::Tools::save_image(output, "offset.pgm");
  return 0;
}