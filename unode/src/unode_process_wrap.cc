#include "unode_process_wrap.h"

#include <cstdint>

#include <uv.h>

namespace {

struct ProcessWrap {
  napi_ref wrapper_ref = nullptr;
  int32_t pid = 0;
  bool alive = false;
};

int32_t g_next_pid = 40000;

ProcessWrap* UnwrapProcess(napi_env env, napi_callback_info info, napi_value* self_out) {
  size_t argc = 0;
  napi_value self = nullptr;
  if (napi_get_cb_info(env, info, &argc, nullptr, &self, nullptr) != napi_ok || self == nullptr) return nullptr;
  ProcessWrap* wrap = nullptr;
  if (napi_unwrap(env, self, reinterpret_cast<void**>(&wrap)) != napi_ok || wrap == nullptr) return nullptr;
  if (self_out != nullptr) *self_out = self;
  return wrap;
}

napi_value MakeInt32(napi_env env, int32_t value) {
  napi_value out = nullptr;
  napi_create_int32(env, value, &out);
  return out;
}

void SetPidProperty(napi_env env, napi_value self, int32_t pid) {
  napi_value pid_value = MakeInt32(env, pid);
  if (pid_value != nullptr) napi_set_named_property(env, self, "pid", pid_value);
}

void ProcessFinalize(napi_env env, void* data, void* /*hint*/) {
  auto* wrap = static_cast<ProcessWrap*>(data);
  if (wrap == nullptr) return;
  if (wrap->wrapper_ref != nullptr) napi_delete_reference(env, wrap->wrapper_ref);
  delete wrap;
}

napi_value ProcessCtor(napi_env env, napi_callback_info info) {
  size_t argc = 0;
  napi_value self = nullptr;
  if (napi_get_cb_info(env, info, &argc, nullptr, &self, nullptr) != napi_ok || self == nullptr) {
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
  }
  auto* wrap = new ProcessWrap();
  napi_wrap(env, self, wrap, ProcessFinalize, nullptr, &wrap->wrapper_ref);
  SetPidProperty(env, self, 0);
  return self;
}

napi_value ProcessSpawn(napi_env env, napi_callback_info info) {
  napi_value self = nullptr;
  ProcessWrap* wrap = UnwrapProcess(env, info, &self);
  if (wrap == nullptr || self == nullptr) return MakeInt32(env, UV_EINVAL);
  if (wrap->alive) return MakeInt32(env, UV_EINVAL);

  wrap->pid = ++g_next_pid;
  wrap->alive = true;
  SetPidProperty(env, self, wrap->pid);
  return MakeInt32(env, 0);
}

napi_value ProcessKill(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {nullptr};
  napi_value self = nullptr;
  if (napi_get_cb_info(env, info, &argc, argv, &self, nullptr) != napi_ok || self == nullptr) {
    return MakeInt32(env, UV_EINVAL);
  }

  ProcessWrap* wrap = nullptr;
  if (napi_unwrap(env, self, reinterpret_cast<void**>(&wrap)) != napi_ok || wrap == nullptr) {
    return MakeInt32(env, UV_EINVAL);
  }
  if (!wrap->alive) return MakeInt32(env, UV_ESRCH);

  int32_t signal = 0;
  bool has_signal = false;
  if (argc >= 1 && argv[0] != nullptr) {
    napi_valuetype t = napi_undefined;
    if (napi_typeof(env, argv[0], &t) == napi_ok && t == napi_number &&
        napi_get_value_int32(env, argv[0], &signal) == napi_ok) {
      has_signal = true;
    }
  }
  if (has_signal && signal == 0) return MakeInt32(env, 0);

  wrap->alive = false;

  napi_value onexit = nullptr;
  bool has_onexit = false;
  if (napi_has_named_property(env, self, "onexit", &has_onexit) == napi_ok && has_onexit) {
    napi_get_named_property(env, self, "onexit", &onexit);
  }
  napi_valuetype fn_type = napi_undefined;
  if (onexit != nullptr && napi_typeof(env, onexit, &fn_type) == napi_ok && fn_type == napi_function) {
    napi_value process_obj = nullptr;
    napi_value next_tick = nullptr;
    napi_value global = nullptr;
    if (napi_get_global(env, &global) == napi_ok &&
        global != nullptr &&
        napi_get_named_property(env, global, "process", &process_obj) == napi_ok &&
        process_obj != nullptr &&
        napi_get_named_property(env, process_obj, "nextTick", &next_tick) == napi_ok) {
      napi_valuetype next_type = napi_undefined;
      if (next_tick != nullptr && napi_typeof(env, next_tick, &next_type) == napi_ok && next_type == napi_function) {
        napi_value exit_code = MakeInt32(env, 0);
        napi_value signal_code = nullptr;
        if (argc >= 1 && argv[0] != nullptr) {
          signal_code = argv[0];
        } else {
          napi_get_null(env, &signal_code);
        }
        napi_value tick_args[3] = {onexit, exit_code, signal_code};
        napi_value ignored = nullptr;
        napi_call_function(env, process_obj, next_tick, 3, tick_args, &ignored);
      }
    }
  }

  return MakeInt32(env, 0);
}

napi_value ProcessClose(napi_env env, napi_callback_info info) {
  ProcessWrap* wrap = UnwrapProcess(env, info, nullptr);
  if (wrap != nullptr) wrap->alive = false;
  napi_value undefined = nullptr;
  napi_get_undefined(env, &undefined);
  return undefined;
}

napi_value ProcessRef(napi_env env, napi_callback_info /*info*/) {
  napi_value undefined = nullptr;
  napi_get_undefined(env, &undefined);
  return undefined;
}

napi_value ProcessUnref(napi_env env, napi_callback_info /*info*/) {
  napi_value undefined = nullptr;
  napi_get_undefined(env, &undefined);
  return undefined;
}

}  // namespace

void UnodeInstallProcessWrapBinding(napi_env env) {
  if (env == nullptr) return;
  napi_property_descriptor methods[] = {
      {"spawn", nullptr, ProcessSpawn, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"kill", nullptr, ProcessKill, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"close", nullptr, ProcessClose, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"ref", nullptr, ProcessRef, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"unref", nullptr, ProcessUnref, nullptr, nullptr, nullptr, napi_default, nullptr},
  };

  napi_value process_ctor = nullptr;
  if (napi_define_class(env,
                        "Process",
                        NAPI_AUTO_LENGTH,
                        ProcessCtor,
                        nullptr,
                        sizeof(methods) / sizeof(methods[0]),
                        methods,
                        &process_ctor) != napi_ok ||
      process_ctor == nullptr) {
    return;
  }

  napi_value binding = nullptr;
  if (napi_create_object(env, &binding) != napi_ok || binding == nullptr) return;
  napi_set_named_property(env, binding, "Process", process_ctor);

  napi_value global = nullptr;
  if (napi_get_global(env, &global) != napi_ok || global == nullptr) return;
  napi_set_named_property(env, global, "__unode_process_wrap", binding);
}
