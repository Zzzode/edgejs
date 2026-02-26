'use strict';

// Minimal stub so require('util').TextEncoder / TextDecoder and
// require('internal/encoding') work. Re-exports global TextEncoder/TextDecoder.
module.exports = {
  get TextEncoder() {
    return globalThis.TextEncoder;
  },
  get TextDecoder() {
    return globalThis.TextDecoder;
  },
};
