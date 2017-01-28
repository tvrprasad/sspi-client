'use strict';

const fs = require('fs');
const os = require('os');

function getLocalhostName() {
  return JSON.parse(fs.readFileSync(
    os.homedir() + '/.sspi-client/test_config.json', 'utf8')).localhostName;
}

function getLocalhostFqdn() {
  return JSON.parse(fs.readFileSync(
    os.homedir() + '/.sspi-client/test_config.json', 'utf8')).localhostFqdn;
}

function getRemoteHostName() {
  return JSON.parse(fs.readFileSync(
    os.homedir() + '/.sspi-client/test_config.json', 'utf8')).remoteHostName;
}

function getRemoteHostFqdn() {
  return JSON.parse(fs.readFileSync(
    os.homedir() + '/.sspi-client/test_config.json', 'utf8')).remoteHostFqdn;
}

module.exports.getLocalhostName = getLocalhostName;
module.exports.getLocalhostFqdn = getLocalhostFqdn;
module.exports.getRemoteHostName = getRemoteHostName;
module.exports.getRemoteHostFqdn = getRemoteHostFqdn;
