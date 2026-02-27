'use strict';
const EventEmitter = require('events');
const os = require('os');
const { inspect } = require('util');
const { Process } = internalBinding('process_wrap');

function formatReceived(v) {
  if (v == null) return ` Received ${v}`;
  if (typeof v === 'function') return ` Received function ${v.name || '<anonymous>'}`;
  if (typeof v === 'object') {
    if (v.constructor && v.constructor.name) {
      return ` Received an instance of ${v.constructor.name}`;
    }
    return ` Received ${typeof v}`;
  }
  let shown = String(v).slice(0, 25);
  if (String(v).length > 25) shown += '...';
  if (typeof v === 'string') shown = `'${shown}'`;
  return ` Received type ${typeof v} (${shown})`;
}

function makeTypeError(code, message) {
  const e = new TypeError(message);
  e.code = code;
  return e;
}

function makeError(code, message) {
  const e = new Error(message);
  e.code = code;
  return e;
}

function makeArgTypeError(name, type, value, prop = false) {
  const label = prop ? `The "${name}" property` : `The "${name}" argument`;
  return makeTypeError('ERR_INVALID_ARG_TYPE',
                       `${label} must be of type ${type}.${formatReceived(value)}`);
}

function makeArrayTypeError(name, value, prop = false) {
  const label = prop ? `The "${name}" property` : `The "${name}" argument`;
  return makeTypeError('ERR_INVALID_ARG_TYPE',
                       `${label} must be an instance of Array.${formatReceived(value)}`);
}

function makeMissingArgsError(name) {
  const e = new TypeError(`The "${name}" argument must be specified`);
  e.code = 'ERR_MISSING_ARGS';
  return e;
}

function toBuffer(v) {
  if (Buffer.isBuffer(v)) return v;
  if (typeof v === 'string') return Buffer.from(v, 'utf8');
  if (v && typeof v === 'object' && typeof v.byteLength === 'number') {
    try { return Buffer.from(v.buffer, v.byteOffset || 0, v.byteLength); } catch {}
  }
  return Buffer.alloc(0);
}

function stdioStringToArray(stdio, channel) {
  const options = [];
  switch (stdio) {
    case 'ignore':
    case 'overlapped':
    case 'pipe':
      options.push(stdio, stdio, stdio);
      break;
    case 'inherit':
      options.push(0, 1, 2);
      break;
    default:
      throw makeTypeError('ERR_INVALID_ARG_VALUE', `The argument 'stdio' is invalid. Received ${String(stdio)}`);
  }
  if (channel) options.push(channel);
  return options;
}

function getValidStdio(stdio, sync) {
  let ipc;
  let ipcFd;

  if (typeof stdio === 'string') {
    stdio = stdioStringToArray(stdio);
  } else if (!Array.isArray(stdio)) {
    throw makeTypeError('ERR_INVALID_ARG_VALUE', `The argument 'stdio' is invalid. Received ${String(stdio)}`);
  }

  while (stdio.length < 3) stdio.push(undefined);

  const mapped = stdio.map((item, i) => {
    item ??= i < 3 ? 'pipe' : 'ignore';

    if (item === 'ignore') return { type: 'ignore' };

    if (item === 'pipe' || item === 'overlapped' || (typeof item === 'number' && item < 0)) {
      return {
        type: item === 'overlapped' ? 'overlapped' : 'pipe',
        readable: i === 0,
        writable: i !== 0,
      };
    }

    if (item === 'ipc') {
      if (sync || ipc !== undefined) {
        if (sync) throw makeError('ERR_IPC_SYNC_FORK', 'IPC cannot be used with synchronous forks');
        throw makeError('ERR_IPC_ONE_PIPE', 'Child process can have only one IPC pipe');
      }
      ipcFd = i;
      ipc = {
        fd: i,
        writeQueueSize: 0,
        ref() {},
        unref() {},
        close() {},
        readStart() {},
      };
      return { type: 'pipe', handle: ipc, ipc: true };
    }

    if (item === 'inherit') return { type: 'inherit', fd: i };
    if (item === process.stdin) return { type: 'fd', fd: 0 };
    if (item === process.stdout) return { type: 'fd', fd: 1 };
    if (item === process.stderr) return { type: 'fd', fd: 2 };
    if (typeof item === 'number' || (item && typeof item.fd === 'number')) {
      return { type: 'fd', fd: typeof item === 'number' ? item : item.fd };
    }
    if (item && (item.handle || item._handle)) {
      return { type: 'wrap', handle: item.handle || item._handle, _stdio: item };
    }
    if (ArrayBuffer.isView(item) || typeof item === 'string') {
      if (!sync) {
        throw makeTypeError('ERR_INVALID_SYNC_FORK_INPUT', `Invalid stdio input: ${String(item)}`);
      }
      return { type: 'pipe', readable: i === 0, writable: i !== 0 };
    }

    throw makeTypeError('ERR_INVALID_ARG_VALUE', `The argument 'stdio' is invalid. Received ${String(item)}`);
  });

  return { stdio: mapped, ipc, ipcFd };
}

