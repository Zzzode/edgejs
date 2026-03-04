#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveConfig(napi_env env, const ResolveOptions& /*options*/) {
  napi_value out = nullptr;
  if (napi_create_object(env, &out) != napi_ok || out == nullptr) return Undefined(env);

  // Keep current ubi defaults for now.
  SetBool(env, out, "hasIntl", false);
  SetBool(env, out, "hasSmallICU", false);
  SetBool(env, out, "hasInspector", false);
  SetBool(env, out, "hasTracing", false);
  SetBool(env, out, "hasOpenSSL", true);
  SetBool(env, out, "openSSLIsBoringSSL", false);
  SetBool(env, out, "fipsMode", false);
  SetBool(env, out, "hasNodeOptions", true);
  SetBool(env, out, "noBrowserGlobals", false);
  SetBool(env, out, "isDebugBuild", false);
  return out;
}

}  // namespace internal_binding
