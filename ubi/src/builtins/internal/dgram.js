'use strict';

const {
  FunctionPrototypeBind,
  Symbol,
  ObjectDefineProperty,
} = primordials;

const {
  codes: { ERR_SOCKET_BAD_TYPE },
} = require('internal/errors');
const { UDP } = internalBinding('udp_wrap');
const { guessHandleType } = require('internal/util');
const { isInt32, validateFunction } = require('internal/validators');
const { UV_EINVAL } = internalBinding('uv');
const kStateSymbol = Symbol('state symbol');
let dns;

function lookup4(lookup, address, callback) {
  return lookup(address || '127.0.0.1', 4, callback);
}

function lookup6(lookup, address, callback) {
  return lookup(address || '::1', 6, callback);
}

function defineOwnMethod(obj, key, fn) {
  ObjectDefineProperty(obj, key, {
    __proto__: null,
    configurable: true,
    enumerable: true,
    writable: true,
    value: fn,
  });
}

function newHandle(type, lookup) {
  if (lookup === undefined) {
    if (dns === undefined) dns = require('dns');
    lookup = dns.lookup;
  } else {
    validateFunction(lookup, 'lookup');
  }

  if (type === 'udp4') {
    const handle = new UDP();
    handle.lookup = FunctionPrototypeBind(lookup4, handle, lookup);
    return handle;
  }

  if (type === 'udp6') {
    const handle = new UDP();
    handle.lookup = FunctionPrototypeBind(lookup6, handle, lookup);
    defineOwnMethod(handle, 'bind', FunctionPrototypeBind(handle.bind6, handle));
    defineOwnMethod(handle, 'connect', FunctionPrototypeBind(handle.connect6, handle));
    defineOwnMethod(handle, 'send', FunctionPrototypeBind(handle.send6, handle));
    return handle;
  }

  throw new ERR_SOCKET_BAD_TYPE();
}

function _createSocketHandle(address, port, addressType, fd, flags) {
  const handle = newHandle(addressType);
  let err;

  if (isInt32(fd) && fd > 0) {
    const type = guessHandleType(fd);
    if (type !== 'UDP') {
      err = UV_EINVAL;
    } else {
      err = handle.open(fd);
    }
  } else if (port || address) {
    err = handle.bind(address, port || 0, flags);
  }

  if (err) {
    handle.close();
    return err;
  }
  return handle;
}

module.exports = {
  kStateSymbol,
  _createSocketHandle,
  newHandle,
};
