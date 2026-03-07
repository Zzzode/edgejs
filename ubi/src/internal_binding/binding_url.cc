#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"
#include "ubi_url.h"

namespace internal_binding {

napi_value ResolveUrl(napi_env env, const ResolveOptions& /*options*/) {
  napi_value out = UbiInstallUrlBinding(env);
  return out != nullptr ? out : Undefined(env);
}

}  // namespace internal_binding
