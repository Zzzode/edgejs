'use strict';

const assert = require('assert');
const os = require('os');

assert.throws(() => {
  os.userInfo({
    get encoding() {
      throw new Error('xyz');
    }
  });
}, /xyz/);
