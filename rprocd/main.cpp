#include "net.h"

#include "lib/async-loop/cpp/loop.h"

#include <string>
#include <sstream>

int main() {
  std::stringstream ether_name;
  ether_name << "rproc0";
  RprocNet vnet(ether_name.str());

  async::Loop loop;
  loop.Run();
  return 0;
}
