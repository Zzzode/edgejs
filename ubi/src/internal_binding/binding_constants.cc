#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveConstants(napi_env env, const ResolveOptions& /*options*/) {
  const napi_value undefined = Undefined(env);
  napi_value out = nullptr;
  if (napi_create_object(env, &out) != napi_ok || out == nullptr) return undefined;

  napi_value os_obj = nullptr;
  if (napi_create_object(env, &os_obj) == napi_ok && os_obj != nullptr) {
    napi_value signals = nullptr;
    if (napi_create_object(env, &signals) == napi_ok && signals != nullptr) {
      napi_set_named_property(env, os_obj, "signals", signals);
    }
    napi_set_named_property(env, out, "os", os_obj);
  }

  napi_value fs_obj = nullptr;
  if (napi_create_object(env, &fs_obj) == napi_ok && fs_obj != nullptr) {
    SetInt32(env, fs_obj, "F_OK", 0);
    SetInt32(env, fs_obj, "R_OK", 4);
    SetInt32(env, fs_obj, "W_OK", 2);
    SetInt32(env, fs_obj, "X_OK", 1);
    napi_set_named_property(env, out, "fs", fs_obj);
  }

  const napi_value os_constants = GetGlobalNamed(env, "__ubi_os_constants");
  if (!IsUndefined(env, os_constants)) {
    napi_set_named_property(env, out, "os", os_constants);
  }

  const napi_value fs_binding = GetGlobalNamed(env, "__ubi_fs");
  if (!IsUndefined(env, fs_binding)) {
    napi_value fs_constants_obj = nullptr;
    if (napi_create_object(env, &fs_constants_obj) == napi_ok && fs_constants_obj != nullptr) {
      napi_value keys = nullptr;
      if (napi_get_property_names(env, fs_binding, &keys) == napi_ok && keys != nullptr) {
        uint32_t key_count = 0;
        if (napi_get_array_length(env, keys, &key_count) == napi_ok) {
          for (uint32_t i = 0; i < key_count; i++) {
            napi_value key = nullptr;
            if (napi_get_element(env, keys, i, &key) != napi_ok || key == nullptr) continue;
            napi_value value = nullptr;
            if (napi_get_property(env, fs_binding, key, &value) != napi_ok || value == nullptr) continue;
            napi_valuetype t = napi_undefined;
            if (napi_typeof(env, value, &t) != napi_ok) continue;
            if (t == napi_number) {
              napi_set_property(env, fs_constants_obj, key, value);
            }
          }
        }
      }
      napi_set_named_property(env, out, "fs", fs_constants_obj);
    }
  }

  return out;
}

}  // namespace internal_binding
