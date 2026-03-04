#include "internal_binding/dispatch.h"

#include "internal_binding/helpers.h"

namespace internal_binding {

napi_value ResolveCrypto(napi_env env, const ResolveOptions& /*options*/) {
  napi_value out = GetGlobalNamed(env, "__ubi_crypto_binding");
  if (!IsUndefined(env, out)) return out;
  return GetGlobalNamed(env, "__ubi_crypto");
}

}  // namespace internal_binding
