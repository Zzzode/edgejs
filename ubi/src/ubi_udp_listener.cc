#include "ubi_udp_listener.h"

UbiUdpListener::~UbiUdpListener() {
  if (wrap_ != nullptr) wrap_->set_listener(nullptr);
}

UbiUdpWrapBase::~UbiUdpWrapBase() {
  set_listener(nullptr);
}

void UbiUdpWrapBase::set_listener(UbiUdpListener* listener) {
  if (listener_ != nullptr) listener_->wrap_ = nullptr;
  listener_ = listener;
  if (listener_ != nullptr) listener_->wrap_ = this;
}

UbiUdpListener* UbiUdpWrapBase::listener() const {
  return listener_;
}
