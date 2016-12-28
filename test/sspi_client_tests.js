'use strict';

const SspiClientApi = require('../src_js/index.js').SspiClientApi;

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
    SspiClientApi.getAvailableSspiPackageNames();
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
  }

  test.expect(2);
  test.done();
}

function ensureInitializationWithCallbackImpl(test, maybeDone) {
  SspiClientApi.ensureInitialization((errorCode, errorString) => {
    test.strictEqual(errorCode, 0);
    test.strictEqual(errorString, '');

    test.strictEqual(SspiClientApi.getDefaultSspiPackageName(), 'Negotiate');

    let availableSspiPackageNames = SspiClientApi.getAvailableSspiPackageNames();
    test.strictEqual(availableSspiPackageNames.length, 3);
    test.strictEqual(availableSspiPackageNames[0], 'Negotiate');
    test.strictEqual(availableSspiPackageNames[1], 'Kerberos');
    test.strictEqual(availableSspiPackageNames[2], 'NTLM');

    maybeDone.done();
  });
}

exports.ensureInitializationWithCallback = function (test) {
  const numRuns = 5;
  const maybeDone = new MaybeDone(test, numRuns);

  for (let i = 0; i < numRuns; i++) {
    ensureInitializationWithCallbackImpl(test, maybeDone);
  }
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
  const serverResponseBeginOffset = 0;
  const serverResponseLength = 0;

  sspiClient.getNextBlob(serverResponse, serverResponseBeginOffset, serverResponseLength,
    (clientResponse, isDone, errorCode, errorString) => {
      test.ok(clientResponse.length > 0);
      test.strictEqual(isDone, false);
      test.strictEqual(errorCode, 0);
      test.strictEqual(errorString, '');

      let notAllZeros = false;
      let notAllEqual = false;
      let prevByte = clientResponse[0];
      for (let i = 0; i < clientResponse.length; i++) {
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

function getNextBlobCannedResponseEmptyInBufImpl(test, serverResponse, serverResponseBeginOffset, maybeDone) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Set unit test mode to get a canned response corresponding to empty serverResponse.
  sspiClient.utEnableCannedResponse();

  const serverResponseLength = 0;

  sspiClient.getNextBlob(serverResponse, serverResponseBeginOffset, serverResponseLength,
    (clientResponse, isDone, errorCode, errorString) => {
      test.strictEqual(clientResponse.length, 25);
      for (let i = 0; i < 25; i++) {
        test.strictEqual(clientResponse[i], i);
      }

      test.strictEqual(true, isDone);
      test.strictEqual(errorCode, 0x80090304);
      test.strictEqual(errorString, 'Canned Response without input data.');

      test.strictEqual(SspiClientApi.getDefaultSspiPackageName(), 'Negotiate');

      let availableSspiPackageNames = SspiClientApi.getAvailableSspiPackageNames();
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
  const maybeDone = new MaybeDone(test, numRuns);

  for (let i = 0; i < numRuns - 2; i++) {
    getNextBlobCannedResponseEmptyInBufImpl(test, null, i, maybeDone);
  }

  getNextBlobCannedResponseEmptyInBufImpl(test, Buffer.alloc(10), 0, maybeDone);
  getNextBlobCannedResponseEmptyInBufImpl(test, 'some string', 'some other string', maybeDone);
}

// Validates
//  - JavaScript going through and executing native code.
//  - server response going from JavaScript to native code correctly.
//  - client response coming back from native code to JavaScript correctly.
function getNextBlobCannedResponseNonEmptyInBufImpl(
  test,
  maybeDone,
  serverResponseBufferSize,
  serverResponseBeginOffset,
  serverResponseLength) {

  const serverResponse = Buffer.alloc(serverResponseBufferSize);
  for (let i = 0; i < serverResponseLength; i++) {
    serverResponse[i + serverResponseBeginOffset] = serverResponseLength - i;
  }

  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Set unit test mode to get a canned response corresponding to empty serverResponse.
  sspiClient.utEnableCannedResponse();

  sspiClient.getNextBlob(serverResponse, serverResponseBeginOffset, serverResponseLength, (clientResponse, isDone, errorCode, errorString) => {
    test.strictEqual(clientResponse.length, serverResponseLength);
    for (let i = 0; i < serverResponseLength; i++) {
      test.strictEqual(clientResponse[i], serverResponse[i + serverResponseBeginOffset]);
    }

    test.strictEqual(isDone, false);
    test.strictEqual(errorCode, 0x80090303);
    test.strictEqual(errorString, 'Canned Response with input data.');

    test.strictEqual(SspiClientApi.getDefaultSspiPackageName(), 'Negotiate');

    maybeDone.done();
  });
}

exports.getNextBlobCannedResponseNonEmptyInBuf = function (test) {
  const numRuns = 7;
  const maybeDone = new MaybeDone(test, numRuns);
  const serverResponseBufferSize = 50;
  let serverResponseBeginOffset = 0;
  let serverResponseLength = 50;

  for (let i = 0; i < numRuns - 2; i++) {
    getNextBlobCannedResponseNonEmptyInBufImpl(
      test,
      maybeDone,
      serverResponseBufferSize,
      serverResponseBeginOffset,
      serverResponseLength);

    serverResponseBeginOffset++;
    serverResponseLength--;
  }

  // From middle to end.
  getNextBlobCannedResponseNonEmptyInBufImpl(
    test,
    maybeDone,
    serverResponseBufferSize,
    10,
    serverResponseBufferSize - 10);

  // From begin to middle.
  getNextBlobCannedResponseNonEmptyInBufImpl(
    test,
    maybeDone,
    serverResponseBufferSize,
    0,
    serverResponseBufferSize - 10);
}

exports.getNextBlobMultipleInCallbacksSameInstance = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Should not throw exception.
  sspiClient.getNextBlob(Buffer.alloc(10), 0, 10, () => {
    sspiClient.getNextBlob(Buffer.alloc(10), 0, 10, () => {
      test.done();
    });
  });
}

exports.getNextBlobMultipleInProgressSameInstanceFails = function (test) {
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  // Should not throw exception.
  sspiClient.getNextBlob(Buffer.alloc(10), 0, 10, () => {
  });

  // Should throw exception.
  const expectedErrorMessage = 'Single invocation of getNextBlob per instance of SspiClient may be in flight.';
  try {
    sspiClient.getNextBlob(Buffer.alloc(10), 0, 10, () => {
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
  const numRuns = 4;
  const maybeDone = new MaybeDone(test, numRuns);

  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, sspiClient.getNextBlob.bind(sspiClient)
    );  // Empty args.

  getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, sspiClient.getNextBlob.bind(sspiClient,
    Buffer.alloc(5)));  // One arg.

  getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, sspiClient.getNextBlob.bind(sspiClient,
    Buffer.alloc(4), 3)); // Two args.

  getNextBlobInvalidNumberOfArgsImpl(test, maybeDone, sspiClient.getNextBlob.bind(sspiClient,
  Buffer.alloc(4), 3, 4)); // Three args.
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
    sspiClient.getNextBlob.bind(sspiClient, 0, stringTypeArg, 10, () => { }),
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
  const maybeDone = new MaybeDone(test, numRuns);

  const expectedErrorMessage = '\'serverResponseLength\' must be a non-negative integer.';
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  for (let i = 0; i < invalidLengths.length; i++) {
    getNextBlobInvalidArgImpl(
      test,
      expectedErrorMessage,
      sspiClient.getNextBlob.bind(sspiClient, Buffer.alloc(25), 0, invalidLengths[i], () => { }),
      maybeDone);
  }
}

exports.getNextBlobInvalidServerResponseBeginOffsetArg = function (test) {
  let invalidOffsets = [
    -10,  // Negative integer.
    "1",  // Number like String.
    "Somestring",   // String.
    3.3,  // Float positive.
    -3.3, // Floag negative.
  ];

  const numRuns = invalidOffsets.length;
  const maybeDone = new MaybeDone(test, numRuns);

  const expectedErrorMessage = '\'serverResponseBeginOffset\' must be a non-negative integer.';
  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  for (let i = 0; i < invalidOffsets.length; i++) {
    getNextBlobInvalidArgImpl(
      test,
      expectedErrorMessage,
      sspiClient.getNextBlob.bind(sspiClient, Buffer.alloc(25), invalidOffsets[i], 25, () => { }),
      maybeDone);
  }
}

exports.getNextBlobBufferTooSmall = function (test) {
  const serverResponseBufferSizeIndex = 0;
  const serverResponseBeginOffsetIndex = 1;
  const serverResponseLengthIndex = 2;
  let testData = [
    [0, 0, 1],
    [1, 1, 1],
    [1, 0, 2],
    [25, 0, 26],
    [25, 10, 16],
    [25, 24, 2],
    [25, 25, 1],
    [25, 26, 1]
  ];

  const numRuns = testData.length;
  const maybeDone = new MaybeDone(test, numRuns);

  const sspiClient = new SspiClientApi.SspiClient('fake_spn');

  for (let i = 0; i < testData.length; i++) {
    const expectedErrorMessage = '\'serverResponse\' buffer too small. '
      + '\'serverResponse\' buffer size=' + testData[i][serverResponseBufferSizeIndex]
      + ', \'serverResponseBeginOffset\'=' + testData[i][serverResponseBeginOffsetIndex]
      + ', \'serverResponseLength\'=' + testData[i][serverResponseLengthIndex];

    if (i === testData.length - 1) {
      test.strictEqual(
        expectedErrorMessage,
        '\'serverResponse\' buffer too small. \'serverResponse\' buffer size=25, '
        + '\'serverResponseBeginOffset\'=26, \'serverResponseLength\'=1');
    }

    getNextBlobInvalidArgImpl(
      test,
      expectedErrorMessage,
      sspiClient.getNextBlob.bind(
        sspiClient,
        Buffer.alloc(testData[i][serverResponseBufferSizeIndex]),
        testData[i][serverResponseBeginOffsetIndex],
        testData[i][serverResponseLengthIndex],
        () => { }),
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
    sspiClient.getNextBlob.bind(sspiClient, Buffer.alloc(10), 0, 10, stringTypeArg),
    maybeDone);
}
