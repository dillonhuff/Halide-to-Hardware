#pragma once

namespace hls {
  template<typename T>
  class stream {
    T elems[1000];
    
  public:

    T read() {
      return elems[0];
    }

    void write(const T& val) {
      elems[0] = val;
    }
  };

}
