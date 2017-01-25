'use strict';

// This test:
//  - Defines test configurations to cover all security packages with and
//    without encryption across two machines.
//  - Establishes connection and makes a query for each configuration.
//  - Does this sequentially.
//  - Goal is test the package with Tedious for all configurations.

const ConfigUtils = require('../utils/config.js');
const Connection = require('../../../../src/tedious/src/tedious').Connection;
const Request = require('../../../../src/tedious/src/tedious').Request;

const config = {
  domain: 'REDMOND',
  options: { database: 'master' }
};

let testConfigs = [
  { server: ConfigUtils.getLocalhostName(), securityPackage: undefined, encrypt: false },
  { server: ConfigUtils.getLocalhostName(), securityPackage: undefined, encrypt: true },
  { server: ConfigUtils.getLocalhostName(), securityPackage: 'negotiate', encrypt: false },
  { server: ConfigUtils.getLocalhostName(), securityPackage: 'negotiate', encrypt: true },
  { server: ConfigUtils.getLocalhostName(), securityPackage: 'kerberos', encrypt: false },
  { server: ConfigUtils.getLocalhostName(), securityPackage: 'kerberos', encrypt: true },
  { server: ConfigUtils.getLocalhostName(), securityPackage: 'ntlm', encrypt: false },
  { server: ConfigUtils.getLocalhostName(), securityPackage: 'ntlm', encrypt: true },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: undefined, encrypt: false },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: undefined, encrypt: true },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: 'negotiate', encrypt: false },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: 'negotiate', encrypt: true },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: 'kerberos', encrypt: false },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: 'kerberos', encrypt: true },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: 'ntlm', encrypt: false },
  { server: ConfigUtils.getRemoteHostName(), securityPackage: 'ntlm', encrypt: true }
];

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

runNextTest(0);

function runNextTest(currentTestIndex) {
  if (currentTestIndex == testConfigs.length) {
    return;
  }

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

      runNextTest(++currentTestIndex);
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
    runNextTest(++currentTestIndex);
  });

  request.on('row', function (columns) {
    rowCount++;
  });

  connection.execSql(request);
}
