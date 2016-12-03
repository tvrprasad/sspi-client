const Fdqn = require('../src_js/fdqn.js');

function fdqnTestSuccessImpl(test, hostidentifier, expectedFdqnPattern) {
  Fdqn.getFdqn(hostidentifier, (err, fdqn) => {
    test.strictEqual(err, null);
    test.notStrictEqual(fdqn.match(expectedFdqnPattern), null,
      'host: ' + hostidentifier + ', fdqn: ' + fdqn + ', patten: ' + expectedFdqnPattern);
    test.done();
  });
}

exports.loopbackIpAddress = function(test) {
  fdqnTestSuccessImpl(test, '127.0.0.1', /^prasadtammana.redmond.corp.microsoft.com$/);
}

exports.ipv4Address = function (test) {
  const facebookIpv4 = '31.13.77.36';
  fdqnTestSuccessImpl(test, facebookIpv4, /^.*\.facebook.com$/);
}

exports.ipv6Address = function (test) {
  const facebookIpv6 = '2a03:2880:f127:83:face:b00c:0:25de';
  fdqnTestSuccessImpl(test, facebookIpv6, /^.*\.facebook.com$/);
}

exports.simpleHostname = function(test) {
  fdqnTestSuccessImpl(test, 'prasadtammana', /^prasadtammana.redmond.corp.microsoft.com$/);
}

exports.localhostLowerCase = function(test) {
  fdqnTestSuccessImpl(test, 'localhost', /^prasadtammana.redmond.corp.microsoft.com$/);
}

exports.localhostUpperCase = function(test) {
  fdqnTestSuccessImpl(test, 'LOCALHOST', /^prasadtammana.redmond.corp.microsoft.com$/);
}

exports.localhostMixedCase = function(test) {
  fdqnTestSuccessImpl(test, 'LoCaLhOsT', /^prasadtammana.redmond.corp.microsoft.com$/);
}

exports.hostFdqn = function (test) {
  fdqnTestSuccessImpl(test, 'abc.example.com', /^abc.example.com$/);
}

exports.invalidIpaddress = function (test) {
  Fdqn.getFdqn('1.1.1.1', (err, fdqn) => {
    test.notStrictEqual(err, null, 'fdqn: ', + fdqn);
    test.done();
  });
}

exports.invalidHostname = function (test) {
  Fdqn.getFdqn('non-existent', (err, fdqn) => {
    test.notStrictEqual(err, null, 'fdqn: ', +fdqn);
    test.done();
  });
}
