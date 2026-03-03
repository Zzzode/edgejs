'use strict';

async function gcUntil(_name, condition, maxCount = 10, gcOptions) {
  for (let i = 0; i < maxCount; i++) {
    if (typeof global.gc === 'function') {
      if (gcOptions !== undefined) {
        await global.gc(gcOptions);
      } else {
        await global.gc();
      }
    }
    if (typeof condition === 'function' && condition()) return;
  }
}

function onGC(_obj, _listener) {
  // Best-effort no-op in ubi compatibility mode.
}

async function checkIfCollectable() {
  // Best-effort no-op in ubi compatibility mode.
}

async function runAndBreathe(fn, repeat, waitTime = 20) {
  for (let i = 0; i < repeat; i++) {
    await fn();
    await new Promise((resolve) => setTimeout(resolve, waitTime));
  }
}

async function checkIfCollectableByCounting() {
  // Best-effort no-op in ubi compatibility mode.
}

module.exports = {
  checkIfCollectable,
  runAndBreathe,
  checkIfCollectableByCounting,
  onGC,
  gcUntil,
};
