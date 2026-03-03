'use strict';

class DummyWebSocket {}
class DummyCloseEvent {}
class DummyMessageEvent {}

module.exports = {
  WebSocket: globalThis.WebSocket || DummyWebSocket,
  CloseEvent: globalThis.CloseEvent || DummyCloseEvent,
  MessageEvent: globalThis.MessageEvent || DummyMessageEvent,
};
