'use strict';

const dgram = require('dgram');

const options = { type: 'udp4', reusePort: true };

function checkSupportReusePort() {
  return new Promise((resolve, reject) => {
    const socket = dgram.createSocket(options);
    socket.bind(0);
    socket.on('listening', () => {
      socket.close(resolve);
    });
    socket.on('error', (err) => {
      socket.close();
      reject(err);
    });
  });
}

module.exports = {
  checkSupportReusePort,
  options,
};
