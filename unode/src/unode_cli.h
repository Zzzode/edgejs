#ifndef UNODE_CLI_H_
#define UNODE_CLI_H_

#include <string>

int UnodeRunCli(int argc, const char* const* argv, std::string* error_out);
int UnodeRunCliScript(const char* script_path, std::string* error_out);

#endif  // UNODE_CLI_H_
