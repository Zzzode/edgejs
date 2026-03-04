#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveProcessWrap(napi_env env, const ResolveOptions& /*options*/) {
  return GetGlobalNamed(env, "__ubi_process_wrap");
}

}  // namespace internal_binding
