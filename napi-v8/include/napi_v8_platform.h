#ifndef NAPI_V8_PLATFORM_H_
#define NAPI_V8_PLATFORM_H_

#include <v8.h>

#include "js_native_api.h"

#ifdef __cplusplus
extern "C" {
#endif

NAPI_EXTERN napi_status napi_v8_create_env(v8::Local<v8::Context> context,
                                           int32_t module_api_version,
                                           napi_env* result);

NAPI_EXTERN napi_status napi_v8_destroy_env(napi_env env);

NAPI_EXTERN napi_status napi_v8_wrap_existing_value(napi_env env,
                                                    v8::Local<v8::Value> value,
                                                    napi_value* result);

// Unofficial/test-only helper. Requests a full GC cycle for testing.
NAPI_EXTERN napi_status unofficial_napi_request_gc_for_testing(napi_env env);

#ifdef __cplusplus
}
#endif

#endif  // NAPI_V8_PLATFORM_H_
