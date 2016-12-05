const dns = require('dns');
const net = require('net');
const os = require('os');

const localhostIdentifier = 'localhost';

// Do a reverse lookup on the IP address and return the first FQDN.
function getFqdnForIpAddress(ipAddress, cb) {
  dns.reverse(ipAddress, function(err, fqdns) {
    if (err) {
      cb(err, fqdns);
    } else if (fqdns[0].toLowerCase() === localhostIdentifier) {
      getFqdn(localhostIdentifier, cb);
    } else {
      cb(err, fqdns[0]);
    }
  });
}

// Get the IP addresses for host. For each of the IP addresses, do a reverse
// lookup, one IP address at a time and take the FQDN from the first successful
// result.
function getFqdnForHostname(hostname, cb) {
  let addressIndex = 0;

  dns.lookup(hostname, { all: true }, function (err, addresses, family) {
    const tryNextAddressOrComplete = (err, fqdn) => {
      addressIndex++;
      if (!err) {
        cb(err, fqdn);
      } else if (addressIndex != addresses.length) {
        getFqdnForIpAddress(addresses[addressIndex].address, tryNextAddressOrComplete);
      } else {
        cb(err);
      }
    };

    if (!err) {
      getFqdnForIpAddress(addresses[addressIndex].address, tryNextAddressOrComplete);
    } else {
      cb(err);
    }
  });
}

// Handles IP address, hostname, localhost or FQDN.
function getFqdn(hostidentifier, cb) {
  if (net.isIP(hostidentifier)) {
    getFqdnForIpAddress(hostidentifier, cb);
  } else if (hostidentifier.toLowerCase() == localhostIdentifier) {
    getFqdnForHostname(os.hostname(), cb);
  } else if (hostidentifier.indexOf('.') == -1) {
    getFqdnForHostname(hostidentifier, cb);
  } else {
    // hostidentifier is an Fqdn, just return it.
    cb(null, hostidentifier);
  }
}

module.exports.getFqdn = getFqdn;

