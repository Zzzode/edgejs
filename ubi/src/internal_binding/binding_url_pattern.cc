#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"
#include "ubi_url_pattern.h"

namespace internal_binding {

napi_value ResolveUrlPattern(napi_env env, const ResolveOptions& /*options*/) {
  napi_value out = UbiInstallUrlPatternBinding(env);
  return out != nullptr ? out : Undefined(env);
}

}  // namespace internal_binding
