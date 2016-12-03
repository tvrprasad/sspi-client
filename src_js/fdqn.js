const dns = require('dns');
const net = require('net');
const os = require('os');

const localhostIdentifier = 'localhost';

// Do a reverse lookup on the IP address and return the first FDQN.
function getFdqnForIpAddress(ipAddress, cb) {
  dns.reverse(ipAddress, function(err, fdqns) {
    if (err) {
      cb(err, fdqns);
    } else if (fdqns[0].toLowerCase() === localhostIdentifier) {
      getFdqn(localhostIdentifier, cb);
    } else {
      cb(err, fdqns[0]);
    }
  });
}

// Get the IP addresses for host. For each of the IP addresses, do a reverse
// lookup, one IP address at a time and take the FDQN from the first successful
// result.
function getFdqnForHostname(hostname, cb) {
  let addressIndex = 0;

  dns.lookup(hostname, { all: true }, function (err, addresses, family) {
    const tryNextAddressOrComplete = (err, fdqn) => {
      addressIndex++;
      if (!err) {
        cb(err, fdqn);
      } else if (addressIndex != addresses.length) {
        getFdqnForIpAddress(addresses[addressIndex].address, tryNextAddressOrComplete);
      } else {
        cb(err);
      }
    };

    if (!err) {
      getFdqnForIpAddress(addresses[addressIndex].address, tryNextAddressOrComplete);
    } else {
      cb(err);
    }
  });
}

// Handles IP address, hostname, localhost or FDQN.
function getFdqn(hostidentifier, cb) {
  if (net.isIP(hostidentifier)) {
    getFdqnForIpAddress(hostidentifier, cb);
  } else if (hostidentifier.toLowerCase() == localhostIdentifier) {
    getFdqnForHostname(os.hostname(), cb);
  } else if (hostidentifier.indexOf('.') == -1) {
    getFdqnForHostname(hostidentifier, cb);
  } else {
    // hostidentifier is an fdqn, just return it.
    cb(null, hostidentifier);
  }
}

module.exports.getFdqn = getFdqn;
