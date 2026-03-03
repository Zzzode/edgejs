#ifndef UBI_CRYPTO_BINDING_H_
#define UBI_CRYPTO_BINDING_H_

#include "node_api.h"

namespace ubi::crypto {

void InstallCryptoBinding(napi_env env);

}  // namespace ubi::crypto

#endif  // UBI_CRYPTO_BINDING_H_
