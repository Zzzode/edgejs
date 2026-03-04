#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveSpawnSync(napi_env env, const ResolveOptions& /*options*/) {
  return GetGlobalNamed(env, "__ubi_spawn_sync");
}

}  // namespace internal_binding
