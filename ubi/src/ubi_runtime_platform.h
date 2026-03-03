#ifndef UBI_RUNTIME_PLATFORM_H_
#define UBI_RUNTIME_PLATFORM_H_

#include "node_api.h"

// Engine-adapter boundary for runtime task draining.
// The current build provides a V8-backed implementation; future engines can
// replace it without changing ubi runtime loop logic.
napi_status UbiRuntimePlatformDrainTasks(napi_env env);

#endif  // UBI_RUNTIME_PLATFORM_H_
