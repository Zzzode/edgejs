'use strict';

// Stub for Node's internal/v8/startup_snapshot. Node code (e.g. internal/dns/utils.js) destructures
// require('internal/v8/startup_snapshot').namespace.addSerializeCallback, so we must export the
// same shape. No-ops are fine since unode does not build snapshots.
function noop() {}
function isBuildingSnapshot() {
  return false;
}

module.exports = {
  initializeCallbacks: noop,
  isBuildingSnapshot,
  runDeserializeCallbacks: noop,
  throwIfBuildingSnapshot: noop,
  addAfterUserSerializeCallback: noop,
  namespace: {
    addSerializeCallback: noop,
    addDeserializeCallback: noop,
    setDeserializeMainFunction: noop,
    isBuildingSnapshot,
  },
};
