const sspiClientNative = require('bindings')('sspi-client');

// SSPI intialization code must only be invoked once. These two variables track
// whether the intialization code is invoked, completed execution and if
// initialization succeeded.
let initializeInvoked = false;
let initializeExecutionCompleted = false;
let initializeSucceeded = false;

let initializeErrorCode = 0;
let initializeErrorString = '';

let sspiPackageName = 'SSPI package not initialized';

// JavaScript wrapper class on top of the native binding that implements
// wrappers to invoke Windows SSPI calls. The native bindings will have the
// minimal code to invoke the Windows SSPI calls. All the error checking,
// handling and any workflow will be done in JavaScript.
class SspiClient {
  // Creates an instance of the object in native code, which will invoke
  // Windows SSPI calls.
  constructor(spn) {
    if (arguments.length !== 1) {
      throw new Error('Invalid number of arguments.');
    } else if (typeof (spn) !== 'string') {
      throw new Error('Invalid argument type for \'spn\'.');
    } else if (spn === '') {
      throw Error('Empty string argument for \'spn\'.');
    }

    this.sspiClientImpl = new sspiClientNative.SspiClient(spn);
    this.getNextBlobInProgress = false;
  }

  // Gets the next SSPI blob on the client side to send to the server as
  // part of authentication negotiation.
  //
  // serverResponse - Buffer with SSPI response from the server.
  // serverResponseLength - Length of response within the buffer.
  //
  // Signature of cb is:
  //  cb(clientResponse, isDone, errorCode, errorString)
  //      clientResponse - Buffer to send to the server.
  //      isDone - boolean that specifies if the negotiation is done.
  //      errorCode - number representing an error code from Windows API.
  //                  0 is success, non-zer failure.
  //      errorString - string error details.
  getNextBlob(serverResponse, serverResponseLength, cb) {
    if (arguments.length !== 3) {
      throw new Error('Invalid number of arguments.');
    } else if (typeof(serverResponseLength) !== 'number'
      || Math.floor(serverResponseLength) !== serverResponseLength
      || serverResponseLength < 0) {
      throw new Error('\'serverResponseLength\' must be a non-negative integer.');
    } else if (serverResponseLength > 0 && !(serverResponse instanceof Buffer)) {
      throw new Error('Invalid argument type for \'serverResponse\'.');
    } else if (typeof (cb) !== 'function') {
      throw new Error('Invalid argument type for \'cb\'.');
    } else if (this.getNextBlobInProgress) {
      throw new Error('Single invocation of getNextBlob per instance of SspiClient may be in flight.');
    }

    this.getNextBlobInProgress = true;

    // Invoke initialization code if it's not already invoked.
    if (!initializeInvoked) {
      initializeInvoked = true;
      sspiClientNative.initialize((sspiPackage, errorCode, errorString) => {
        initializeExecutionCompleted = true;
        if (errorCode === 0) {
          initializeSucceeded = true;
          sspiPackageName = sspiPackage;
        } else {
          initializeErrorCode = errorCode;
          initializeErrorString = errorString;
        }
      });
    }

    // Inside the function defined below, 'this' will be the global context.
    const that = this;

    // Wait for initialization to complete. If initialization fails, invoke
    // callback with error information. Else, invoke native implementation.
    const invokeGetNextBlob = function () {
      if (!initializeExecutionCompleted) {
        // You cannot user process.nextTick() here as it will block all
        // I/O which means initialization in native code will never get
        // a chance to run and the process will just hang.
        setImmediate(invokeGetNextBlob);
      } else if (!initializeSucceeded) {
        cb(null, null, initializeErrorCode, initializeErrorString);
      } else {
        that.sspiClientImpl.getNextBlob(serverResponse, serverResponseLength, function () {
          that.getNextBlobInProgress = false;
          cb.apply(null, arguments);
        });
      }
    }

    invokeGetNextBlob();
  }

  // Class methods below are for unit testing only.
  utEnableCannedResponse() {
    this.sspiClientImpl.utEnableCannedResponse(true);
  }

  utDisableCannedResponse() {
    this.sspiClientImpl.utEnableCannedResponse(false);
  }

  utEnableForceCompleteAuth() {
    this.sspiClientImpl.utForceCompleteAuth(true);
  }

  utDisableForceCompleteAuth() {
    this.sspiClientImpl.utForceCompleteAuth(false);
  }
}

function getSspiPackageName() {
  return sspiPackageName;
}

// Methods defined below this line are for unit testing only.
function enableNativeDebugLogging() {
    sspiClientNative.enableDebugLogging(true);
}

function disableNativeDebugLogging() {
    sspiClientNative.enableDebugLogging(false);
}

module.exports.SspiClient = SspiClient;
module.exports.enableNativeDebugLogging = enableNativeDebugLogging;
module.exports.disableNativeDebugLogging = disableNativeDebugLogging;
module.exports.getSspiPackageName = getSspiPackageName;
