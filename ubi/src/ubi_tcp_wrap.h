#ifndef UBI_TCP_WRAP_H_
#define UBI_TCP_WRAP_H_

#include "node_api.h"
#include <uv.h>

void UbiInstallTcpWrapBinding(napi_env env);
uv_stream_t* UbiTcpWrapGetStream(napi_env env, napi_value value);

#endif  // UBI_TCP_WRAP_H_