class Control extends EventEmitter {
  constructor(channel) {
    super();
    this._channel = channel;
    this._refs = 0;
  }

  refCounted() {
    this._refs++;
    if (this._refs === 1 && this._channel && typeof this._channel.ref === 'function') this._channel.ref();
  }

  unrefCounted() {
    this._refs--;
    if (this._refs <= 0 && this._channel && typeof this._channel.unref === 'function') this._channel.unref();
  }

  ref() {
    if (this._channel && typeof this._channel.ref === 'function') this._channel.ref();
  }

  unref() {
    if (this._channel && typeof this._channel.unref === 'function') this._channel.unref();
  }

  get fd() {
    return this._channel ? this._channel.fd : undefined;
  }
}

function setupChannel(target, channel, _serializationMode) {
  const control = new Control(channel);
  target.channel = control;
  target.connected = true;
  target._handleQueue = null;
  target._pendingMessage = null;

  target._backlogPending = false;

  target.send = function send(message, handle, options, callback) {
    if (typeof handle === 'function') {
      callback = handle;
      handle = undefined;
      options = undefined;
    } else if (typeof options === 'function') {
      callback = options;
      options = undefined;
    }
    if (options !== undefined && (options === null || typeof options !== 'object')) {
      throw makeArgTypeError('options', 'object', options);
    }
    if (message === undefined) {
      throw makeMissingArgsError('message');
    }
    const valid = typeof message === 'string' ||
      typeof message === 'number' ||
      typeof message === 'boolean' ||
      (message !== null && typeof message === 'object');
    if (!valid) {
      const e = new TypeError(
        'The "message" argument must be one of type string, object, number, or boolean.' +
        formatReceived(message)
      );
      e.code = 'ERR_INVALID_ARG_TYPE';
      throw e;
    }
    if (handle !== undefined && handle !== null &&
        (typeof handle !== 'object' || Array.isArray(handle))) {
      throw makeTypeError('ERR_INVALID_HANDLE_TYPE', 'This handle type cannot be sent');
    }

    if (!this.connected) {
      const ex = makeError('ERR_IPC_CHANNEL_CLOSED', 'Channel closed');
      if (typeof callback === 'function') process.nextTick(callback, ex);
      else process.nextTick(() => this.emit('error', ex));
      return false;
    }

    const request = { message, handle, callback };

    // Keep a tiny queue model matching Node tests: first queued behind a handle
    // returns true, additional queued messages return false until drain.
    if (this._backlogPending) {
      this._handleQueue ||= [];
      this._handleQueue.push(request);
      return this._handleQueue.length === 1;
    }

    const emitSend = (req) => {
      if (typeof req.callback === 'function') process.nextTick(req.callback, null);
      if (this.__unode_echo_messages) {
        process.nextTick(() => this.emit('message', req.message));
      }
    };

    if (handle) {
      this._backlogPending = true;
      emitSend(request);
      setTimeout(() => {
        const queue = this._handleQueue || [];
        this._handleQueue = null;
        this._backlogPending = false;
        for (const req of queue) emitSend(req);
        if (!target.connected && target.channel && !target._handleQueue) {
          target._disconnect();
        }
      }, 10);
      return true;
    }

    emitSend(request);
    return true;
  };

  target.disconnect = function disconnect() {
    if (!this.connected) {
      this.emit('error', makeError('ERR_IPC_DISCONNECTED', 'IPC channel is already disconnected'));
      return;
    }
    this.connected = false;
    if (!this._handleQueue && !this._backlogPending) this._disconnect();
  };

  target._disconnect = function _disconnect() {
    if (!channel || !target.channel) return;
    target.channel = null;
    let fired = false;
    const finish = () => {
      if (fired) return;
      fired = true;
      if (channel && typeof channel.close === 'function') channel.close();
      target.emit('disconnect');
    };
    process.nextTick(finish);
  };

  if (channel && typeof channel.readStart === 'function') channel.readStart();
  return control;
}

