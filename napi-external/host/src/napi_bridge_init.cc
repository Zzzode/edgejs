// N-API bridge for the WASM host.
//
// Uses unofficial_napi_create_env() from napi-v8 to obtain a proper
// napi_env with all V8 scopes managed correctly.  Each N-API function
// is wrapped with an extern "C" bridge that takes/returns u32 handle IDs
// instead of opaque pointers, so the Rust host can translate between
// WASM guest memory and native N-API calls.

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "js_native_api.h"
#include "unofficial_napi.h"

namespace {

std::mutex g_mu;
napi_env g_env = nullptr;
void* g_scope = nullptr;  // opaque scope handle from unofficial_napi_create_env

// Handle table: maps u32 IDs to napi_value pointers.
std::unordered_map<uint32_t, napi_value> g_values;
uint32_t g_next_id = 1;

uint32_t StoreValue(napi_value val) {
  if (val == nullptr) return 0;
  uint32_t id = g_next_id++;
  g_values[id] = val;
  return id;
}

napi_value LoadValue(uint32_t id) {
  if (id == 0) return nullptr;
  auto it = g_values.find(id);
  return it != g_values.end() ? it->second : nullptr;
}

// Handle table for napi_ref (references)
std::unordered_map<uint32_t, napi_ref> g_refs;
uint32_t g_next_ref_id = 1;

uint32_t StoreRef(napi_ref ref) {
  if (ref == nullptr) return 0;
  uint32_t id = g_next_ref_id++;
  g_refs[id] = ref;
  return id;
}

napi_ref LoadRef(uint32_t id) {
  if (id == 0) return nullptr;
  auto it = g_refs.find(id);
  return it != g_refs.end() ? it->second : nullptr;
}

void RemoveRef(uint32_t id) { g_refs.erase(id); }

// Handle table for napi_deferred (promise deferreds)
std::unordered_map<uint32_t, napi_deferred> g_deferreds;
uint32_t g_next_deferred_id = 1;

uint32_t StoreDeferred(napi_deferred d) {
  if (d == nullptr) return 0;
  uint32_t id = g_next_deferred_id++;
  g_deferreds[id] = d;
  return id;
}

napi_deferred LoadDeferred(uint32_t id) {
  if (id == 0) return nullptr;
  auto it = g_deferreds.find(id);
  return it != g_deferreds.end() ? it->second : nullptr;
}

void RemoveDeferred(uint32_t id) { g_deferreds.erase(id); }

// Handle table for napi_escapable_handle_scope
std::unordered_map<uint32_t, napi_escapable_handle_scope> g_esc_scopes;
uint32_t g_next_esc_scope_id = 1;

uint32_t StoreEscScope(napi_escapable_handle_scope s) {
  if (s == nullptr) return 0;
  uint32_t id = g_next_esc_scope_id++;
  g_esc_scopes[id] = s;
  return id;
}

napi_escapable_handle_scope LoadEscScope(uint32_t id) {
  if (id == 0) return nullptr;
  auto it = g_esc_scopes.find(id);
  return it != g_esc_scopes.end() ? it->second : nullptr;
}

void RemoveEscScope(uint32_t id) { g_esc_scopes.erase(id); }

}  // namespace

// ============================================================
// Initialization
// ============================================================

extern "C" int snapi_bridge_init() {
  std::lock_guard<std::mutex> lock(g_mu);
  if (g_env != nullptr) return 1;
  napi_status s = unofficial_napi_create_env(8, &g_env, &g_scope);
  return (s == napi_ok && g_env != nullptr) ? 1 : 0;
}

// ============================================================
// Value creation
// ============================================================

