const SspiClientApi = require('../src_js/index.js');

// Comment/Uncomment to enable/disable debug logging in native code.
// SspiClientApi.enableNativeDebugLogging();

class MaybeDone {
  constructor(test, numRuns) {
    this.test = test;
    this.numRuns = numRuns;
    this.numRunsSoFar = 0;
  }

  done() {
    this.numRunsSoFar++;

    if (this.numRunsSoFar == this.numRuns) {
      this.test.done();
    }
  }
}

// This test relies on being the first test to run.
exports.ensureInitializationNoCallback = function (test) {
  SspiClientApi.ensureInitialization();

  let expectedErrorMessage = 'Initialization not completed.';

  try {
    SspiClientApi.getDefaultSspiPackageName();
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
  }

  expectedErrorMessage = 'Initialization not completed.';

  try {
    SspiClientApi.getSupportedSspiPackageNames();
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
  }

  test.expect(2);
  test.done();
}

exports.ensureInitializationWithCallback = function (test) {
  SspiClientApi.ensureInitialization((errorCode, errorString) => {
    test.strictEqual(errorCode, 0);
    test.strictEqual(errorString, '');

    test.strictEqual(SspiClientApi.getDefaultSspiPackageName(), 'Negotiate');

    let availableSspiPackageNames = SspiClientApi.getSupportedSspiPackageNames();
    test.strictEqual(availableSspiPackageNames.length, 3);
    test.strictEqual(availableSspiPackageNames[0], 'Negotiate');
    test.strictEqual(availableSspiPackageNames[1], 'Kerberos');
    test.strictEqual(availableSspiPackageNames[2], 'NTLM');

    test.done();
  });
}

