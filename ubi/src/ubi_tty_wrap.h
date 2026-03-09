#ifndef UBI_TTY_WRAP_H_
#define UBI_TTY_WRAP_H_

#include <uv.h>

#include "node_api.h"

napi_value UbiInstallTtyWrapBinding(napi_env env);
uv_stream_t* UbiTtyWrapGetStream(napi_env env, napi_value value);

#endif  // UBI_TTY_WRAP_H_
