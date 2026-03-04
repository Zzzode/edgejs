#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveEncodingBinding(napi_env env, const ResolveOptions& /*options*/) {
  return GetGlobalNamed(env, "__ubi_encoding");
}

}  // namespace internal_binding
