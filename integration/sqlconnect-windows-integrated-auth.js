const Connection = require('../../../src/tedious/src/tedious').Connection;
var Request = require('../../../src/tedious/src/tedious').Request;

const config = {
  domain: 'REDMOND',
  options: { database: 'master' }
};

let testConfigs = [
  { server: 'prasad-asus', securityPackage: undefined, encrypt: false},
  { server: 'prasad-asus', securityPackage: undefined, encrypt: true },
  { server: 'prasad-asus', securityPackage: 'negotiate', encrypt: false },
  { server: 'prasad-asus', securityPackage: 'negotiate', encrypt: true },
  { server: 'prasad-asus', securityPackage: 'kerberos', encrypt: false },
  { server: 'prasad-asus', securityPackage: 'kerberos', encrypt: true },
  { server: 'prasad-asus', securityPackage: 'ntlm', encrypt: false },
  { server: 'prasad-asus', securityPackage: 'ntlm', encrypt: true },

  // These are known failures.
  //{ server: 'prasadtammana', securityPackage: undefined, encrypt: false },
  //{ server: 'prasadtammana', securityPackage: undefined, encrypt: true },
  //{ server: 'prasadtammana', securityPackage: 'negotiate', encrypt: false },
  //{ server: 'prasadtammana', securityPackage: 'negotiate', encrypt: true },

  { server: 'prasadtammana', securityPackage: 'kerberos', encrypt: false },
  { server: 'prasadtammana', securityPackage: 'kerberos', encrypt: true },
  { server: 'prasadtammana', securityPackage: 'ntlm', encrypt: false },
  { server: 'prasadtammana', securityPackage: 'ntlm', encrypt: true }
];

const sqlQuery = 'SELECT * FROM dbo.MSreplication_options';

let currentTestIndex = 0;

runNextTest();

function runNextTest() {
  config.server = testConfigs[currentTestIndex].server;
  config.securityPackage = testConfigs[currentTestIndex].securityPackage;
  config.options.encrypt = testConfigs[currentTestIndex].encrypt;

  const connection = new Connection(config);
  connection.on('connect', function (err) {
    if (err) {
      console.log("Connection failed. Error details:");
      console.log(err);
    }
    else {
      executeStatement(connection);
    }
  });
}

function executeStatement(connection) {
  request = new Request(sqlQuery, function (err) {
    if (err) {
      console.log(err);
      connection.close();
    } else {
      console.log('Success: Rows=', request.rowCount, ' for config:');
      console.log(testConfigs[currentTestIndex]);
      console.log();
      connection.close();

      currentTestIndex++;
      if (currentTestIndex != testConfigs.length) {
        runNextTest();
      }
    }
  });

  connection.execSql(request);
}
