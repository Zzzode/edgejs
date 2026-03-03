'use strict';

const EventEmitter = require('events');

let nextWorkerId = 1;

class Cluster extends EventEmitter {
  constructor() {
    super();
    this.isWorker = false;
    this.isPrimary = true;
    this.isMaster = true;
    this.worker = undefined;
    this.workers = Object.create(null);
    this.settings = {};
  }

  setupPrimary(settings) {
    if (settings && typeof settings === 'object') {
      this.settings = { ...this.settings, ...settings };
    }
    return this;
  }

  setupMaster(settings) {
    return this.setupPrimary(settings);
  }

  fork(env) {
    const worker = new EventEmitter();
    worker.id = nextWorkerId++;
    worker.pid = worker.id;
    worker.process = worker;
    worker.connected = true;
    worker.exitedAfterDisconnect = false;
    worker.isConnected = () => worker.connected;
    worker.send = () => worker.connected;
    worker.kill = () => {
      worker.disconnect();
      return true;
    };
    worker.disconnect = () => {
      if (!worker.connected) return;
      worker.connected = false;
      worker.exitedAfterDisconnect = true;
      process.nextTick(() => {
        worker.emit('disconnect');
        worker.emit('exit', 0, null);
      });
      delete this.workers[worker.id];
    };
    worker._env = env && typeof env === 'object' ? { ...env } : {};
    this.workers[worker.id] = worker;
    process.nextTick(() => this.emit('fork', worker));
    return worker;
  }
}

module.exports = new Cluster();
