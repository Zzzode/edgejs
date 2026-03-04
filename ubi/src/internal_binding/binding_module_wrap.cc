#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

namespace {

napi_value ReturnFirstArg(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {nullptr};
  if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok || argc < 1 || argv[0] == nullptr) {
    return Undefined(env);
  }
  return argv[0];
}

}  // namespace

napi_value ResolveModuleWrap(napi_env env, const ResolveOptions& /*options*/) {
  napi_value out = nullptr;
  if (napi_create_object(env, &out) != napi_ok || out == nullptr) return Undefined(env);

  SetInt32(env, out, "kSourcePhase", 0);
  SetInt32(env, out, "kEvaluationPhase", 1);
  SetInt32(env, out, "kEvaluated", 2);

  napi_value create_required_module_facade = nullptr;
  if (napi_create_function(env,
                           "createRequiredModuleFacade",
                           NAPI_AUTO_LENGTH,
                           ReturnFirstArg,
                           nullptr,
                           &create_required_module_facade) == napi_ok &&
      create_required_module_facade != nullptr) {
    napi_set_named_property(env, out, "createRequiredModuleFacade", create_required_module_facade);
  }

  return out;
}

}  // namespace internal_binding
