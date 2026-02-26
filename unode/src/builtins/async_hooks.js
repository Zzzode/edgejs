'use strict';

const { AsyncResource } = require('internal/async_hooks');

function createHook() {
  return {
    enable() { return this; },
    disable() { return this; },
  };
}

module.exports = {
  AsyncResource,
  createHook,
  executionAsyncId() { return 0; },
  triggerAsyncId() { return 0; },
  executionAsyncResource() { return {}; },
};
