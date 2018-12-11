#ifndef rpc_hpp
#define rpc_hpp

#ifdef CORE_RPC_HPP
#include "../core/rpc.hpp"
#else

#include <stdint.h>

const uint8_t RpcPayloadMax = 0;
const uint8_t RpcNumber = 0;
struct Rpc {
  inline uint8_t Size(uint8_t i) { return 0; }
  inline void VtAct(uint8_t i) { }
};

#endif // CORE_RPC_HPP
#endif
