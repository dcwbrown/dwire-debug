#ifdef CORE_RPC_HPP

#include "command.hpp"
#include "rpc.hpp"

#include <sstream>

void RpcConsole::Act() {
  if (!radix) {
    SysConWrite(text);
    return;
  }

  std::ostringstream out;

  if ((radix & 0x7f) == 16)
    out << std::hex;
  else if ((radix & 0x7f) == 8)
    out << std::oct;

  if (radix & 0x80)
    out << (int32_t)value;
  else
    out << value;

  SysConWrite(out.str().c_str());
}

#endif
