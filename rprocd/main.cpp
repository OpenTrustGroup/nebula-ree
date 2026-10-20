#include "net.h"

#include "lib/async-loop/cpp/loop.h"

#include <string>
#include <sstream>

int main() {
  RprocNet vnet0("rproc0", "192.168.1.1", 0);
  RprocNet vnet1("rproc1", "192.168.0.1", 1);

  async::Loop loop;
  loop.Run();
  return 0;
}
