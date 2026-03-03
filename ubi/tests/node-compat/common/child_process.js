'use strict';

const assert = require('assert');
const { spawnSync, execFileSync } = require('child_process');
const common = require('./');
const util = require('util');

function cleanupStaleProcess(_filename) {
  // Keep API parity with Node's test helper; only needed on Windows.
  if (!common.isWindows) return;
}

const kExpiringChildRunTime = common.platformTimeout(20 * 1000);
const kExpiringParentTimer = 1;

function logAfterTime(time) {
  setTimeout(() => {
    console.log('child stdout');
    console.error('child stderr');
  }, time);
}

function checkOutput(str, check) {
  if ((check instanceof RegExp && !check.test(str)) ||
    (typeof check === 'string' && check !== str)) {
    return { passed: false, reason: `did not match ${util.inspect(check)}` };
  }
  if (typeof check === 'function') {
    try {
      check(str);
    } catch (error) {
      return {
        passed: false,
        reason: `did not match expectation, checker throws:\n${util.inspect(error)}`,
      };
    }
  }
  return { passed: true };
}

function expectSyncExit(caller, spawnArgs, {
  status,
  signal,
  stderr: stderrCheck,
  stdout: stdoutCheck,
  trim = false,
}) {
  const child = spawnSync(...spawnArgs);
  const failures = [];
  let stderrStr;
  let stdoutStr;

  if (status !== undefined && child.status !== status) {
    failures.push(`- process terminated with status ${child.status}, expected ${status}`);
  }
  if (signal !== undefined && child.signal !== signal) {
    failures.push(`- process terminated with signal ${child.signal}, expected ${signal}`);
  }

  function logAndThrow() {
    const error = new Error(failures.join('\n'));
    let command = spawnArgs[0];
    if (Array.isArray(spawnArgs[1])) command += ` ${spawnArgs[1].join(' ')}`;
    error.command = command;
    Error.captureStackTrace(error, caller);
    throw error;
  }

  if (failures.length !== 0) logAndThrow();

  if (stderrCheck !== undefined) {
    stderrStr = child.stderr.toString();
    const { passed, reason } = checkOutput(trim ? stderrStr.trim() : stderrStr, stderrCheck);
    if (!passed) failures.push(`- stderr ${reason}`);
  }
  if (stdoutCheck !== undefined) {
    stdoutStr = child.stdout.toString();
    const { passed, reason } = checkOutput(trim ? stdoutStr.trim() : stdoutStr, stdoutCheck);
    if (!passed) failures.push(`- stdout ${reason}`);
  }

  if (failures.length !== 0) logAndThrow();
  return { child, stderr: stderrStr, stdout: stdoutStr };
}

function spawnSyncAndExit(...args) {
  const spawnArgs = args.slice(0, args.length - 1);
  const expectations = args[args.length - 1];
  return expectSyncExit(spawnSyncAndExit, spawnArgs, expectations);
}

function spawnSyncAndExitWithoutError(...args) {
  return expectSyncExit(spawnSyncAndExitWithoutError, [...args], {
    status: 0,
    signal: null,
  });
}

function spawnSyncAndAssert(...args) {
  const expectations = args.pop();
  return expectSyncExit(spawnSyncAndAssert, [...args], {
    status: 0,
    signal: null,
    ...expectations,
  });
}

module.exports = {
  cleanupStaleProcess,
  logAfterTime,
  kExpiringChildRunTime,
  kExpiringParentTimer,
  spawnSyncAndAssert,
  spawnSyncAndExit,
  spawnSyncAndExitWithoutError,
};
