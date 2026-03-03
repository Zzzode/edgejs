'use strict';

class HeapSnapshotStream {
  read() { return null; }
  on() { return this; }
}

module.exports = {
  HeapSnapshotStream,
  getHeapStatistics() { return {}; },
  getHeapCodeStatistics() { return {}; },
  getHeapSpaceStatistics() { return []; },
  writeHeapSnapshot() { return ''; },
};