exports.ensureInitializationTooManyArgs = function (test) {
  const expectedErrorMessage = 'Invalid number of arguments.';

  try {
    SspiClientApi.ensureInitialization('arg1', 'arg2');
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.ensureInitializationInvalidArgTypeCb = function (test) {
  const expectedErrorMessage = 'Invalid argument type for \'cb\'.';

  try {
    const numberTypeArg = 75;
    SspiClientApi.ensureInitialization(numberTypeArg);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.constructorSuccess = function (test) {
  const sspiClient = new SspiClientApi.SspiClient("fake_spn");
  test.ok(sspiClient);
  test.done();
}

exports.constructorEmptyArgs = function(test) {
  const expectedErrorMessage = 'Invalid number of arguments.';

  try {
    const sspiClient = new SspiClientApi.SspiClient();
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.constructorTooManyArgs = function (test) {
  const expectedErrorMessage = 'Invalid number of arguments.';

  try {
    const sspiClient = new SspiClientApi.SspiClient('fake_spn', 'fake_securitypackage', 'spuriousarg');
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.constructorInvalidArgTypeSpn = function(test) {
  const expectedErrorMessage = 'Invalid argument type for \'spn\'.';

  try {
    const numberTypeArg = 75;
    const sspiClient = new SspiClientApi.SspiClient(numberTypeArg);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.constructorInvalidArgTypeSecurityPackage = function (test) {
  const expectedErrorMessage = 'Invalid argument type for \'securityPackage\'.';

  try {
    const numberTypeArg = 75;
    const sspiClient = new SspiClientApi.SspiClient('fake_spn', numberTypeArg);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.constructorEmptyStringSpn = function (test) {
  const expectedErrorMessage = 'Empty string argument for \'spn\'.';

  try {
    const emptyStringArg = '';
    const sspiClient = new SspiClientApi.SspiClient(emptyStringArg);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.constructorInvalidSecurityPackage = function (test) {
  const expectedErrorMessage =
    '\'securityPackage\' if specified must be one of \'negotiate\' or \'kerberos\' or \'ntlm\'.';

  try {
    const invalidSecurityPackage = 'invalid-security-package';
    const sspiClient = new SspiClientApi.SspiClient('fake_spn', invalidSecurityPackage);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

function getNextBlobBasicImpl(test, sspiClient) {
  const serverResponse = null;
  const serverResponseLength = 0;

  sspiClient.getNextBlob(serverResponse, serverResponseLength, (clientResponse, isDone, errorCode, errorString) => {
    test.ok(clientResponse.length > 0);
    test.strictEqual(isDone, false);
    test.strictEqual(errorCode, 0);
    test.strictEqual(errorString, '');

    let notAllZeros = false;
    let notAllEqual = false;
    let prevByte = clientResponse[0];
    for (i = 0; i < clientResponse.length; i++) {
      if (prevByte != clientResponse[i]) {
        notAllEqual = true;
      }

      if (clientResponse[i] != 0) {
        notAllZeros = true;
      }

      if (notAllZeros && notAllEqual) {
        break;
      }
    }

    test.ok(notAllEqual);
    test.ok(notAllZeros);

    test.done();
  });
}

// Basic test that goes through the motions of a first invocation of
// getNextBlob on an instance of SspiClient. This sends a fake spn but the
// first call will succeed as this is just preparing client response to send
// to the server where actual validation will happen.
exports.getNextBlobBasic = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');
  getNextBlobBasicImpl(test, sspiClient);
}

exports.getNextBlobBasicNegotiate = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn', 'negotiate');
  sspiClient.utEnableForceCompleteAuth();
  getNextBlobBasicImpl(test, sspiClient);
}

exports.getNextBlobBasicKerberos = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn', 'negotiate');
  sspiClient.utEnableForceCompleteAuth();
  getNextBlobBasicImpl(test, sspiClient);
}

exports.getNextBlobBasicNtlm = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn', 'negotiate');
  sspiClient.utEnableForceCompleteAuth();
  getNextBlobBasicImpl(test, sspiClient);
}

exports.getNextBlobBasicForceCompleteAuth = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');
  sspiClient.utEnableForceCompleteAuth();
  getNextBlobBasicImpl(test, sspiClient);
}

function getNextBlobCannedResponseEmptyInBufImpl(test, serverResponse, serverResponseLength, maybeDone) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Set unit test mode to get a canned response corresponding to empty serverResponse.
  sspiClient.utEnableCannedResponse();

  sspiClient.getNextBlob(serverResponse, serverResponseLength, (clientResponse, isDone, errorCode, errorString) => {
    test.strictEqual(clientResponse.length, 25);
    for (i = 0; i < 25; i++) {
      test.strictEqual(clientResponse[i], i);
    }

    test.strictEqual(true, isDone);
    test.strictEqual(errorCode, 0x80090304);
    test.strictEqual(errorString, 'Canned Response without input data.');

    test.strictEqual(SspiClientApi.getDefaultSspiPackageName(), 'Negotiate');

    let availableSspiPackageNames = SspiClientApi.getSupportedSspiPackageNames();
    test.strictEqual(availableSspiPackageNames.length, 3);
    test.strictEqual(availableSspiPackageNames[0], 'Negotiate');
    test.strictEqual(availableSspiPackageNames[1], 'Kerberos');
    test.strictEqual(availableSspiPackageNames[2], 'NTLM');

    maybeDone.done();
  });
}

// Validates
//  - JavaScript going through and executing native code.
//  - client response coming back from native code to JavaScript correctly.
exports.getNextBlobCannedResponseEmptyInBuf = function (test) {
  const numRuns = 7;
  maybeDone = new MaybeDone(test, numRuns);

  for (i = 0; i < numRuns - 2; i++) {
    getNextBlobCannedResponseEmptyInBufImpl(test, null, 0, maybeDone);
  }

  getNextBlobCannedResponseEmptyInBufImpl(test, Buffer.alloc(10), 0, maybeDone);
  getNextBlobCannedResponseEmptyInBufImpl(test, 'some string', 0, maybeDone);
}

// Validates
//  - JavaScript going through and executing native code.
//  - server response going from JavaScript to native code correctly.
//  - client response coming back from native code to JavaScript correctly.
exports.getNextBlobCannedResponseNonEmptyInBufImpl = function (test) {
  const serverResponse = Buffer.alloc(35);
  const serverResponseLength = 20;
  for (i = 0; i < serverResponseLength; i++) {
    serverResponse[i] = serverResponseLength - i;
  }

  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Set unit test mode to get a canned response corresponding to empty serverResponse.
  sspiClient.utEnableCannedResponse();

  sspiClient.getNextBlob(serverResponse, serverResponseLength, (clientResponse, isDone, errorCode, errorString) => {
    test.strictEqual(clientResponse.length, serverResponseLength);
    for (i = 0; i < serverResponseLength; i++) {
      test.strictEqual(clientResponse[i], serverResponse[i]);
    }

    test.strictEqual(isDone, false);
    test.strictEqual(errorCode, 0x80090303);
    test.strictEqual(errorString, 'Canned Response with input data.');

    test.strictEqual(SspiClientApi.getDefaultSspiPackageName(), 'Negotiate');

    test.done();
  });
}

exports.getNextBlobMultipleInCallbacksSameInstance = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Should not throw exception.
  sspiClient.getNextBlob(Buffer.alloc(10), 10, () => {
    sspiClient.getNextBlob(Buffer.alloc(10), 10, () => {
      test.done();
    });
  });
}

exports.getNextBlobMultipleInProgressSameInstanceFails = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Should not throw exception.
  sspiClient.getNextBlob(Buffer.alloc(10), 10, () => {
  });

  // Should throw exception.
  const expectedErrorMessage = 'Single invocation of getNextBlob per instance of SspiClient may be in flight.';
  try {
    sspiClient.getNextBlob(Buffer.alloc(10), 10, () => {
    });
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

function getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, getNextBlobBound) {
  const expectedErrorMessage = 'Invalid number of arguments.';

  try {
    getNextBlobBound();
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    maybeDone.done();
  }
}

exports.getNextBlobInvalidNumberOfArgs = function (test) {
  const numRuns = 3;
  const maybeDone = new MaybeDone(test, 3);

  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, sspiClient.getNextBlob.bind(sspiClient)
    );  // Empty args.

  getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, sspiClient.getNextBlob.bind(sspiClient,
    Buffer.alloc(5)));  // One arg.

  getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, sspiClient.getNextBlob.bind(sspiClient,
    Buffer.alloc(4), 3)); // Two args.
}

function getNextBlobInvalidArgImpl(test, expectedErrorMessage, getNextBlobBound, maybeDone) {
  try {
    getNextBlobBound();
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    maybeDone.done();
  }
}

exports.getNextBlobInvalidServerResponseArg = function (test) {
  const numRuns = 1;
  const maybeDone = new MaybeDone(test, numRuns);
  const expectedErrorMessage = 'Invalid argument type for \'serverResponse\'.';
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');
  const stringTypeArg = 'Some string';
  getNextBlobInvalidArgImpl(
    test,
    expectedErrorMessage,
    sspiClient.getNextBlob.bind(sspiClient, stringTypeArg, 10, () => { }),
    maybeDone);
}

exports.getNextBlobInvalidServerResponseLengthArg = function (test) {
  let invalidLengths = [
    -10,  // Negative integer.
    "1",  // Number like String.
    "Somestring",   // String.
    3.3,  // Float positive.
    -3.3, // Floag negative.
  ];

  const numRuns = invalidLengths.length;
  maybeDone = new MaybeDone(test, numRuns);

  const expectedErrorMessage = '\'serverResponseLength\' must be a non-negative integer.';
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  for (i = 0; i < invalidLengths.length; i++) {
    getNextBlobInvalidArgImpl(
      test,
      expectedErrorMessage,
      sspiClient.getNextBlob.bind(sspiClient, Buffer.alloc(25), invalidLengths[i], () => { }),
      maybeDone);
  }
}

exports.getNextBlobInvalidCallbackArg = function (test) {
  const numRuns = 1;
  const maybeDone = new MaybeDone(test, numRuns);
  const expectedErrorMessage = 'Invalid argument type for \'cb\'.';
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');
  const stringTypeArg = 'Some string';
  getNextBlobInvalidArgImpl(
    test,
    expectedErrorMessage,
    sspiClient.getNextBlob.bind(sspiClient, Buffer.alloc(10), 10, stringTypeArg),
    maybeDone);
}
