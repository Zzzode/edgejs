#include "unode_util.h"

#include <uv.h>

#include <cstdint>

namespace {

static uint32_t GetUVHandleTypeCode(uv_handle_type type) {
  switch (type) {
    case UV_TCP:
      return 0;
    case UV_TTY:
      return 1;
    case UV_UDP:
      return 2;
    case UV_FILE:
      return 3;
    case UV_NAMED_PIPE:
      return 4;
    case UV_UNKNOWN_HANDLE:
      return 5;
    default:
      return 5;
  }
}

napi_value GuessHandleType(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {nullptr};
  if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok ||
      argc < 1) {
    return nullptr;
  }
  int32_t fd = 0;
  if (napi_get_value_int32(env, argv[0], &fd) != napi_ok || fd < 0) {
    return nullptr;
  }
  uv_handle_type t = uv_guess_handle(static_cast<uv_file>(fd));
  uint32_t code = GetUVHandleTypeCode(t);
  napi_value result = nullptr;
  if (napi_create_uint32(env, code, &result) != napi_ok) {
    return nullptr;
  }
  return result;
}

void SetMethod(napi_env env, napi_value obj, const char* name,
               napi_callback cb) {
  napi_value fn = nullptr;
  if (napi_create_function(env, name, NAPI_AUTO_LENGTH, cb, nullptr, &fn) ==
          napi_ok &&
      fn != nullptr) {
    napi_set_named_property(env, obj, name, fn);
  }
}

}  // namespace

void UnodeInstallUtilBinding(napi_env env) {
  napi_value binding = nullptr;
  if (napi_create_object(env, &binding) != napi_ok || binding == nullptr) {
    return;
  }
  SetMethod(env, binding, "guessHandleType", GuessHandleType);

  napi_value global = nullptr;
  if (napi_get_global(env, &global) != napi_ok || global == nullptr) {
    return;
  }
  napi_set_named_property(env, global, "__unode_util", binding);
}
