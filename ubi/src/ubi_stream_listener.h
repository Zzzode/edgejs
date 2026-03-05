#ifndef UBI_STREAM_LISTENER_H_
#define UBI_STREAM_LISTENER_H_

#include <cstddef>

#include <uv.h>

struct UbiStreamListener;

using UbiStreamOnAlloc = bool (*)(UbiStreamListener* listener,
                                  size_t suggested_size,
                                  uv_buf_t* out);
using UbiStreamOnRead = bool (*)(UbiStreamListener* listener,
                                 ssize_t nread,
                                 const uv_buf_t* buf);
using UbiStreamOnClose = void (*)(UbiStreamListener* listener);

struct UbiStreamListener {
  UbiStreamListener* previous = nullptr;
  UbiStreamOnAlloc on_alloc = nullptr;
  UbiStreamOnRead on_read = nullptr;
  UbiStreamOnClose on_close = nullptr;
  void* data = nullptr;
};

struct UbiStreamListenerState {
  UbiStreamListener* current = nullptr;
};

void UbiInitStreamListenerState(UbiStreamListenerState* state,
                                UbiStreamListener* initial);
void UbiPushStreamListener(UbiStreamListenerState* state,
                           UbiStreamListener* listener);
bool UbiRemoveStreamListener(UbiStreamListenerState* state,
                             UbiStreamListener* listener);
bool UbiStreamEmitAlloc(UbiStreamListenerState* state,
                        size_t suggested_size,
                        uv_buf_t* out);
bool UbiStreamEmitRead(UbiStreamListenerState* state,
                       ssize_t nread,
                       const uv_buf_t* buf);
void UbiStreamNotifyClosed(UbiStreamListenerState* state);

#endif  // UBI_STREAM_LISTENER_H_
