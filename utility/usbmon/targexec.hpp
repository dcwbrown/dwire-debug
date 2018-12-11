#ifndef targexec_hpp
#define targexec_hpp

#include <stdint.h>

class TargExec {
public:
  TargExec(const char* file);

  struct Data {
    uint8_t* blob = nullptr;
    uint16_t load_address;
    uint16_t load_length;
    uint16_t buffer_size;
  } data;
};

#endif
