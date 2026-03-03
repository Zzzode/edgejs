#include "ubi_crypto.h"

#include "crypto/ubi_crypto_binding.h"

void UbiInstallCryptoBinding(napi_env env) {
  ubi::crypto::InstallCryptoBinding(env);
  napi_value global = nullptr;
  napi_value binding = nullptr;
  if (napi_get_global(env, &global) != napi_ok || global == nullptr) return;
  if (napi_get_named_property(env, global, "__ubi_crypto_binding", &binding) != napi_ok || binding == nullptr) {
    return;
  }
  napi_set_named_property(env, global, "__ubi_crypto", binding);
}
