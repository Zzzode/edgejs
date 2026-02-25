#include "unode_cli.h"

#include <string>

#include "unofficial_napi.h"
#include "unode_runtime.h"

namespace {

constexpr const char kUsage[] = "Usage: unode <script.js>";

}  // namespace

int UnodeRunCliScript(const char* script_path, std::string* error_out) {
  if (error_out != nullptr) {
    error_out->clear();
  }
  if (script_path == nullptr || script_path[0] == '\0') {
    if (error_out != nullptr) {
      *error_out = kUsage;
    }
    return 1;
  }

  napi_env env = nullptr;
  void* env_scope = nullptr;
  const napi_status create_status = unofficial_napi_create_env(8, &env, &env_scope);
  if (create_status != napi_ok || env == nullptr || env_scope == nullptr) {
    if (error_out != nullptr) {
      *error_out = "Failed to initialize runtime environment";
    }
    return 1;
  }

  const int exit_code = UnodeRunScriptFile(env, script_path, error_out);
  const napi_status release_status = unofficial_napi_release_env(env_scope);
  if (release_status != napi_ok) {
    if (error_out != nullptr) {
      *error_out = "Failed to release runtime environment";
    }
    return 1;
  }

  return exit_code;
}

int UnodeRunCli(int argc, const char* const* argv, std::string* error_out) {
  if (error_out != nullptr) {
    error_out->clear();
  }
  if (argv == nullptr || argc != 2) {
    if (error_out != nullptr) {
      *error_out = kUsage;
    }
    return 1;
  }
  return UnodeRunCliScript(argv[1], error_out);
}
