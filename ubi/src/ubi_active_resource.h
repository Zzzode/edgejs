#ifndef UBI_ACTIVE_RESOURCE_H_
#define UBI_ACTIVE_RESOURCE_H_

#include "node_api.h"

using UbiActiveHandleHasRef = bool (*)(void* data);
using UbiActiveHandleGetOwner = napi_value (*)(napi_env env, void* data);

void* UbiRegisterActiveHandle(napi_env env,
                              napi_value keepalive_owner,
                              const char* resource_name,
                              UbiActiveHandleHasRef has_ref,
                              UbiActiveHandleGetOwner get_owner,
                              void* data);
void UbiUnregisterActiveHandle(napi_env env, void* token);

void UbiTrackActiveRequest(napi_env env, napi_value req, const char* resource_name);
void UbiUntrackActiveRequest(napi_env env, napi_value req);

napi_value UbiGetActiveHandlesArray(napi_env env);
napi_value UbiGetActiveRequestsArray(napi_env env);
napi_value UbiGetActiveResourcesInfoArray(napi_env env);

#endif  // UBI_ACTIVE_RESOURCE_H_
