'use strict';

class AsyncContextFrame extends Map {
  static get enabled() {
    return false;
  }

  static current() {}
  static set(_frame) {}
  static exchange(_frame) {}
  static disable(_store) {}
}

module.exports = AsyncContextFrame;
