#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveTimers(napi_env env, const ResolveOptions& /*options*/) {
  napi_value global = GetGlobal(env);
  if (global == nullptr) return Undefined(env);

  napi_value timers_binding = nullptr;
  if (napi_get_named_property(env, global, "__ubi_timers_binding_js", &timers_binding) == napi_ok &&
      timers_binding != nullptr) {
    return timers_binding;
  }
  if (napi_get_named_property(env, global, "__ubi_timers_binding", &timers_binding) == napi_ok &&
      timers_binding != nullptr) {
    return timers_binding;
  }
  return Undefined(env);
}

}  // namespace internal_binding
