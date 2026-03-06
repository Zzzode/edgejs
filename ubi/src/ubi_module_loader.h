#ifndef UBI_MODULE_LOADER_H_
#define UBI_MODULE_LOADER_H_

#include "node_api.h"

napi_status UbiInstallModuleLoader(napi_env env, const char* entry_script_path);

// Store primordials and internalBinding in loader state so they are passed from C++ when calling
// the module wrapper (Node-aligned: fn->Call(context, undefined, argc, argv) with argv from C++).
// Call after the bootstrap prelude so every user module receives the same reference.
void UbiSetPrimordials(napi_env env, napi_value primordials);
void UbiSetInternalBinding(napi_env env, napi_value internal_binding);
void UbiSetPrivateSymbols(napi_env env, napi_value private_symbols);
void UbiSetPerIsolateSymbols(napi_env env, napi_value per_isolate_symbols);
napi_value UbiGetRequireFunction(napi_env env);
napi_value UbiGetInternalBinding(napi_env env);
bool UbiRequireBuiltin(napi_env env, const char* id, napi_value* out);

// Run the internal task_queue tick callback registered via setTickCallback().
// When no callback is registered, `called` is set to false and napi_ok is returned.
napi_status UbiRunTaskQueueTickCallback(napi_env env, bool* called);

// Read the task_queue tickInfo flags populated by internal/process/task_queues.
// Returns false when the binding has not been initialized yet.
bool UbiGetTaskQueueFlags(napi_env env, bool* has_tick_scheduled, bool* has_rejection_to_warn);

#endif  // UBI_MODULE_LOADER_H_
