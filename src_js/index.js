var os = require('os');

const NativeAuthProvider = require('./native.js').NativeAuthProvider;

module.exports = function(options) {
  // require and export only for platforms where the module is supported.
  // Individual module require'ed use features of node.js that would trigger
  // syntax errors in older versions. This allows for the module to be included
  // in applications like Tedious which supports older version of node.js, even
  // if the functionality itself won't be available. The application can decide
  // what to do if the module is not supported on the platform where it's running.
  if (require('semver').gte(process.version, '4.0.0') && os.type() === 'Windows_NT') {
    return function(connection) {
      return new NativeAuthProvider(connection, options);
    };
  }
  return undefined;
};
