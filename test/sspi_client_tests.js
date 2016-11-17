const SspiClientApi = require('../src_js/index.js');

// Comment/Uncomment to enable/disable debug logging in native code.
// SspiClientApi.enableNativeDebugLogging();

exports.constructorSuccess = function(test) {
  const sspiClient = new SspiClientApi.SspiClient("ServerName");
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

exports.constructorInvalidArgType = function(test) {
  const expectedErrorMessage = 'Invalid argument type for \'serverName\'.';

  try {
    const numberTypeArg = 75;
    const sspiClient = new SspiClientApi.SspiClient(numberTypeArg);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.constructorEmptyStringArg = function(test) {
  const expectedErrorMessage = 'Empty string argument for \'serverName\'.';

  try {
    const emptyStringArg = '';
    const sspiClient = new SspiClientApi.SspiClient(emptyStringArg);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

function getNextBlobSuccessImpl(test, maybeDone) {
  const sspiClient = new SspiClientApi.SspiClient('ServerName');
  sspiClient.getNextBlob((clientResponse, isDone, errorCode, errorString) => {
    test.strictEqual(clientResponse.length, 25);
    for (i = 0; i < 25; i++) {
      test.strictEqual(clientResponse[i], i);
    }

    test.strictEqual(true, isDone);
    test.strictEqual(errorCode, 0x80090304);
    test.strictEqual(errorString, 'ErrorOutOfPaper');

    maybeDone();
  });
}

exports.getNextBlobSuccess = function (test) {
  const numRuns = 5;

  let numRunsSoFar = 0;
  const doneCallback = () => {
    numRunsSoFar++;
    if (numRunsSoFar == numRuns) {
      test.done();
    }
  }

  for (i = 0; i < numRuns; i++) {
    getNextBlobSuccessImpl(test, doneCallback);
  }
}

exports.getNextBlobEmptyArgs = function(test) {
  const expectedErrorMessage = 'Invalid number of arguments.';

  const sspiClient = new SspiClientApi.SspiClient('ServerName');
  try {
    sspiClient.getNextBlob();
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}

exports.getNextBlobInvalidArgType = function(test) {
  const expectedErrorMessage = 'Invalid argument type for \'cb\'.';

  const sspiClient = new SspiClientApi.SspiClient('ServerName');
  try {
    const stringTypeArg = 'Some string';
    sspiClient.getNextBlob(stringTypeArg);
  } catch (err) {
    test.strictEqual(err.message, expectedErrorMessage);
    test.done();
  }
}
