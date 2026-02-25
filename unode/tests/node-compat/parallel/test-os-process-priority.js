'use strict';
const assert = require('assert');
const os = require('os');
const {
  PRIORITY_LOW,
  PRIORITY_BELOW_NORMAL,
  PRIORITY_NORMAL,
  PRIORITY_ABOVE_NORMAL,
  PRIORITY_HIGH,
  PRIORITY_HIGHEST
} = os.constants.priority;

assert.strictEqual(typeof PRIORITY_LOW, 'number');
assert.strictEqual(typeof PRIORITY_BELOW_NORMAL, 'number');
assert.strictEqual(typeof PRIORITY_NORMAL, 'number');
assert.strictEqual(typeof PRIORITY_ABOVE_NORMAL, 'number');
assert.strictEqual(typeof PRIORITY_HIGH, 'number');
assert.strictEqual(typeof PRIORITY_HIGHEST, 'number');

[null, true, false, 'foo', {}, [], /x/].forEach((pid) => {
  const errObj = {
    code: 'ERR_INVALID_ARG_TYPE',
    message: /The "pid" argument must be of type number\./
  };
  assert.throws(() => {
    os.setPriority(pid, PRIORITY_NORMAL);
  }, errObj);
  assert.throws(() => {
    os.getPriority(pid);
  }, errObj);
});

[NaN, Infinity, -Infinity, 3.14, 2 ** 32].forEach((pid) => {
  const errObj = {
    code: 'ERR_OUT_OF_RANGE',
    message: /The value of "pid" is out of range\./
  };
  assert.throws(() => {
    os.setPriority(pid, PRIORITY_NORMAL);
  }, errObj);
  assert.throws(() => {
    os.getPriority(pid);
  }, errObj);
});

[null, true, false, 'foo', {}, [], /x/].forEach((priority) => {
  assert.throws(() => {
    os.setPriority(0, priority);
  }, {
    code: 'ERR_INVALID_ARG_TYPE',
    message: /The "priority" argument must be of type number\./
  });
});

[
  NaN,
  Infinity,
  -Infinity,
  3.14,
  2 ** 32,
  PRIORITY_HIGHEST - 1,
  PRIORITY_LOW + 1,
].forEach((priority) => {
  assert.throws(() => {
    os.setPriority(0, priority);
  }, {
    code: 'ERR_OUT_OF_RANGE',
    message: /The value of "priority" is out of range\./
  });
});

for (let i = PRIORITY_HIGHEST; i <= PRIORITY_LOW; i++) {
  try {
    os.setPriority(0, i);
  } catch (err) {
    if (err.info && err.info.code === 'EACCES') continue;
    assert(err);
  }
  const priority = os.getPriority(0);
  assert.strictEqual(typeof priority, 'number');
}

assert.throws(() => { os.getPriority(-1); }, {
  code: 'ERR_SYSTEM_ERROR',
  message: /A system error occurred: uv_os_getpriority returned /,
  name: 'SystemError'
});
