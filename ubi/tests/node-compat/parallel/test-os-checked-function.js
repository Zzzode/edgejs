'use strict';

const assert = require('assert');

const binding = globalThis.__ubi_os;
const originalGetHomeDirectory = binding.getHomeDirectory;
binding.getHomeDirectory = function(ctx) {
  ctx.syscall = 'foo';
  ctx.code = 'bar';
  ctx.message = 'baz';
};

const resolved = require.resolve('os');
delete require.cache[resolved];
const os = require('os');

assert.throws(os.homedir, {
  message: /^A system error occurred: foo returned bar \(baz\)$/
});

binding.getHomeDirectory = originalGetHomeDirectory;
