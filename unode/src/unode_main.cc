#include <iostream>
#include <string>

#include "unode_cli.h"

int main(int argc, char** argv) {
  std::string error;
  const int exit_code = UnodeRunCli(argc, argv, &error);
  if (!error.empty()) {
    std::cerr << error << "\n";
  }
  return exit_code;
}
