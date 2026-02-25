'use strict';

const assert = require('assert');
const { constants } = require('node:os');

assert.throws(() => { constants.signals.FOOBAR = 1337; }, TypeError);
