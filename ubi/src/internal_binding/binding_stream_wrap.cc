#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveStreamWrap(napi_env env, const ResolveOptions& /*options*/) {
  return GetGlobalNamed(env, "__ubi_stream_wrap");
}

}  // namespace internal_binding
