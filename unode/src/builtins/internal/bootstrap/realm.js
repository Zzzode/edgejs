'use strict';

const fs = require('fs');
const builtinsRoot = `${__dirname}/../../../`;
const builtinCandidates = [
  (id) => `${builtinsRoot}${id}.js`,
  (id) => `${builtinsRoot}${id}/index.js`,
];

const BuiltinModule = {
  exists(id) {
    const name = String(id || '').replace(/^node:/, '');
    if (!name) return false;
    for (const makePath of builtinCandidates) {
      if (fs.existsSync(makePath(name))) return true;
    }
    return false;
  },
};

module.exports = { BuiltinModule };
