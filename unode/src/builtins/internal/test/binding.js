'use strict';

const { internalBinding, primordials } = require('./binding_runtime');

process.emitWarning(
  'These APIs are for internal testing only. Do not use them.',
  'internal/test/binding');

function filteredInternalBinding(id) {
  if (typeof id === 'string' && id.startsWith('internal_only')) {
    const error = new Error(`No such binding: ${id}`);
    error.code = 'ERR_INVALID_MODULE';
    throw error;
  }
  return internalBinding(id);
}

module.exports = { internalBinding: filteredInternalBinding, primordials };
