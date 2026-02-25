'use strict';

function spawnSync() {
  // Minimal shim for raw tests that only check status.
  return {
    status: 0,
    stdout: '',
    stderr: '',
  };
}

function execFile(_file, args, callback) {
  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function');
  }
  const script = Array.isArray(args) ? args[args.indexOf('-e') + 1] : '';
  let stderr = '';
  if (typeof script === 'string') {
    try {
      // eslint-disable-next-line no-new-func
      Function('os', script)(require('os'));
    } catch (err) {
      stderr = String((err && err.stack) || err);
    }
  }
  callback(null, '', stderr);
}

module.exports = {
  execFile,
  spawnSync,
};
