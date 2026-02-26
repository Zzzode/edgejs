'use strict';

const path = require('path');
const { internalBinding } = require('internal/test/binding_runtime');

module.exports = require(path.resolve(__dirname, '../../../node/lib/string_decoder.js'));
