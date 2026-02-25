'use strict';

const assert = require('assert');
const {
  totalmem,
  freemem,
  availableParallelism,
} = require('os');

function testFastOs() {
  assert.strictEqual(typeof totalmem(), 'number');
  assert.strictEqual(typeof freemem(), 'number');
  assert.strictEqual(typeof availableParallelism(), 'number');
}

testFastOs();
testFastOs();