extern "C" int snapi_bridge_get_undefined(uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_get_undefined(g_env, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_null(uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_get_null(g_env, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_boolean(int value, uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_get_boolean(g_env, value != 0, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_global(uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_get_global(g_env, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_string_utf8(const char* str,
                                               uint32_t wasm_length,
                                               uint32_t* out_id) {
  size_t length =
      (wasm_length == 0xFFFFFFFFu) ? NAPI_AUTO_LENGTH : (size_t)wasm_length;
  napi_value result;
  napi_status s = napi_create_string_utf8(g_env, str, length, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_string_latin1(const char* str,
                                                 uint32_t wasm_length,
                                                 uint32_t* out_id) {
  size_t length =
      (wasm_length == 0xFFFFFFFFu) ? NAPI_AUTO_LENGTH : (size_t)wasm_length;
  napi_value result;
  napi_status s = napi_create_string_latin1(g_env, str, length, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_int32(int32_t value, uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_int32(g_env, value, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_uint32(uint32_t value, uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_uint32(g_env, value, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_double(double value, uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_double(g_env, value, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_int64(int64_t value, uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_int64(g_env, value, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_object(uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_object(g_env, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_array(uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_array(g_env, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_array_with_length(uint32_t length,
                                                     uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_array_with_length(g_env, (size_t)length, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// Value reading
// ============================================================

extern "C" int snapi_bridge_get_value_string_utf8(uint32_t id, char* buf,
                                                  size_t bufsize,
                                                  size_t* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_string_utf8(g_env, val, buf, bufsize, result);
}

extern "C" int snapi_bridge_get_value_string_latin1(uint32_t id, char* buf,
                                                    size_t bufsize,
                                                    size_t* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_string_latin1(g_env, val, buf, bufsize, result);
}

extern "C" int snapi_bridge_get_value_int32(uint32_t id, int32_t* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_int32(g_env, val, result);
}

extern "C" int snapi_bridge_get_value_uint32(uint32_t id, uint32_t* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_uint32(g_env, val, result);
}

extern "C" int snapi_bridge_get_value_double(uint32_t id, double* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_double(g_env, val, result);
}

extern "C" int snapi_bridge_get_value_int64(uint32_t id, int64_t* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_int64(g_env, val, result);
}

extern "C" int snapi_bridge_get_value_bool(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool b;
  napi_status s = napi_get_value_bool(g_env, val, &b);
  if (s != napi_ok) return s;
  *result = b ? 1 : 0;
  return napi_ok;
}

// ============================================================
// Type checking
// ============================================================

extern "C" int snapi_bridge_typeof(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  napi_valuetype vtype;
  napi_status s = napi_typeof(g_env, val, &vtype);
  if (s != napi_ok) return s;
  *result = (int)vtype;
  return napi_ok;
}

extern "C" int snapi_bridge_is_array(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_array(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_is_error(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_error(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_is_arraybuffer(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_arraybuffer(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_is_typedarray(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_typedarray(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_is_dataview(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_dataview(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_is_date(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_date(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_is_promise(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_promise(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_instanceof(uint32_t obj_id, uint32_t ctor_id,
                                       int* result) {
  napi_value obj = LoadValue(obj_id);
  napi_value ctor = LoadValue(ctor_id);
  if (!obj || !ctor) return napi_invalid_arg;
  bool is;
  napi_status s = napi_instanceof(g_env, obj, ctor, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

// ============================================================
// Coercion
// ============================================================

extern "C" int snapi_bridge_coerce_to_bool(uint32_t id, uint32_t* out_id) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_coerce_to_bool(g_env, val, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_coerce_to_number(uint32_t id, uint32_t* out_id) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_coerce_to_number(g_env, val, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_coerce_to_string(uint32_t id, uint32_t* out_id) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_coerce_to_string(g_env, val, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_coerce_to_object(uint32_t id, uint32_t* out_id) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_coerce_to_object(g_env, val, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// Object operations
// ============================================================

extern "C" int snapi_bridge_set_property(uint32_t obj_id, uint32_t key_id,
                                         uint32_t val_id) {
  napi_value obj = LoadValue(obj_id);
  napi_value key = LoadValue(key_id);
  napi_value val = LoadValue(val_id);
  if (!obj || !key || !val) return napi_invalid_arg;
  return napi_set_property(g_env, obj, key, val);
}

extern "C" int snapi_bridge_get_property(uint32_t obj_id, uint32_t key_id,
                                         uint32_t* out_id) {
  napi_value obj = LoadValue(obj_id);
  napi_value key = LoadValue(key_id);
  if (!obj || !key) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_get_property(g_env, obj, key, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_has_property(uint32_t obj_id, uint32_t key_id,
                                         int* result) {
  napi_value obj = LoadValue(obj_id);
  napi_value key = LoadValue(key_id);
  if (!obj || !key) return napi_invalid_arg;
  bool has;
  napi_status s = napi_has_property(g_env, obj, key, &has);
  if (s != napi_ok) return s;
  *result = has ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_has_own_property(uint32_t obj_id, uint32_t key_id,
                                             int* result) {
  napi_value obj = LoadValue(obj_id);
  napi_value key = LoadValue(key_id);
  if (!obj || !key) return napi_invalid_arg;
  bool has;
  napi_status s = napi_has_own_property(g_env, obj, key, &has);
  if (s != napi_ok) return s;
  *result = has ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_delete_property(uint32_t obj_id, uint32_t key_id,
                                            int* result) {
  napi_value obj = LoadValue(obj_id);
  napi_value key = LoadValue(key_id);
  if (!obj || !key) return napi_invalid_arg;
  bool deleted;
  napi_status s = napi_delete_property(g_env, obj, key, &deleted);
  if (s != napi_ok) return s;
  *result = deleted ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_set_named_property(uint32_t obj_id,
                                               const char* name,
                                               uint32_t val_id) {
  napi_value obj = LoadValue(obj_id);
  napi_value val = LoadValue(val_id);
  if (!obj || !val || !name) return napi_invalid_arg;
  return napi_set_named_property(g_env, obj, name, val);
}

extern "C" int snapi_bridge_get_named_property(uint32_t obj_id,
                                               const char* name,
                                               uint32_t* out_id) {
  napi_value obj = LoadValue(obj_id);
  if (!obj || !name) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_get_named_property(g_env, obj, name, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_has_named_property(uint32_t obj_id,
                                               const char* name,
                                               int* result) {
  napi_value obj = LoadValue(obj_id);
  if (!obj || !name) return napi_invalid_arg;
  bool has;
  napi_status s = napi_has_named_property(g_env, obj, name, &has);
  if (s != napi_ok) return s;
  *result = has ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_set_element(uint32_t obj_id, uint32_t index,
                                        uint32_t val_id) {
  napi_value obj = LoadValue(obj_id);
  napi_value val = LoadValue(val_id);
  if (!obj || !val) return napi_invalid_arg;
  return napi_set_element(g_env, obj, index, val);
}

extern "C" int snapi_bridge_get_element(uint32_t obj_id, uint32_t index,
                                        uint32_t* out_id) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_get_element(g_env, obj, index, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_has_element(uint32_t obj_id, uint32_t index,
                                        int* result) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  bool has;
  napi_status s = napi_has_element(g_env, obj, index, &has);
  if (s != napi_ok) return s;
  *result = has ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_delete_element(uint32_t obj_id, uint32_t index,
                                           int* result) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  bool deleted;
  napi_status s = napi_delete_element(g_env, obj, index, &deleted);
  if (s != napi_ok) return s;
  *result = deleted ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_get_array_length(uint32_t arr_id,
                                             uint32_t* result) {
  napi_value arr = LoadValue(arr_id);
  if (!arr) return napi_invalid_arg;
  return napi_get_array_length(g_env, arr, result);
}

extern "C" int snapi_bridge_get_property_names(uint32_t obj_id,
                                               uint32_t* out_id) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_get_property_names(g_env, obj, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_all_property_names(uint32_t obj_id,
                                                   int mode, int filter,
                                                   int conversion,
                                                   uint32_t* out_id) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_get_all_property_names(
      g_env, obj, (napi_key_collection_mode)mode, (napi_key_filter)filter,
      (napi_key_conversion)conversion, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_prototype(uint32_t obj_id, uint32_t* out_id) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_get_prototype(g_env, obj, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_object_freeze(uint32_t obj_id) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  return napi_object_freeze(g_env, obj);
}

extern "C" int snapi_bridge_object_seal(uint32_t obj_id) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  return napi_object_seal(g_env, obj);
}

// ============================================================
// Comparison
// ============================================================

extern "C" int snapi_bridge_strict_equals(uint32_t a_id, uint32_t b_id,
                                          int* result) {
  napi_value a = LoadValue(a_id);
  napi_value b = LoadValue(b_id);
  if (!a || !b) return napi_invalid_arg;
  bool eq;
  napi_status s = napi_strict_equals(g_env, a, b, &eq);
  if (s != napi_ok) return s;
  *result = eq ? 1 : 0;
  return napi_ok;
}

// ============================================================
// Error handling
// ============================================================

extern "C" int snapi_bridge_create_error(uint32_t code_id, uint32_t msg_id,
                                         uint32_t* out_id) {
  napi_value code = LoadValue(code_id);  // can be null (0)
  napi_value msg = LoadValue(msg_id);
  if (!msg) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_create_error(g_env, code, msg, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_type_error(uint32_t code_id,
                                              uint32_t msg_id,
                                              uint32_t* out_id) {
  napi_value code = LoadValue(code_id);
  napi_value msg = LoadValue(msg_id);
  if (!msg) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_create_type_error(g_env, code, msg, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_range_error(uint32_t code_id,
                                               uint32_t msg_id,
                                               uint32_t* out_id) {
  napi_value code = LoadValue(code_id);
  napi_value msg = LoadValue(msg_id);
  if (!msg) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_create_range_error(g_env, code, msg, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_throw(uint32_t error_id) {
  napi_value error = LoadValue(error_id);
  if (!error) return napi_invalid_arg;
  return napi_throw(g_env, error);
}

extern "C" int snapi_bridge_throw_error(const char* code, const char* msg) {
  return napi_throw_error(g_env, code, msg);
}

extern "C" int snapi_bridge_throw_type_error(const char* code,
                                             const char* msg) {
  return napi_throw_type_error(g_env, code, msg);
}

extern "C" int snapi_bridge_throw_range_error(const char* code,
                                              const char* msg) {
  return napi_throw_range_error(g_env, code, msg);
}

extern "C" int snapi_bridge_is_exception_pending(int* result) {
  bool pending;
  napi_status s = napi_is_exception_pending(g_env, &pending);
  if (s != napi_ok) return s;
  *result = pending ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_get_and_clear_last_exception(uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_get_and_clear_last_exception(g_env, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// Symbol
// ============================================================

extern "C" int snapi_bridge_create_symbol(uint32_t description_id,
                                          uint32_t* out_id) {
  napi_value description = LoadValue(description_id);  // can be null (0)
  napi_value result;
  napi_status s = napi_create_symbol(g_env, description, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// BigInt
// ============================================================

extern "C" int snapi_bridge_create_bigint_int64(int64_t value,
                                                uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_bigint_int64(g_env, value, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_bigint_uint64(uint64_t value,
                                                 uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_bigint_uint64(g_env, value, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_value_bigint_int64(uint32_t id, int64_t* value,
                                                   int* lossless) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool loss;
  napi_status s = napi_get_value_bigint_int64(g_env, val, value, &loss);
  if (s != napi_ok) return s;
  *lossless = loss ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_get_value_bigint_uint64(uint32_t id,
                                                    uint64_t* value,
                                                    int* lossless) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool loss;
  napi_status s = napi_get_value_bigint_uint64(g_env, val, value, &loss);
  if (s != napi_ok) return s;
  *lossless = loss ? 1 : 0;
  return napi_ok;
}

// ============================================================
// Date
// ============================================================

extern "C" int snapi_bridge_create_date(double time, uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_date(g_env, time, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_date_value(uint32_t id, double* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_date_value(g_env, val, result);
}

// ============================================================
// Promise
// ============================================================

extern "C" int snapi_bridge_create_promise(uint32_t* deferred_out,
                                           uint32_t* out_id) {
  napi_deferred deferred;
  napi_value promise;
  napi_status s = napi_create_promise(g_env, &deferred, &promise);
  if (s != napi_ok) return s;
  *deferred_out = StoreDeferred(deferred);
  *out_id = StoreValue(promise);
  return napi_ok;
}

extern "C" int snapi_bridge_resolve_deferred(uint32_t deferred_id,
                                             uint32_t value_id) {
  napi_deferred d = LoadDeferred(deferred_id);
  napi_value val = LoadValue(value_id);
  if (!d || !val) return napi_invalid_arg;
  napi_status s = napi_resolve_deferred(g_env, d, val);
  if (s == napi_ok) RemoveDeferred(deferred_id);
  return s;
}

extern "C" int snapi_bridge_reject_deferred(uint32_t deferred_id,
                                            uint32_t value_id) {
  napi_deferred d = LoadDeferred(deferred_id);
  napi_value val = LoadValue(value_id);
  if (!d || !val) return napi_invalid_arg;
  napi_status s = napi_reject_deferred(g_env, d, val);
  if (s == napi_ok) RemoveDeferred(deferred_id);
  return s;
}

// ============================================================
// ArrayBuffer
// ============================================================

extern "C" int snapi_bridge_create_arraybuffer(uint32_t byte_length,
                                               uint32_t* out_id) {
  void* data;
  napi_value result;
  napi_status s =
      napi_create_arraybuffer(g_env, (size_t)byte_length, &data, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_create_external_arraybuffer(uint64_t data_addr,
                                                         uint32_t byte_length,
                                                         uint32_t* out_id) {
  void* data = (void*)(uintptr_t)data_addr;
  napi_value result;
  napi_status s = napi_create_external_arraybuffer(
      g_env, data, (size_t)byte_length, nullptr, nullptr, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_arraybuffer_info(uint32_t id,
                                                 uint64_t* data_out,
                                                 uint32_t* byte_length) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  void* data;
  size_t len;
  napi_status s = napi_get_arraybuffer_info(g_env, val, &data, &len);
  if (s != napi_ok) return s;
  if (data_out) *data_out = (uint64_t)(uintptr_t)data;
  *byte_length = (uint32_t)len;
  return napi_ok;
}

extern "C" int snapi_bridge_detach_arraybuffer(uint32_t id) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_detach_arraybuffer(g_env, val);
}

extern "C" int snapi_bridge_is_detached_arraybuffer(uint32_t id, int* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is;
  napi_status s = napi_is_detached_arraybuffer(g_env, val, &is);
  if (s != napi_ok) return s;
  *result = is ? 1 : 0;
  return napi_ok;
}

// ============================================================
// TypedArray
// ============================================================

extern "C" int snapi_bridge_create_typedarray(int type, uint32_t length,
                                              uint32_t arraybuffer_id,
                                              uint32_t byte_offset,
                                              uint32_t* out_id) {
  napi_value arraybuffer = LoadValue(arraybuffer_id);
  if (!arraybuffer) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_create_typedarray(
      g_env, (napi_typedarray_type)type, (size_t)length, arraybuffer,
      (size_t)byte_offset, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_typedarray_info(uint32_t id, int* type_out,
                                                uint32_t* length_out,
                                                uint32_t* arraybuffer_out,
                                                uint32_t* byte_offset_out) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  napi_status s = napi_get_typedarray_info(g_env, val, &type, &length, nullptr,
                                           &arraybuffer, &byte_offset);
  if (s != napi_ok) return s;
  if (type_out) *type_out = (int)type;
  if (length_out) *length_out = (uint32_t)length;
  if (arraybuffer_out) *arraybuffer_out = StoreValue(arraybuffer);
  if (byte_offset_out) *byte_offset_out = (uint32_t)byte_offset;
  return napi_ok;
}

// ============================================================
// DataView
// ============================================================

extern "C" int snapi_bridge_create_dataview(uint32_t byte_length,
                                            uint32_t arraybuffer_id,
                                            uint32_t byte_offset,
                                            uint32_t* out_id) {
  napi_value arraybuffer = LoadValue(arraybuffer_id);
  if (!arraybuffer) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_create_dataview(g_env, (size_t)byte_length, arraybuffer,
                                       (size_t)byte_offset, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_dataview_info(uint32_t id,
                                              uint32_t* byte_length_out,
                                              uint32_t* arraybuffer_out,
                                              uint32_t* byte_offset_out) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  size_t byte_length;
  napi_value arraybuffer;
  size_t byte_offset;
  napi_status s = napi_get_dataview_info(g_env, val, &byte_length, nullptr,
                                         &arraybuffer, &byte_offset);
  if (s != napi_ok) return s;
  if (byte_length_out) *byte_length_out = (uint32_t)byte_length;
  if (arraybuffer_out) *arraybuffer_out = StoreValue(arraybuffer);
  if (byte_offset_out) *byte_offset_out = (uint32_t)byte_offset;
  return napi_ok;
}

// ============================================================
// External values
// ============================================================

extern "C" int snapi_bridge_create_external(uint64_t data_val,
                                            uint32_t* out_id) {
  // Store arbitrary u64 data value as a void*. No finalizer.
  napi_value result;
  napi_status s = napi_create_external(g_env, (void*)(uintptr_t)data_val,
                                       nullptr, nullptr, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_value_external(uint32_t id,
                                               uint64_t* data_out) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  void* data;
  napi_status s = napi_get_value_external(g_env, val, &data);
  if (s != napi_ok) return s;
  *data_out = (uint64_t)(uintptr_t)data;
  return napi_ok;
}

// ============================================================
// References
// ============================================================

extern "C" int snapi_bridge_create_reference(uint32_t value_id,
                                             uint32_t initial_refcount,
                                             uint32_t* ref_out) {
  napi_value val = LoadValue(value_id);
  if (!val) return napi_invalid_arg;
  napi_ref ref;
  napi_status s = napi_create_reference(g_env, val, initial_refcount, &ref);
  if (s != napi_ok) return s;
  *ref_out = StoreRef(ref);
  return napi_ok;
}

extern "C" int snapi_bridge_delete_reference(uint32_t ref_id) {
  napi_ref ref = LoadRef(ref_id);
  if (!ref) return napi_invalid_arg;
  napi_status s = napi_delete_reference(g_env, ref);
  if (s == napi_ok) RemoveRef(ref_id);
  return s;
}

extern "C" int snapi_bridge_reference_ref(uint32_t ref_id, uint32_t* result) {
  napi_ref ref = LoadRef(ref_id);
  if (!ref) return napi_invalid_arg;
  return napi_reference_ref(g_env, ref, result);
}

extern "C" int snapi_bridge_reference_unref(uint32_t ref_id, uint32_t* result) {
  napi_ref ref = LoadRef(ref_id);
  if (!ref) return napi_invalid_arg;
  return napi_reference_unref(g_env, ref, result);
}

extern "C" int snapi_bridge_get_reference_value(uint32_t ref_id,
                                                uint32_t* out_id) {
  napi_ref ref = LoadRef(ref_id);
  if (!ref) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_get_reference_value(g_env, ref, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// Handle scopes (escapable)
// ============================================================

extern "C" int snapi_bridge_open_escapable_handle_scope(uint32_t* scope_out) {
  napi_escapable_handle_scope scope;
  napi_status s = napi_open_escapable_handle_scope(g_env, &scope);
  if (s != napi_ok) return s;
  *scope_out = StoreEscScope(scope);
  return napi_ok;
}

extern "C" int snapi_bridge_close_escapable_handle_scope(uint32_t scope_id) {
  napi_escapable_handle_scope scope = LoadEscScope(scope_id);
  if (!scope) return napi_invalid_arg;
  napi_status s = napi_close_escapable_handle_scope(g_env, scope);
  if (s == napi_ok) RemoveEscScope(scope_id);
  return s;
}

extern "C" int snapi_bridge_escape_handle(uint32_t scope_id,
                                          uint32_t escapee_id,
                                          uint32_t* out_id) {
  napi_escapable_handle_scope scope = LoadEscScope(scope_id);
  napi_value escapee = LoadValue(escapee_id);
  if (!scope || !escapee) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_escape_handle(g_env, scope, escapee, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// Type tagging
// ============================================================

extern "C" int snapi_bridge_type_tag_object(uint32_t obj_id,
                                            uint64_t tag_lower,
                                            uint64_t tag_upper) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  napi_type_tag tag;
  tag.lower = tag_lower;
  tag.upper = tag_upper;
  return napi_type_tag_object(g_env, obj, &tag);
}

extern "C" int snapi_bridge_check_object_type_tag(uint32_t obj_id,
                                                  uint64_t tag_lower,
                                                  uint64_t tag_upper,
                                                  int* result) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  napi_type_tag tag;
  tag.lower = tag_lower;
  tag.upper = tag_upper;
  bool matches;
  napi_status s = napi_check_object_type_tag(g_env, obj, &tag, &matches);
  if (s != napi_ok) return s;
  *result = matches ? 1 : 0;
  return napi_ok;
}

// ============================================================
// Function calling (call JS functions from native)
// ============================================================

extern "C" int snapi_bridge_call_function(uint32_t recv_id, uint32_t func_id,
                                          uint32_t argc,
                                          const uint32_t* argv_ids,
                                          uint32_t* out_id) {
  napi_value recv = LoadValue(recv_id);
  napi_value func = LoadValue(func_id);
  if (!recv || !func) return napi_invalid_arg;
  std::vector<napi_value> argv(argc);
  for (uint32_t i = 0; i < argc; i++) {
    argv[i] = LoadValue(argv_ids[i]);
    if (!argv[i]) return napi_invalid_arg;
  }
  napi_value result;
  napi_status s =
      napi_call_function(g_env, recv, func, argc, argv.data(), &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// Script execution
// ============================================================

extern "C" int snapi_bridge_run_script(uint32_t script_id,
                                       uint32_t* out_value_id) {
  napi_value script_val = LoadValue(script_id);
  if (!script_val) return napi_invalid_arg;
  napi_value result;
  napi_status s = napi_run_script(g_env, script_val, &result);
  if (s != napi_ok) return s;
  *out_value_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// UTF-16 strings
// ============================================================

extern "C" int snapi_bridge_create_string_utf16(const uint16_t* str,
                                                uint32_t wasm_length,
                                                uint32_t* out_id) {
  size_t length =
      (wasm_length == 0xFFFFFFFFu) ? NAPI_AUTO_LENGTH : (size_t)wasm_length;
  napi_value result;
  napi_status s = napi_create_string_utf16(g_env, (const char16_t*)str, length,
                                           &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_value_string_utf16(uint32_t id,
                                                   uint16_t* buf,
                                                   size_t bufsize,
                                                   size_t* result) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_string_utf16(g_env, val, (char16_t*)buf, bufsize,
                                     result);
}

// ============================================================
// BigInt words (arbitrary precision)
// ============================================================

extern "C" int snapi_bridge_create_bigint_words(int sign_bit,
                                                uint32_t word_count,
                                                const uint64_t* words,
                                                uint32_t* out_id) {
  napi_value result;
  napi_status s = napi_create_bigint_words(g_env, sign_bit, (size_t)word_count,
                                           words, &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

extern "C" int snapi_bridge_get_value_bigint_words(uint32_t id,
                                                   int* sign_bit,
                                                   size_t* word_count,
                                                   uint64_t* words) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  return napi_get_value_bigint_words(g_env, val, sign_bit, word_count, words);
}

// ============================================================
// Instance data
// ============================================================

extern "C" int snapi_bridge_set_instance_data(uint64_t data_val) {
  return napi_set_instance_data(g_env, (void*)(uintptr_t)data_val,
                                nullptr, nullptr);
}

extern "C" int snapi_bridge_get_instance_data(uint64_t* data_out) {
  void* data = nullptr;
  napi_status s = napi_get_instance_data(g_env, &data);
  if (s != napi_ok) return s;
  *data_out = (uint64_t)(uintptr_t)data;
  return napi_ok;
}

extern "C" int snapi_bridge_adjust_external_memory(int64_t change,
                                                   int64_t* adjusted) {
  return napi_adjust_external_memory(g_env, change, adjusted);
}

// ============================================================
// Node Buffers (implemented via Uint8Array + ArrayBuffer since
// napi-v8 doesn't have node_api.h buffer functions)
// ============================================================

extern "C" int snapi_bridge_create_buffer(uint32_t length,
                                          uint64_t* data_out,
                                          uint32_t* out_id) {
  // Create an ArrayBuffer, then a Uint8Array view over it
  napi_value arraybuffer;
  void* ab_data = nullptr;
  napi_status s = napi_create_arraybuffer(g_env, (size_t)length, &ab_data,
                                          &arraybuffer);
  if (s != napi_ok) return s;
  napi_value uint8array;
  s = napi_create_typedarray(g_env, napi_uint8_array, (size_t)length,
                             arraybuffer, 0, &uint8array);
  if (s != napi_ok) return s;
  *out_id = StoreValue(uint8array);
  if (data_out) *data_out = (uint64_t)(uintptr_t)ab_data;
  return napi_ok;
}

extern "C" int snapi_bridge_create_buffer_copy(uint32_t length,
                                               const void* src_data,
                                               uint64_t* result_data_out,
                                               uint32_t* out_id) {
  // Create an ArrayBuffer, copy data in, wrap with Uint8Array
  napi_value arraybuffer;
  void* ab_data = nullptr;
  napi_status s = napi_create_arraybuffer(g_env, (size_t)length, &ab_data,
                                          &arraybuffer);
  if (s != napi_ok) return s;
  if (ab_data && src_data && length > 0) {
    memcpy(ab_data, src_data, length);
  }
  napi_value uint8array;
  s = napi_create_typedarray(g_env, napi_uint8_array, (size_t)length,
                             arraybuffer, 0, &uint8array);
  if (s != napi_ok) return s;
  *out_id = StoreValue(uint8array);
  if (result_data_out) *result_data_out = (uint64_t)(uintptr_t)ab_data;
  return napi_ok;
}

extern "C" int snapi_bridge_is_buffer(uint32_t id, int* result) {
  // A "buffer" in our implementation is a Uint8Array
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  bool is_ta;
  napi_status s = napi_is_typedarray(g_env, val, &is_ta);
  if (s != napi_ok) return s;
  if (!is_ta) {
    *result = 0;
    return napi_ok;
  }
  // Check if it's specifically a Uint8Array
  napi_typedarray_type ta_type;
  s = napi_get_typedarray_info(g_env, val, &ta_type, nullptr, nullptr,
                               nullptr, nullptr);
  if (s != napi_ok) return s;
  *result = (ta_type == napi_uint8_array) ? 1 : 0;
  return napi_ok;
}

extern "C" int snapi_bridge_get_buffer_info(uint32_t id,
                                            uint64_t* data_out,
                                            uint32_t* length_out) {
  napi_value val = LoadValue(id);
  if (!val) return napi_invalid_arg;
  // Get typedarray info to find the backing arraybuffer and length
  napi_typedarray_type ta_type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  napi_status s = napi_get_typedarray_info(g_env, val, &ta_type, &length,
                                           nullptr, &arraybuffer, &byte_offset);
  if (s != napi_ok) return s;
  if (length_out) *length_out = (uint32_t)length;
  // Get the data pointer from the backing ArrayBuffer (not from the typedarray,
  // which may return a V8 sandbox-mapped address for external arraybuffers)
  if (data_out) {
    void* ab_data = nullptr;
    size_t ab_len = 0;
    s = napi_get_arraybuffer_info(g_env, arraybuffer, &ab_data, &ab_len);
    if (s != napi_ok) return s;
    // Apply byte_offset to get the actual data start
    *data_out = (uint64_t)(uintptr_t)((uint8_t*)ab_data + byte_offset);
  }
  return napi_ok;
}

// ============================================================
// Node version (stub — we're not running in Node, return fake version)
// ============================================================

extern "C" int snapi_bridge_get_node_version(uint32_t* major,
                                             uint32_t* minor,
                                             uint32_t* patch) {
  // Return a reasonable fake version since we're running on pure V8
  if (major) *major = 22;
  if (minor) *minor = 0;
  if (patch) *patch = 0;
  return napi_ok;
}

// ============================================================
// Object wrapping
// ============================================================

extern "C" int snapi_bridge_wrap(uint32_t obj_id, uint64_t native_data,
                                 uint32_t* ref_out) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  napi_ref ref = nullptr;
  napi_status s = napi_wrap(g_env, obj, (void*)(uintptr_t)native_data,
                            nullptr, nullptr, ref_out ? &ref : nullptr);
  if (s != napi_ok) return s;
  if (ref_out) *ref_out = StoreRef(ref);
  return napi_ok;
}

extern "C" int snapi_bridge_unwrap(uint32_t obj_id, uint64_t* data_out) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  void* data = nullptr;
  napi_status s = napi_unwrap(g_env, obj, &data);
  if (s != napi_ok) return s;
  if (data_out) *data_out = (uint64_t)(uintptr_t)data;
  return napi_ok;
}

extern "C" int snapi_bridge_remove_wrap(uint32_t obj_id, uint64_t* data_out) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  void* data = nullptr;
  napi_status s = napi_remove_wrap(g_env, obj, &data);
  if (s != napi_ok) return s;
  if (data_out) *data_out = (uint64_t)(uintptr_t)data;
  return napi_ok;
}

extern "C" int snapi_bridge_add_finalizer(uint32_t obj_id, uint64_t data_val,
                                          uint32_t* ref_out) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  // No actual WASM callback for finalizer; just register with nullptr callback
  napi_ref ref = nullptr;
  napi_status s = napi_add_finalizer(g_env, obj, (void*)(uintptr_t)data_val,
                                     nullptr, nullptr, ref_out ? &ref : nullptr);
  if (s != napi_ok) return s;
  if (ref_out) *ref_out = StoreRef(ref);
  return napi_ok;
}

// ============================================================
// napi_new_instance
// ============================================================

extern "C" int snapi_bridge_new_instance(uint32_t ctor_id, uint32_t argc,
                                         const uint32_t* argv_ids,
                                         uint32_t* out_id) {
  napi_value ctor = LoadValue(ctor_id);
  if (!ctor) return napi_invalid_arg;
  std::vector<napi_value> argv(argc);
  for (uint32_t i = 0; i < argc; i++) {
    argv[i] = LoadValue(argv_ids[i]);
    if (!argv[i]) return napi_invalid_arg;
  }
  napi_value result;
  napi_status s = napi_new_instance(g_env, ctor, argc, argv.data(), &result);
  if (s != napi_ok) return s;
  *out_id = StoreValue(result);
  return napi_ok;
}

// ============================================================
// napi_define_properties
// ============================================================

extern "C" int snapi_bridge_define_properties(uint32_t obj_id,
                                              uint32_t prop_count,
                                              const char** utf8names,
                                              const uint32_t* value_ids,
                                              const int32_t* attributes) {
  napi_value obj = LoadValue(obj_id);
  if (!obj) return napi_invalid_arg;
  std::vector<napi_property_descriptor> descs(prop_count);
  for (uint32_t i = 0; i < prop_count; i++) {
    memset(&descs[i], 0, sizeof(napi_property_descriptor));
    descs[i].utf8name = utf8names[i];
    descs[i].value = LoadValue(value_ids[i]);
    descs[i].attributes = (napi_property_attributes)attributes[i];
  }
  return napi_define_properties(g_env, obj, prop_count, descs.data());
}

// ============================================================
// Cleanup
// ============================================================

extern "C" void snapi_bridge_dispose() {
  std::lock_guard<std::mutex> lock(g_mu);
  g_values.clear();
  g_next_id = 1;
  g_refs.clear();
  g_next_ref_id = 1;
  g_deferreds.clear();
  g_next_deferred_id = 1;
  g_esc_scopes.clear();
  g_next_esc_scope_id = 1;
  if (g_scope) {
    unofficial_napi_release_env(g_scope);
    g_scope = nullptr;
    g_env = nullptr;
  }
}
