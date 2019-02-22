#include <cassert>
#include <iostream>

#include "bin/vhls_target.h"

using namespace std;
using namespace hls;

int main() {
  stream<AxiPackedStencil<uint16_t, 1, 1> > arg_0;

  AxiPackedStencil<uint16_t, 1, 1> st;
  
  stream<AxiPackedStencil<uint16_t, 1, 1> > arg_1;
  st(0, 0) = 15;
  assert(st(0, 0) == 15);
  
  arg_1.write(st);

  auto& res = arg_1.read();

  assert(res(0, 0) == 15);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      AxiPackedStencil<uint16_t, 1, 1> st;
      st(0, 0) = 10;
      cout << "st(0, 0) before writing " << st(0, 0) << endl;
      arg_0.write(st);
    }
  }

  vhls_target(arg_0, arg_1);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      auto st = arg_1.read();
      cout << "st(0, 0) = " << st(0, 0) << endl;
      assert(st(0, 0) == 10*2);
    }
  }
  
}
