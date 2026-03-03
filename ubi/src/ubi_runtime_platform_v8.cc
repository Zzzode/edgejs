#include "ubi_runtime_platform.h"

#include "unofficial_napi.h"

napi_status UbiRuntimePlatformDrainTasks(napi_env env) {
  return unofficial_napi_process_microtasks(env);
}
