#include "internal/napi_v8_env.h"
#include "js_native_api.h"

extern "C" napi_status NAPI_CDECL unofficial_napi_request_gc_for_testing(napi_env env) {
  if (env == nullptr || env->isolate == nullptr) {
    return napi_invalid_arg;
  }
  env->isolate->RequestGarbageCollectionForTesting(
      v8::Isolate::GarbageCollectionType::kFullGarbageCollection);
  env->isolate->PerformMicrotaskCheckpoint();
  return napi_ok;
}
