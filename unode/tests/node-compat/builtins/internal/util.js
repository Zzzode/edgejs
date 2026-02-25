'use strict';

const isWindows = typeof process !== 'undefined' && process.platform === 'win32';
const kEmptyObject = Object.freeze({ __proto__: null });

function getLazy(initializer) {
  let initialized = false;
  let value;
  return function lazyValue() {
    if (!initialized) {
      value = initializer();
      initialized = true;
    }
    return value;
  };
}

function assignFunctionName(name, fn) {
  try {
    Object.defineProperty(fn, 'name', {
      __proto__: null,
      configurable: true,
      value: typeof name === 'symbol' ? name.description || '' : String(name),
    });
  } catch {
    // Ignore defineProperty failures for non-configurable names.
  }
  return fn;
}

const promisify = function promisify(fn) {
  return (...args) => new Promise((resolve, reject) => {
    fn(...args, (err, value) => (err ? reject(err) : resolve(value)));
  });
};
promisify.custom = Symbol.for('nodejs.util.promisify.custom');

function getCIDR(address, netmask, family) {
  if (typeof address !== 'string' || typeof netmask !== 'string') return null;
  if (family === 'IPv4') {
    const parts = netmask.split('.');
    if (parts.length !== 4) return null;
    let bits = 0;
    for (const part of parts) {
      const value = Number(part);
      if (!Number.isInteger(value) || value < 0 || value > 255) return null;
      bits += value.toString(2).split('').filter((ch) => ch === '1').length;
    }
    return `${address}/${bits}`;
  }
  if (family === 'IPv6') {
    const expanded = netmask.includes('::')
      ? (() => {
          const [left, right] = netmask.split('::');
          const leftParts = left ? left.split(':') : [];
          const rightParts = right ? right.split(':') : [];
          const missing = 8 - (leftParts.length + rightParts.length);
          return [...leftParts, ...Array(Math.max(0, missing)).fill('0'), ...rightParts];
        })()
      : netmask.split(':');
    if (expanded.length !== 8) return null;
    let bits = 0;
    for (const part of expanded) {
      const value = parseInt(part || '0', 16);
      if (!Number.isInteger(value) || value < 0 || value > 0xffff) return null;
      bits += value.toString(2).split('').filter((ch) => ch === '1').length;
    }
    return `${address}/${bits}`;
  }
  return null;
}

module.exports = {
  getLazy,
  isWindows,
  assignFunctionName,
  kEmptyObject,
  getCIDR,
  promisify,
};
