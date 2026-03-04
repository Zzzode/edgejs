#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveUrlPattern(napi_env env, const ResolveOptions& /*options*/) {
  napi_value out = nullptr;
  if (napi_create_object(env, &out) != napi_ok || out == nullptr) return Undefined(env);

  napi_value global = GetGlobal(env);
  if (global != nullptr) {
    napi_value ctor = nullptr;
    if (napi_get_named_property(env, global, "URLPattern", &ctor) == napi_ok && ctor != nullptr) {
      napi_set_named_property(env, out, "URLPattern", ctor);
    }
  }
  return out;
}

}  // namespace internal_binding
