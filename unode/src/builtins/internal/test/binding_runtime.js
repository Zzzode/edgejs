'use strict';

const UV_ENOENT = -2;
const UV_EEXIST = -17;
const UV_EOF = -4095;
const UV_EINVAL = -22;
const UV_EBADF = -9;
const UV_ENOTCONN = -57;
const UV_ECANCELED = -89;
const UV_ETIMEDOUT = -60;
const UV_ENOMEM = -12;
const UV_ENOTSOCK = -88;
const UV_UNKNOWN = -4094;
const UV_EAI_MEMORY = -3001;
const kHasBackingStore = new WeakSet();
const kImmediateInfo = new Int32Array(3);
const kTimeoutInfo = new Int32Array(1);
const kTickInfo = new Int32Array(1);
const kIsBuildingSnapshotBuffer = new Uint8Array([0]);
const basePrimordials = require('../util/primordials');

function uncurryThis(fn) {
  return Function.call.bind(fn);
}

const primordials = {
  ...basePrimordials,
  Array,
  ArrayBufferIsView: ArrayBuffer.isView,
  ArrayIsArray: Array.isArray,
  ArrayPrototypeForEach: uncurryThis(Array.prototype.forEach),
  BigInt,
  Date,
  DateNow: Date.now,
  Float32Array,
  Float64Array,
  MathFloor: Math.floor,
  MathMin: Math.min,
  MathTrunc: Math.trunc,
  Number,
  NumberParseInt: Number.parseInt,
  NumberIsInteger: Number.isInteger,
  NumberIsNaN: Number.isNaN,
  NumberIsFinite: Number.isFinite,
  NumberMAX_SAFE_INTEGER: Number.MAX_SAFE_INTEGER,
  NumberMIN_SAFE_INTEGER: Number.MIN_SAFE_INTEGER,
  ObjectDefineProperties: Object.defineProperties,
  ObjectDefineProperty: Object.defineProperty,
  ObjectGetOwnPropertyDescriptor: Object.getOwnPropertyDescriptor,
  ObjectPrototypeHasOwnProperty: uncurryThis(Object.prototype.hasOwnProperty),
  ObjectSetPrototypeOf: Object.setPrototypeOf,
  RegExp,
  RegExpPrototypeTest: uncurryThis(RegExp.prototype.test),
  RegExpPrototypeSymbolReplace: uncurryThis(RegExp.prototype[Symbol.replace]),
  StringPrototypeCharCodeAt: uncurryThis(String.prototype.charCodeAt),
  StringPrototypeSlice: uncurryThis(String.prototype.slice),
  StringPrototypeToLowerCase: uncurryThis(String.prototype.toLowerCase),
  StringPrototypeTrim: uncurryThis(String.prototype.trim),
  Symbol,
  SymbolFor: Symbol.for,
  SymbolSpecies: Symbol.species,
  SymbolToPrimitive: Symbol.toPrimitive,
  TypedArrayPrototypeFill: uncurryThis(Uint8Array.prototype.fill),
  TypedArrayPrototypeGetBuffer: (ta) => ta.buffer,
  TypedArrayPrototypeGetByteLength: (ta) => ta.byteLength,
  TypedArrayPrototypeGetByteOffset: (ta) => ta.byteOffset,
  TypedArrayPrototypeGetLength: (ta) => ta.length,
  TypedArrayPrototypeSet: uncurryThis(Uint8Array.prototype.set),
  TypedArrayPrototypeSubarray: uncurryThis(Uint8Array.prototype.subarray),
  TypedArrayPrototypeSlice: uncurryThis(Uint8Array.prototype.slice),
  Uint8Array,
  Uint8ArrayPrototype: Uint8Array.prototype,
};

const kUntransferable = Symbol('untransferable_object_private_symbol');
function getNativeInternalBinding() {
  const ib = globalThis.internalBinding;
  if (typeof ib === 'function' && ib !== internalBinding) return ib;
  return null;
}

class UnodePipeStub {}
class UnodePipeConnectWrapStub {}

