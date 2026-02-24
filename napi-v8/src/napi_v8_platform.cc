#include "napi_v8_platform.h"

#include <new>

#include "internal/napi_v8_env.h"

extern "C" {

napi_status NAPI_CDECL napi_v8_create_env(v8::Local<v8::Context> context,
                                          int32_t module_api_version,
                                          napi_env* result) {
  if (result == nullptr || context.IsEmpty()) {
    return napi_invalid_arg;
  }
  auto* env = new (std::nothrow) napi_env__(context, module_api_version);
  if (env == nullptr) {
    return napi_generic_failure;
  }
  *result = env;
  return napi_ok;
}

napi_status NAPI_CDECL napi_v8_destroy_env(napi_env env) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  delete env;
  return napi_ok;
}

napi_status NAPI_CDECL napi_v8_wrap_existing_value(napi_env env,
                                                   v8::Local<v8::Value> value,
                                                   napi_value* result) {
  if (env == nullptr || value.IsEmpty() || result == nullptr) {
    return napi_invalid_arg;
  }
  *result = napi_v8_wrap_value(env, value);
  return (*result == nullptr) ? napi_generic_failure : napi_ok;
}

}  // extern "C"
