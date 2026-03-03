'use strict';

class PerformanceObserverEntryList {
  constructor(entries) {
    this._entries = entries || [];
  }

  getEntries() {
    return this._entries.slice();
  }
}

class PerformanceObserver {
  constructor(callback) {
    this._callback = callback;
    this._types = new Set();
  }

  observe(options) {
    const type = options && options.type;
    if (!type) return;
    if (!globalThis.__ubi_perf_observers) globalThis.__ubi_perf_observers = new Map();
    if (!globalThis.__ubi_perf_observers.has(type)) globalThis.__ubi_perf_observers.set(type, new Set());
    globalThis.__ubi_perf_observers.get(type).add(this);
    this._types.add(type);
  }

  disconnect() {
    if (!globalThis.__ubi_perf_observers) return;
    for (const type of this._types) {
      const set = globalThis.__ubi_perf_observers.get(type);
      if (set) set.delete(this);
    }
    this._types.clear();
  }

  _push(entry) {
    if (typeof this._callback === 'function') {
      this._callback(new PerformanceObserverEntryList([entry]));
    }
  }
}

module.exports = {
  PerformanceObserver,
  performance: {
    now() {
      return Date.now();
    },
  },
};
