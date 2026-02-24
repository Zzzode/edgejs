#ifndef NAPI_V8_ENV_H_
#define NAPI_V8_ENV_H_

#include <memory>
#include <string>
#include <vector>

#include <v8.h>

#include "js_native_api.h"

struct napi_value__ {
  explicit napi_value__(napi_env env, v8::Local<v8::Value> local);
  ~napi_value__();

  v8::Local<v8::Value> local() const;

  napi_env env;
  v8::Global<v8::Value> value;
};

struct napi_callback_info__ {
  napi_env env = nullptr;
  void* data = nullptr;
  napi_value this_arg = nullptr;
  std::vector<napi_value> args;
};

struct napi_env__ {
  explicit napi_env__(v8::Local<v8::Context> context, int32_t module_api_version);
  ~napi_env__();

  v8::Local<v8::Context> context() const;

  v8::Isolate* isolate = nullptr;
  v8::Global<v8::Context> context_ref;
  napi_extended_error_info last_error{};
  std::string last_error_message;
  v8::Global<v8::Value> last_exception;
  int32_t module_api_version = 8;
};

napi_status napi_v8_set_last_error(napi_env env,
                                   napi_status status,
                                   const char* message);

napi_status napi_v8_clear_last_error(napi_env env);

napi_value napi_v8_wrap_value(napi_env env, v8::Local<v8::Value> value);
v8::Local<v8::Value> napi_v8_unwrap_value(napi_value value);

#endif  // NAPI_V8_ENV_H_
