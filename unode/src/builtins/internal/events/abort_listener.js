'use strict';

const { SymbolDispose } = primordials;

function addAbortListener(signal, listener) {
  if (!signal || typeof signal.addEventListener !== 'function') {
    return { [SymbolDispose]() {} };
  }
  signal.addEventListener('abort', listener, { once: true });
  return {
    [SymbolDispose]() {
      signal.removeEventListener('abort', listener);
    },
  };
}

module.exports = { addAbortListener };
