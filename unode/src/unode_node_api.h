#ifndef UNODE_NODE_API_H_
#define UNODE_NODE_API_H_

#include "js_native_api.h"

napi_status UnodeInstallConsole(napi_env env);
napi_status UnodeInstallUnofficialNapiTestingUntilGc(napi_env env, napi_value target);
napi_status UnodeInstallForceGc(napi_env env);
napi_status UnodePerformMicrotaskCheckpoint(napi_env env);
napi_status UnodeForceGc(napi_env env);

#endif  // UNODE_NODE_API_H_