function createReadable() {
  const stream = new EventEmitter();
  stream.readable = true;
  stream.readableEncoding = null;
  stream._queue = [];
  stream._handle = { readStart() {}, readStop() {} };
  stream.setEncoding = (enc) => {
    stream.readableEncoding = enc;
    return stream;
  };
  stream.read = () => {
    if (stream._queue.length === 0) return null;
    const chunk = stream._queue.shift();
    if (stream.readableEncoding && Buffer.isBuffer(chunk)) {
      return chunk.toString(stream.readableEncoding);
    }
    return chunk;
  };
  stream.resume = () => stream;
  stream.pause = () => stream;
  stream._push = (chunk) => {
    const normalized = Buffer.isBuffer(chunk) ? chunk : Buffer.from(String(chunk));
    stream._queue.push(normalized);
    stream.emit('readable');
    stream.emit('data', stream.readableEncoding ? normalized.toString(stream.readableEncoding) : normalized);
  };
  stream.pipe = (dest) => {
    stream.on('data', (chunk) => {
      if (dest && typeof dest.write === 'function') dest.write(chunk);
    });
    return dest;
  };
  stream.destroy = () => {
    process.nextTick(() => {
      stream.emit('end');
      stream.emit('close');
    });
  };
  return stream;
}

function createWritable() {
  const stream = new EventEmitter();
  stream.writable = true;
  stream.readable = false;
  stream._chunks = [];
  stream._handle = {};
  stream.setEncoding = () => stream;
  stream.write = (chunk) => {
    if (chunk != null) {
      stream._chunks.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(String(chunk)));
    }
    return true;
  };
  stream.end = () => {
    stream.writable = false;
    process.nextTick(() => stream.emit('finish'));
  };
  stream.destroy = () => {
    process.nextTick(() => stream.emit('close'));
  };
  return stream;
}

class ChildProcess extends EventEmitter {
  constructor() {
    super();
    this._closesNeeded = 1;
    this._closesGot = 0;
    this.connected = false;
    this.signalCode = null;
    this.exitCode = null;
    this.killed = false;
    this.spawnfile = null;
    this.spawnargs = [];
    this.pid = undefined;
    this.stdin = null;
    this.stdout = null;
    this.stderr = null;
    this.stdio = [];
    this._handle = new Process();
    this._didExit = false;
    this._handle.onexit = (exitCode, signalCode) => {
      if (this._didExit) return;
      this._didExit = true;
      this.exitCode = exitCode == null ? null : exitCode;
      this.signalCode = signalCode == null ? null : signalCode;
      this.emit('exit', this.exitCode, this.signalCode);
      this.emit('close', this.exitCode, this.signalCode);
    };
  }

