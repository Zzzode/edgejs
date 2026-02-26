'use strict';

function noop() {}

module.exports = {
  setHasRejectionToWarn: noop,
  hasRejectionToWarn() { return false; },
  listenForRejections() {},
  processPromiseRejections() { return false; },
};
