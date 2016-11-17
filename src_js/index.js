const sspiClientNative = require('bindings')('sspi-client');

// SSPI intialization code must only be invoked once. These two variables track
// whether the intialization code is invoked, completed execution and if
// initialization succeeded.
let initializeInvoked = false;
let initializeExecutionCompleted = false;
let initializeSucceeded = false;

let initializeErrorCode = 0;
let initializeErrorString = '';

// JavaScript wrapper class on top of the native binding that implements
// wrappers to invoke Windows SSPI calls. The native bindings will have the
// minimal code to invoke the Windows SSPI calls. All the error checking,
// handling and any workflow will be done in JavaScript.
class SspiClient {
  // Creates an instance of the object in native code, which will invoke
  // Windows SSPI calls.
  constructor(serverName) {
    if (arguments.length !== 1) {
      throw new Error('Invalid number of arguments.');
    } else if (typeof (serverName) !== 'string') {
      throw new Error('Invalid argument type for \'serverName\'.');
    } else if (serverName === '') {
      throw Error('Empty string argument for \'serverName\'.');
    }

    this.sspiClientImpl = new sspiClientNative.SspiClient(serverName);
  }

  // Gets the next SSPI blob on the client side to send to the server as
  // part of authentication negotiation.
  //
  // Signature of cb is:
  //  cb(clientResponse, isDone, errorCode, errorString)
  //      clientResponse - Buffer to send to the server.
  //      isDone - boolean that specifies if the negotiation is done.
  //      errorCode - number representing an error code from Windows API.
  //                  0 is success, non-zer failure.
  //      errorString - string error details.
  getNextBlob(cb) {
    if (arguments.length !== 1) {
      throw new Error('Invalid number of arguments.');
    } else if (typeof (cb) !== 'function') {
      throw new Error('Invalid argument type for \'cb\'.');
    }

    // Invoke initialization code if it's not already invoked.
    if (!initializeInvoked) {
      initializeInvoked = true;
      sspiClientNative.initialize((errorCode, errorString) => {
        initializeExecutionCompleted = true;
        if (errorCode === 0) {
          initializeSucceeded = true;
        } else {
          initializeErrorCode = errorCode;
          initializeErrorString = errorString;
        }
      });
    }

    // Wait for initialization to complete. If initialization fails, invoke
    // callback with error information. Else, invoke native implementation.
    const that = this;
    const invokeGetNextBlob = function () {
      if (!initializeExecutionCompleted) {
        // You cannot user process.nextTick() here as it will block all
        // I/O which means initialization in native code will never get
        // a chance to run and the process will just hang.
        setImmediate(invokeGetNextBlob);
      } else if (!initializeSucceeded) {
        cb(null, null, errorCode, errorString);
      } else {
        that.sspiClientImpl.getNextBlob(cb);
      }
    }

    invokeGetNextBlob();
  }
}

function enableNativeDebugLogging() {
    sspiClientNative.enableDebugLogging(true);
}

function disableNativeDebugLogging() {
    sspiClientNative.enableDebugLogging(false);
}

module.exports.SspiClient = SspiClient;
module.exports.enableNativeDebugLogging = enableNativeDebugLogging;
module.exports.disableNativeDebugLogging = disableNativeDebugLogging;