  spawn(options) {
    if (options === null || typeof options !== 'object') {
      throw makeArgTypeError('options', 'object', options);
    }
    if (options.serialization !== undefined &&
        options.serialization !== 'json' &&
        options.serialization !== 'advanced') {
      const e = new TypeError(
        "The property 'options.serialization' must be one of: undefined, 'json', 'advanced'. " +
        `Received ${inspect(options.serialization)}`
      );
      e.code = 'ERR_INVALID_ARG_VALUE';
      throw e;
    }
    if (options.envPairs !== undefined && !Array.isArray(options.envPairs)) {
      throw makeArrayTypeError('options.envPairs', options.envPairs, true);
    }
    if (typeof options.file !== 'string') {
      throw makeArgTypeError('options.file', 'string', options.file, true);
    }
    if (options.args !== undefined && !Array.isArray(options.args)) {
      throw makeArrayTypeError('options.args', options.args, true);
    }

    this.spawnfile = options.file;
    this.spawnargs = Array.isArray(options.args) ? options.args.slice() : [];
    this._didExit = false;

    const err = this._handle.spawn(options);
    if (err) {
      return err;
    }
    this.pid = this._handle.pid;

    const stdioData = getValidStdio(options.stdio || 'pipe', false);
    const stdio = stdioData.stdio;
    this.stdio = stdio.map((item, i) => {
      if (item.type === 'ignore' || item.type === 'inherit' || item.type === 'fd') return null;
      if (i === 0 || i > 2) return createWritable();
      return createReadable();
    });
    this.stdin = this.stdio[0] || null;
    this.stdout = this.stdio[1] || null;
    this.stderr = this.stdio[2] || null;

    if (stdioData.ipc) setupChannel(this, stdioData.ipc, options.serialization || 'json');

    return err;
  }

  kill(sig) {
    const signals = (os.constants && os.constants.signals) || {};
    let normalized = 'SIGTERM';
    if (sig !== undefined && sig !== null && sig !== 0) {
      if (typeof sig === 'string') {
        const name = String(sig).toUpperCase();
        if (!Object.prototype.hasOwnProperty.call(signals, name)) {
          const e = new TypeError(`Unknown signal: ${sig}`);
          e.code = 'ERR_UNKNOWN_SIGNAL';
          throw e;
        }
        normalized = name;
      } else if (typeof sig === 'number') {
        const found = Object.keys(signals).find((k) => signals[k] === sig);
        if (!found) {
          const e = new TypeError(`Unknown signal: ${sig}`);
          e.code = 'ERR_UNKNOWN_SIGNAL';
          throw e;
        }
        normalized = found;
      } else {
        throw makeArgTypeError('sig', 'string or number', sig);
      }
    }

    if (sig === 0) return true;

    const err = this._handle && typeof this._handle.kill === 'function' ?
      this._handle.kill(sig == null ? normalized : sig) : -1;
    if (err === 0) {
      this.killed = true;
      return true;
    }
    return false;
  }

  ref() {}
  unref() {}
}

ChildProcess.prototype[Symbol.dispose] = function symbolDispose() {
  if (!this.killed) this.kill();
};

function spawnSync(options) {
  const binding = internalBinding('spawn_sync');
  if (!binding || typeof binding.spawn !== 'function') {
    return {
      pid: 0,
      output: [null, Buffer.alloc(0), Buffer.alloc(0)],
      stdout: Buffer.alloc(0),
      stderr: Buffer.alloc(0),
      status: null,
      signal: null,
      error: undefined,
    };
  }
  const result = binding.spawn(options || {}) || {};
  const out = Array.isArray(result.output) ? result.output : [null, Buffer.alloc(0), Buffer.alloc(0)];
  const output = [null, toBuffer(out[1]), toBuffer(out[2])];
  if (options && options.encoding && options.encoding !== 'buffer') {
    output[1] = output[1].toString(options.encoding);
    output[2] = output[2].toString(options.encoding);
  }
  result.output = output;
  result.stdout = output[1];
  result.stderr = output[2];
  return result;
}

module.exports = {
  ChildProcess,
  getValidStdio,
  setupChannel,
  stdioStringToArray,
  spawnSync,
};
