#ifndef JS_NATIVE_API_ENTRY_POINT_H_
#define JS_NATIVE_API_ENTRY_POINT_H_

#include <node_api.h>

#ifdef __cplusplus
extern "C" {
#endif

napi_value Init(napi_env env, napi_value exports);

#ifdef __cplusplus
}
#endif

#endif  // JS_NATIVE_API_ENTRY_POINT_H_
