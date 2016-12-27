'use strict';

const net = require('net');
const SspiClientApi = require('../src_js/index.js');
const Fqdn = require('../src_js/fqdn.js');
const MakeSpn = require('../src_js/make_spn.js')

let sspiClient = null;
let serverResponse = null;
let expectedServerResponseSize = 0;
let canInvokeGetNextBlob = false;

const getBufferLengthSafe = (buffer) => {
  if (buffer) {
    return buffer.length;
  } else {
    return 0;
  }
}

const host = 'localhost';
const port = 2000;

const connectOpts = {
  host: host,
  port: port
};

const clientSocket = net.connect(connectOpts);

const onConnect = () => {
  console.log('Connected to server.');
  getFqdn();
}

const onError = (err) => {
  console.log('Error: ', err);
  clientSocket.destroy();
}

const onData = (data) => {
  // console.log('Data Length: ', data.length, ' Data: ', data);

  if (canInvokeGetNextBlob) {
    throw new Error('Unexpected data from server while client ready to generate token.');
  }

  if (serverResponse) {
    serverResponse = Buffer.concat([serverResponse, data]);
  } else {
    serverResponse = data;
  }

  // First 4 bytes in the server response indicate the response size.
  if (!expectedServerResponseSize && serverResponse.length >= 4) {
    expectedServerResponseSize = serverResponse.readUInt32LE(0);
    serverResponse = serverResponse.slice(4);
  }

  if (expectedServerResponseSize && serverResponse.length == expectedServerResponseSize) {
    canInvokeGetNextBlob = true;
    console.log('Token buffer received (', expectedServerResponseSize, 'bytes):');
    console.log();
  }

  if (expectedServerResponseSize && serverResponse.length > expectedServerResponseSize) {
    throw new Error('Too many bytes from server: expected response size: ' + expectedServerResponseSize
      + ' , actual response size: ' + serverResponse.length);
  }
}

clientSocket.on('connect', onConnect);
clientSocket.on('error', onError);
clientSocket.on('data', onData);

const getFqdn = () => {
  Fqdn.getFqdn(host, (err, fqdn) => {
    if (err) {
      console.log(err);
      return;
    }

    const spn = MakeSpn.makeSpn('MSSQLSvc', fqdn, 2000);
    // const spn = 'redmond\\venktam';
    // const spn = 'random string';
    console.log('spn: ', spn);
    console.log();
    sspiClient = new SspiClientApi.SspiClient(spn);
    canInvokeGetNextBlob = true;
    getNextBlob();
  });
}

const getNextBlob = () => {
  if (canInvokeGetNextBlob) {
    canInvokeGetNextBlob = false;
    sspiClient.getNextBlob(serverResponse, 0, getBufferLengthSafe(serverResponse), (clientResponse, isDone, errorCode, errorString) => {
      if (errorCode) {
        console.log(errorString);
      } else {
        if (clientResponse.length) {
          // First 4 bytes in client response indicate response size to the server.
          const buf = new Buffer(4);
          buf.writeUInt32LE(clientResponse.length);
          clientSocket.write(buf);

          clientSocket.write(clientResponse);
        }

        console.log('Token buffer generated (', clientResponse.length, 'bytes): isDone=', isDone);
        // console.log('Sent Data: ', clientResponse);

        serverResponse = null;
        expectedServerResponseSize = 0;

        if (!isDone) {
          setTimeout(getNextBlob, 1);
        } else {
          console.log();
          console.log('Authentication succeeded. Security package = ', SspiClientApi.getDefaultSspiPackageName());
        }
      }
    });
  } else {
    // Didn't get full server response yet. Try again.
    setTimeout(getNextBlob, 1);
  }
}
