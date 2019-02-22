#pragma once

namespace hls {
  template<typename T>
  class stream {
    T elems[1000];
    int read_addr;
    int write_addr;
    
  public:

    stream() {
      read_addr = 0;
      write_addr = 0;
    }

    T& read() {
      T& res = elems[read_addr];
      read_addr = read_addr + 1;
      return res;
    }

    void write(const T& val) {
      elems[write_addr] = val;
      write_addr = write_addr + 1;
    }
  };

}
