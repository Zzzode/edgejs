'use strict';

const common = require('../common');
const assert = require('assert');
const os = require('os');
const path = require('path');

if (common.isWindows) delete process.env.USERPROFILE;
else delete process.env.HOME;

const home = os.homedir();
assert.strictEqual(typeof home, 'string');
assert(home.includes(path.sep));
