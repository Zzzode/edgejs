'use strict';

class BlockList {
  constructor() {
    this._rules = [];
  }

  static isBlockList(value) {
    return value instanceof BlockList;
  }

  addAddress(address, type = 'ipv4') {
    this._rules.push({ kind: 'address', address: String(address), type: String(type).toLowerCase() });
  }

  addRange(start, end, type = 'ipv4') {
    this._rules.push({ kind: 'range', start: String(start), end: String(end), type: String(type).toLowerCase() });
  }

  addSubnet(network, prefix, type = 'ipv4') {
    this._rules.push({ kind: 'subnet', network: String(network), prefix: Number(prefix), type: String(type).toLowerCase() });
  }

  check(address, type = 'ipv4') {
    const addr = String(address);
    const fam = String(type).toLowerCase();
    for (const rule of this._rules) {
      if (rule.type !== fam) continue;
      if (rule.kind === 'address' && rule.address === addr) return true;
    }
    return false;
  }
}

module.exports = { BlockList };
