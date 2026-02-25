'use strict';

function test(_name, fn) {
  if (typeof fn === 'function') fn();
}

module.exports = {
  test,
};
