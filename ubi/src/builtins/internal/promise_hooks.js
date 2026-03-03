'use strict';

module.exports = {
  createHook() {
    return {
      enable() { return this; },
      disable() { return this; },
    };
  },
  onInit() {},
  onBefore() {},
  onAfter() {},
  onSettled() {},
};
