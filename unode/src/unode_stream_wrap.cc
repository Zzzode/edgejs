#include "unode_stream_wrap.h"

#include <cstdint>

namespace {

int32_t* g_stream_state = nullptr;

struct EmptyWrap {
  napi_ref wrapper_ref = nullptr;
};

void EmptyWrapFinalize(napi_env env, void* data, void* /*hint*/) {
  auto* wrap = static_cast<EmptyWrap*>(data);
  if (wrap->wrapper_ref != nullptr) napi_delete_reference(env, wrap->wrapper_ref);
  delete wrap;
}

napi_value EmptyCtor(napi_env env, napi_callback_info info) {
  napi_value self = nullptr;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &self, nullptr);
  auto* wrap = new EmptyWrap();
  napi_wrap(env, self, wrap, EmptyWrapFinalize, nullptr, &wrap->wrapper_ref);
  return self;
}

void SetNamedU32(napi_env env, napi_value obj, const char* key, uint32_t value) {
  napi_value v = nullptr;
  if (napi_create_uint32(env, value, &v) == napi_ok && v != nullptr) {
    napi_set_named_property(env, obj, key, v);
  }
}

}  // namespace

int32_t* UnodeGetStreamBaseState() {
  return g_stream_state;
}

void UnodeInstallStreamWrapBinding(napi_env env) {
  napi_value binding = nullptr;
  if (napi_create_object(env, &binding) != napi_ok || binding == nullptr) return;

  napi_value write_wrap_ctor = nullptr;
  if (napi_define_class(env,
                        "WriteWrap",
                        NAPI_AUTO_LENGTH,
                        EmptyCtor,
                        nullptr,
                        0,
                        nullptr,
                        &write_wrap_ctor) != napi_ok ||
      write_wrap_ctor == nullptr) {
    return;
  }

  napi_value shutdown_wrap_ctor = nullptr;
  if (napi_define_class(env,
                        "ShutdownWrap",
                        NAPI_AUTO_LENGTH,
                        EmptyCtor,
                        nullptr,
                        0,
                        nullptr,
                        &shutdown_wrap_ctor) != napi_ok ||
      shutdown_wrap_ctor == nullptr) {
    return;
  }

  void* state_data = nullptr;
  napi_value state_ab = nullptr;
  if (napi_create_arraybuffer(env, sizeof(int32_t) * kUnodeStreamStateLength, &state_data, &state_ab) != napi_ok ||
      state_ab == nullptr || state_data == nullptr) {
    return;
  }
  g_stream_state = static_cast<int32_t*>(state_data);
  for (int i = 0; i < kUnodeStreamStateLength; i++) g_stream_state[i] = 0;

  napi_value stream_state = nullptr;
  if (napi_create_typedarray(env,
                             napi_int32_array,
                             kUnodeStreamStateLength,
                             state_ab,
                             0,
                             &stream_state) != napi_ok ||
      stream_state == nullptr) {
    return;
  }

  napi_set_named_property(env, binding, "WriteWrap", write_wrap_ctor);
  napi_set_named_property(env, binding, "ShutdownWrap", shutdown_wrap_ctor);
  napi_set_named_property(env, binding, "streamBaseState", stream_state);

  SetNamedU32(env, binding, "kReadBytesOrError", kUnodeReadBytesOrError);
  SetNamedU32(env, binding, "kArrayBufferOffset", kUnodeArrayBufferOffset);
  SetNamedU32(env, binding, "kBytesWritten", kUnodeBytesWritten);
  SetNamedU32(env, binding, "kLastWriteWasAsync", kUnodeLastWriteWasAsync);

  napi_value global = nullptr;
  if (napi_get_global(env, &global) != napi_ok || global == nullptr) return;
  napi_set_named_property(env, global, "__unode_stream_wrap", binding);
}
