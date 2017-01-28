# Tedious Integration Tests

## Setup sspi-client
Follow instructions under Setup and Build in [README.md][].

## Setup test configuration.
Follow instructions in [README.md][] to setup test_config.json with right
values and in the right location.

## Get Tedious bits.
Integrated Auth is not integrated into the master Tedious repository yet. Get
the bits with Integrated Auth as below:

1. git clone https://github.com/tvrprasad/tedious.git
2. cd tedious
3. git checkout windows-integrated-auth-draft
4. npm install

## Edit Paths.
sspi-client is not added to npm registry yet. So Tedious's package.json does
not have a dependency that pulls in sspi-client. We need to manually tweak
paths to get these tests to work.

### Tedious Paths In Tests
In sqlconnect_windows_integrated_auth.js and sqlconnect_stress.js, find the
following lines and tweak the relative paths to Tedious so they match the
location of Tedious on your machine.

```JavaScript
const Connection = require('../../../../src/tedious/src/tedious').Connection;
const Request = require('../../../../src/tedious/src/tedious').Request;
```

### sspi-client Paths in Tedious
In Tedious\src\connection.js, find the following lines and tweak the relative
paths to Tedious so they matche the location of Tedious on your machine.

```JavaScript
const SspiModuleSupported = require('../../sspi-client/src_js/index').ModuleSupported;
const SspiClientApi = require('../../sspi-client/src_js/index').SspiClientApi;
const Fqdn = require('../../sspi-client/src_js/index.js').Fqdn;
const MakeSpn = require('../../sspi-client/src_js/index.js').MakeSpn;
```

### Run Tests
These tests can take a few seconds. 

<code>
node sqlconnect_stress.js

node sqlconnect_windows_integrated_auth.js
</code>

[README.md]: https://github.com/tvrprasad/sspi-client/blob/master/README.md "README.md"
