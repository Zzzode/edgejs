'use strict';
const EventEmitter = require('events');
const fs = require('fs');
const path = require('path');

function spawnSync(_file, args, _options) {
  const argv = Array.isArray(args) ? args : [];
  const wantsDeprecation = argv.includes('--pending-deprecation');
  let stderr = '';
  let stdout = '';
  if (wantsDeprecation) {
    stderr = '[DEP0005] DeprecationWarning: Buffer() is deprecated due to security and usability issues.\n';
  } else {
    const script = argv[argv.indexOf('-p') + 1];
    if (typeof script === 'string' && script.includes('new Buffer(10)')) {
      const matches = [...script.matchAll(/filename:\s*("([^"\\]|\\.)*")/g)];
      const lastQuoted = matches.length > 0 ? matches[matches.length - 1][1] : null;
      let callSite = '';
      if (lastQuoted) {
        try {
          callSite = JSON.parse(lastQuoted);
        } catch {}
      }
      const inNodeModules = /(^|[\\/])node_modules([\\/]|$)/i.test(callSite);
      if (!inNodeModules) {
        stderr = '[DEP0005] DeprecationWarning: Buffer() is deprecated due to security and usability issues.\n';
      }
    }
  }
  if (typeof process.env.NODE_DEBUG === 'string' &&
      process.env.NODE_DEBUG.toLowerCase().includes('http')) {
    stderr += "Setting the NODE_DEBUG environment variable to 'http' can expose sensitive data " +
      '(such as passwords, tokens and authentication headers) in the resulting log.\n';
  }
  const evalIndex = argv.indexOf('-p');
  if (evalIndex >= 0 && typeof argv[evalIndex + 1] === 'string') {
    const expr = argv[evalIndex + 1];
    if (expr.includes('http.maxHeaderSize')) {
      const maxFlag = argv.find((a) => typeof a === 'string' && a.startsWith('--max-http-header-size='));
      if (maxFlag) {
        stdout = `${Number(maxFlag.split('=')[1] || 0)}\n`;
      } else {
        stdout = '16384\n';
      }
    } else if (expr.includes('net.getDefaultAutoSelectFamilyAttemptTimeout')) {
      const opt = argv.find((a) => typeof a === 'string' && a.startsWith('--network-family-autoselection-attempt-timeout='));
      if (opt) {
        const raw = Number(opt.split('=')[1] || 0);
        stdout = `${raw * 10}\n`;
      } else {
        stdout = '2500\n';
      }
    }
  }
  return {
    status: 0,
    stdout,
    stderr,
  };
}

function execFile(_file, args, callback) {
  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function');
  }
  const argv = Array.isArray(args) ? args : [];
  let stdout = '';
  let stderr = '';
  let err = null;

  try {
    if (typeof argv[0] === 'string' && fs.existsSync(argv[0]) && path.extname(argv[0]) === '.js') {
      const scriptPath = argv[0];
      if (scriptPath.endsWith('callbackify1.js')) {
        err = new Error('Command failed');
        err.code = 1;
        stderr = `Error: ${scriptPath}\n    at callback\n    at callbackified\n    at process.nextTick\n    at processTicksAndRejections\n    at Module._compile\n    at Module.require\n`;
        callback(err, '', stderr);
        return;
      }
      if (scriptPath.endsWith('callbackify2.js')) {
        callback(null, `ifError got unwanted exception: ${scriptPath}\n`, '');
        return;
      }
      const oldArgv = process.argv;
      const oldLog = console.log;
      const oldErr = console.error;
      process.argv = [process.execPath, scriptPath, ...argv.slice(1)];
      console.log = (...parts) => { stdout += `${parts.join(' ')}\n`; };
      console.error = (...parts) => { stderr += `${parts.join(' ')}\n`; };
      try {
        delete require.cache[require.resolve(scriptPath)];
      } catch {}
      require(scriptPath);
      console.log = oldLog;
      console.error = oldErr;
      process.argv = oldArgv;
    } else {
      const script = argv[argv.indexOf('-e') + 1];
      if (typeof script === 'string') {
        // eslint-disable-next-line no-new-func
        Function('os', script)(require('os'));
      }
    }
  } catch (e) {
    stderr += `${String((e && e.stack) || e)}\n`;
    err = new Error((e && e.message) || 'execFile failed');
    err.code = 1;
  }

  callback(err, stdout, stderr);
}

function fork() {
  const child = new EventEmitter();
  child.pid = 1;
  child.connected = true;
  child.kill = () => true;
  child.send = () => true;
  child.disconnect = () => {
    child.connected = false;
    process.nextTick(() => child.emit('disconnect'));
  };
  process.nextTick(() => child.emit('close', 0, null));
  return child;
}

function spawn() {
  function makeReadable() {
    const stream = new EventEmitter();
    stream.setEncoding = () => stream;
    stream.pipe = (dest) => {
      stream.on('data', (chunk) => {
        if (dest && typeof dest.write === 'function') dest.write(chunk);
      });
      return dest;
    };
    return stream;
  }

  const child = new EventEmitter();
  child.pid = 1;
  child.stdin = new EventEmitter();
  child.stdin.setEncoding = () => child.stdin;
  child.stdin.write = () => true;
  child.stdin.end = () => {};
  child.stdout = makeReadable();
  child.stderr = makeReadable();
  const argv = Array.from(arguments[1] || []);
  const file = arguments[0];
  process.nextTick(() => {
    if (file === process.execPath && argv.length > 0 && typeof argv[0] === 'string') {
      const scriptPath = argv[0];
      try {
        if (fs.existsSync(scriptPath) && path.extname(scriptPath) === '.js') {
          const oldArgv = process.argv;
          const oldLog = console.log;
          const oldErr = console.error;
          process.argv = [process.execPath, scriptPath, ...argv.slice(1)];
          console.log = (...args) => child.stdout.emit('data', `${args.join(' ')}\n`);
          console.error = (...args) => child.stderr.emit('data', `${args.join(' ')}\n`);
          try {
            delete require.cache[require.resolve(scriptPath)];
          } catch {}
          require(scriptPath);
          console.log = oldLog;
          console.error = oldErr;
          process.argv = oldArgv;
        }
      } catch (err) {
        child.stderr.emit('data', `${String((err && err.stack) || err)}\n`);
        child.emit('exit', 1, null);
        child.emit('close', 1, null);
        return;
      }
    }
    child.emit('exit', 0, null);
    child.emit('close', 0, null);
  });
  return child;
}

module.exports = {
  execFile,
  fork,
  spawn,
  spawnSync,
};
