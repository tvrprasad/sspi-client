'use strict';

const os = require('os');

function makeSpn(serviceClassname, fqdn, instanceNameOrPort) {
  if (os.type() !== 'Windows_NT') {
    throw new Error('Package currently not-supported on non-Windows platforms.');
  }

  return serviceClassname + '/' + fqdn + ':' + instanceNameOrPort;
}

module.exports.makeSpn = makeSpn;
