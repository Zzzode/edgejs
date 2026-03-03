'use strict';

const fs = require('fs');
const path = require('path');

const testRoot = process.env.NODE_TEST_DIR
  ? fs.realpathSync(process.env.NODE_TEST_DIR)
  : path.resolve(__dirname, '..');

const tmpdirName = '.tmp.' + (process.env.TEST_SERIAL_ID || process.env.TEST_THREAD_ID || '0');
let tmpPath = path.join(testRoot, tmpdirName);

function refresh() {
  try {
    fs.rmSync(tmpPath, { maxRetries: 3, recursive: true, force: true });
  } catch (_) {}
  fs.mkdirSync(tmpPath, { recursive: true });
}

function resolve(...paths) {
  return path.resolve(tmpPath, ...paths);
}

function hasEnoughSpace() {
  return true;
}

function fileURL(...paths) {
  const fullPath = path.resolve(tmpPath + '/', ...paths);
  const href = 'file://' + (fullPath.indexOf('/') === 0 ? fullPath : '/' + fullPath);
  return { href };
}

module.exports = {
  fileURL,
  hasEnoughSpace,
  refresh,
  resolve,
  get path() {
    return tmpPath;
  },
  set path(newPath) {
    tmpPath = path.resolve(newPath);
  },
};
