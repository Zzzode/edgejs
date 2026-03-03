'use strict';

const { isIP } = require('net');

const mockedErrorCode = 'ENOTFOUND';
const mockedSysCall = 'getaddrinfo';

function errorLookupMock(code = mockedErrorCode, syscall = mockedSysCall) {
  return function lookupWithError(hostname, _dnsopts, cb) {
    const err = new Error(`${syscall} ${code} ${hostname}`);
    err.code = code;
    err.errno = code;
    err.syscall = syscall;
    err.hostname = hostname;
    cb(err);
  };
}

function createMockedLookup(...addresses) {
  const normalized = addresses.map((address) => ({ address, family: isIP(address) }));
  return function lookup(_hostname, options, cb) {
    if (options && options.all === true) return process.nextTick(() => cb(null, normalized));
    const first = normalized[0] || { address: '127.0.0.1', family: 4 };
    process.nextTick(() => cb(null, first.address, first.family));
  };
}

module.exports = {
  errorLookupMock,
  mockedErrorCode,
  mockedSysCall,
  createMockedLookup,
};
