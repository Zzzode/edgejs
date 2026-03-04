#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveStringDecoder(napi_env env, const ResolveOptions& /*options*/) {
  return GetGlobalNamed(env, "__ubi_string_decoder");
}

}  // namespace internal_binding
