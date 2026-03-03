'use strict';

const active = new Map();

function observersFor(type) {
  if (!globalThis.__ubi_perf_observers) globalThis.__ubi_perf_observers = new Map();
  if (!globalThis.__ubi_perf_observers.has(type)) globalThis.__ubi_perf_observers.set(type, new Set());
  return globalThis.__ubi_perf_observers.get(type);
}

module.exports = {
  hasObserver(type) {
    return observersFor(type).size > 0;
  },
  startPerf(resource, symbol, data) {
    if (!resource || !symbol) return;
    active.set(resource, {
      symbol,
      startTime: Date.now(),
      ...(data || {}),
    });
  },
  stopPerf(resource, symbol, data) {
    const entry = active.get(resource);
    if (!entry || entry.symbol !== symbol) return;
    active.delete(resource);
    const now = Date.now();
    const perfEntry = {
      name: entry.name || data?.name || 'dns',
      entryType: entry.type || data?.type || 'dns',
      startTime: entry.startTime,
      duration: Math.max(0, now - entry.startTime),
      detail: { ...(entry.detail || {}), ...(data?.detail || {}) },
    };
    for (const obs of observersFor(perfEntry.entryType)) {
      try {
        obs._push(perfEntry);
      } catch {}
    }
  },
};