function internalBinding(name) {
  if (name === 'uv') {
    return {
      UV_ENOENT,
      UV_EEXIST,
      UV_EOF,
      UV_EINVAL,
      UV_EBADF,
      UV_ENOTCONN,
      UV_ECANCELED,
      UV_ETIMEDOUT,
      UV_ENOMEM,
      UV_ENOTSOCK,
      UV_UNKNOWN,
      UV_EAI_MEMORY,
    };
  }
  if (name === 'constants') {
    return {
      os: {
        UV_UDP_REUSEADDR: 4,
      },
    };
  }
  if (name === 'timers') {
    return {
      immediateInfo: kImmediateInfo,
      timeoutInfo: kTimeoutInfo,
      toggleTimerRef() {},
      toggleImmediateRef() {},
      scheduleTimer() {},
      getLibuvNow() {
        return Date.now();
      },
    };
  }
  if (name === 'task_queue') {
    return {
      tickInfo: kTickInfo,
      runMicrotasks() {},
      setTickCallback() {},
      enqueueMicrotask(fn) {
        if (typeof queueMicrotask === 'function') return queueMicrotask(fn);
        if (typeof fn === 'function') fn();
      },
    };
  }
  if (name === 'trace_events') {
    return {
      getCategoryEnabledBuffer() {
        return new Uint8Array(1);
      },
      trace() {},
    };
  }
  if (name === 'mksnapshot') {
    return {
      setSerializeCallback() {},
      setDeserializeCallback() {},
      setDeserializeMainFunction() {},
      isBuildingSnapshotBuffer: kIsBuildingSnapshotBuffer,
    };
  }
  if (name === 'errors') {
    return {
      noSideEffectsToString(value) {
        return String(value);
      },
      triggerUncaughtException(err) {
        throw err;
      },
      exitCodes: {
        kNoFailure: 0,
        kGenericUserError: 1,
      },
    };
  }
  if (name === 'symbols') {
    const syms = {
      async_id_symbol: Symbol('async_id_symbol'),
      owner_symbol: Symbol('owner_symbol'),
      resource_symbol: Symbol('resource_symbol'),
      trigger_async_id_symbol: Symbol('trigger_async_id_symbol'),
    };
    return {
      ...syms,
      symbols: syms,
    };
  }
  if (name === 'os') return globalThis.__unode_os || {};
  if (name === 'buffer') return globalThis.__unode_buffer || {};
  if (name === 'http_parser') return globalThis.__unode_http_parser || {};
  if (name === 'stream_wrap') return globalThis.__unode_stream_wrap || {};
  if (name === 'js_stream') {
    class JSStream {
      close(cb) {
        if (typeof cb === 'function') cb();
      }
      readBuffer() {}
      emitEOF() {}
      finishWrite(req, status) {
        if (req && typeof req.oncomplete === 'function') req.oncomplete(status || 0, this, req);
      }
      finishShutdown(req, status) {
        if (req && typeof req.oncomplete === 'function') req.oncomplete(status || 0, this, req);
      }
    }
    return { JSStream };
  }
  if (name === 'tcp_wrap') return globalThis.__unode_tcp_wrap || {};
  if (name === 'udp_wrap') return globalThis.__unode_udp_wrap || {};
  if (name === 'pipe_wrap') {
    if (globalThis.__unode_pipe_wrap) return globalThis.__unode_pipe_wrap;
    return {
      Pipe: UnodePipeStub,
      PipeConnectWrap: UnodePipeConnectWrapStub,
      constants: {
        SOCKET: 0,
        SERVER: 1,
      },
    };
  }
  if (name === 'cares_wrap') {
    if (globalThis.__unode_cares_wrap) return globalThis.__unode_cares_wrap;
    return {
      convertIpv6StringToBuffer() {
        return null;
      },
    };
  }
  if (name === 'string_decoder') {
    if (globalThis.__unode_string_decoder) return globalThis.__unode_string_decoder;
    const nativeInternalBinding = getNativeInternalBinding();
    if (nativeInternalBinding) return nativeInternalBinding(name);
    return {};
  }
  if (name === 'url') return globalThis.__unode_url || {};
  if (name === 'url_pattern') {
    return {
      URLPattern: typeof globalThis.URLPattern === 'function' ? globalThis.URLPattern : undefined,
    };
  }
  if (name === 'encoding_binding') {
    return {
      toASCII(input) {
        const b = globalThis.__unode_url || {};
        if (typeof b.domainToASCII === 'function') return b.domainToASCII(String(input));
        return String(input || '').toLowerCase();
      },
    };
  }
  if (name === 'debug') {
    return {
      getV8FastApiCallCount() {
        return 0;
      },
    };
  }
  if (name === 'util') {
    return {
      constants: {
        ALL_PROPERTIES: 0,
        ONLY_ENUMERABLE: 1,
      },
      getOwnNonIndexProperties(obj) {
        return Object.getOwnPropertyNames(obj).filter((n) => {
          const index = Number(n);
          return !Number.isInteger(index) || String(index) !== n;
        });
      },
      getCallSites() {
        return [];
      },
      isInsideNodeModules() {
        return false;
      },
      privateSymbols: {
        untransferable_object_private_symbol: kUntransferable,
      },
      arrayBufferViewHasBuffer(view) {
        if (view == null || typeof view !== 'object') return false;
        let byteLength = 0;
        try {
          byteLength = view.byteLength;
        } catch {
          return false;
        }
        if (typeof byteLength !== 'number') return false;
        if (kHasBackingStore.has(view)) return true;
        if (byteLength >= 96) return true;
        kHasBackingStore.add(view);
        return false;
      },
    };
  }
  const nativeInternalBinding = getNativeInternalBinding();
  if (nativeInternalBinding) return nativeInternalBinding(name);
  return {};
}

module.exports = { internalBinding, primordials };
