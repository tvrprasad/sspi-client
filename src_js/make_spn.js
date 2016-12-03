function makeSpn(serviceClassname, fdqn, instanceNameOrPort, cb) {
  return serviceClassname + '/' + fdqn + ':' + instanceNameOrPort;
}

module.exports.makeSpn = makeSpn;
