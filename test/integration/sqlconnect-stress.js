'use strict';

// This test:
//    - sets up about 1000 parallel connections
//    - spread across two different machines
//    - using a mix of all supported security packages
//    - with encrypt set to true and false
//    - makes a simple query on each connection in parallel
//    - validates that the query returns at least one row of data

const fs = require('fs');
const os = require('os');

const Connection = require('../../../../src/tedious/src/tedious').Connection;
const Request = require('../../../../src/tedious/src/tedious').Request;

const config = {
  domain: 'REDMOND',
  options: { database: 'master' }
};

function getLocalhostName() {
  return JSON.parse(fs.readFileSync(
    os.homedir() + '/.sspi-client/test_config.json', 'utf8')).localhostName;
}

function getRemoteHostName() {
  return JSON.parse(fs.readFileSync(
    os.homedir() + '/.sspi-client/test_config.json', 'utf8')).remoteHostName;
}

let testConfigTemplate = [
  { server: getLocalhostName(), securityPackage: undefined, encrypt: false },
  { server: getLocalhostName(), securityPackage: undefined, encrypt: true },
  { server: getLocalhostName(), securityPackage: 'negotiate', encrypt: false },
  { server: getLocalhostName(), securityPackage: 'negotiate', encrypt: true },
  { server: getLocalhostName(), securityPackage: 'kerberos', encrypt: false },
  { server: getLocalhostName(), securityPackage: 'kerberos', encrypt: true },
  { server: getLocalhostName(), securityPackage: 'ntlm', encrypt: false },
  { server: getLocalhostName(), securityPackage: 'ntlm', encrypt: true },
  { server: getRemoteHostName(), securityPackage: undefined, encrypt: false },
  { server: getRemoteHostName(), securityPackage: undefined, encrypt: true },
  { server: getRemoteHostName(), securityPackage: 'negotiate', encrypt: false },
  { server: getRemoteHostName(), securityPackage: 'negotiate', encrypt: true },
  { server: getRemoteHostName(), securityPackage: 'kerberos', encrypt: false },
  { server: getRemoteHostName(), securityPackage: 'kerberos', encrypt: true },
  { server: getRemoteHostName(), securityPackage: 'ntlm', encrypt: false },
  { server: getRemoteHostName(), securityPackage: 'ntlm', encrypt: true }
];

let testConfigs = [];
const maxNumTests = 1000;

while (testConfigs.length < maxNumTests) {
  testConfigs = testConfigs.concat(testConfigTemplate);
}

const sqlQuery = 'SELECT * FROM dbo.MSreplication_options';

let successCount = 0;
let failureCount = 0;

process.on('exit', () => {
  let resultStr = '#### ';
  if (successCount === testConfigs.length) {
    resultStr += 'SUCCESS: ';
  } else {
    resultStr += 'FAILURE: ';
  }

  console.log('####################################################');
  console.log(resultStr, successCount, ' out of ', testConfigs.length, ' tests succeeded.');
  console.log(resultStr, failureCount, ' out of ', testConfigs.length, ' tests failed.');
  console.log('####################################################');
});

for (let i = 0; i < testConfigs.length; i++) {
  runNextTest(i);
}

function runNextTest(currentTestIndex) {
  config.server = testConfigs[currentTestIndex].server;
  config.securityPackage = testConfigs[currentTestIndex].securityPackage;
  config.options.encrypt = testConfigs[currentTestIndex].encrypt;

  const connection = new Connection(config);
  connection.on('connect', function (err) {
    if (err) {
      failureCount++;
      console.log('ERROR: Connection failed for config:');
      console.log(testConfigs[currentTestIndex]);
      console.log(err);
      console.log();
    }
    else {
      executeStatement(connection, currentTestIndex);
    }
  });
}

function executeStatement(connection, currentTestIndex) {
  let rowCount = 0;

  const request = new Request(sqlQuery, function (err) {
    if (err) {
      failureCount++;
      console.log('ERROR: Query failed for config:');
      console.log(testConfigs[currentTestIndex]);
      console.log(err);
      console.log();
    } else {
      if (rowCount > 0) {
        successCount++;
      } else {
        failureCount++;
        console.log('ERROR: Query completed without data coming back. Rows=', rowCount, '. For config:');
        console.log(testConfigs[currentTestIndex]);
        console.log();
      }
    }

    connection.close();
  });

  request.on('row', function (columns) {
    rowCount++;
  });

  connection.execSql(request);
}
