#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

namespace {

napi_value NoopCallback(napi_env env, napi_callback_info /*info*/) {
  return Undefined(env);
}

}  // namespace

napi_value ResolveAsyncWrap(napi_env env, const ResolveOptions& /*options*/) {
  napi_value out = nullptr;
  if (napi_create_object(env, &out) != napi_ok || out == nullptr) return Undefined(env);

  napi_value setup_hooks = nullptr;
  if (napi_create_function(env, "setupHooks", NAPI_AUTO_LENGTH, NoopCallback, nullptr, &setup_hooks) == napi_ok &&
      setup_hooks != nullptr) {
    napi_set_named_property(env, out, "setupHooks", setup_hooks);
  }
  return out;
}

}  // namespace internal_binding
