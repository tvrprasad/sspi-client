# Tedious Integration Tests

## Get sspi-client source
git clone https://github.com/tvrprasad/sspi-client.git

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

## Edit Paths in Tests
In sqlconnect_windows_integrated_auth.js and sqlconnect_stress.js (in
sspi-client directory you git cloned above), find the following lines and tweak
the relative paths to Tedious so they match the location of Tedious on your
machine.

```JavaScript
const Connection = require('../../../../src/tedious/src/tedious').Connection;
const Request = require('../../../../src/tedious/src/tedious').Request;
```

### Run Tests
These tests can take a few seconds. 

<code>
node sqlconnect_stress.js

node sqlconnect_windows_integrated_auth.js
</code>
