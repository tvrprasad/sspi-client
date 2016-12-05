function makeSpn(serviceClassname, fqdn, instanceNameOrPort, cb) {
  return serviceClassname + '/' + fqdn + ':' + instanceNameOrPort;
}

module.exports.makeSpn = makeSpn;
