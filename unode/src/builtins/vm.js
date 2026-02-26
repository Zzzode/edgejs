'use strict';

function runWithContext(code, context) {
  const sandbox = context && typeof context === 'object' ? context : {};
  const keys = Object.keys(sandbox);
  const hadOwn = Object.create(null);
  const previous = Object.create(null);

  for (const key of keys) {
    hadOwn[key] = Object.prototype.hasOwnProperty.call(globalThis, key);
    if (hadOwn[key]) {
      previous[key] = globalThis[key];
    }
    globalThis[key] = sandbox[key];
  }

  try {
    return (0, eval)(String(code));
  } finally {
    for (const key of keys) {
      if (hadOwn[key]) {
        globalThis[key] = previous[key];
      } else {
        delete globalThis[key];
      }
    }
  }
}

function runInNewContext(code, context) {
  return runWithContext(code, context);
}

function runInThisContext(code) {
  return (0, eval)(String(code));
}

module.exports = {
  runInNewContext,
  runInThisContext,
};
