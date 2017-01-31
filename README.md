# Client Side SSPI Authentication Module.

## Overview
SSPI is a Microsoft specific API that may be used by applications for
authenticated communications. This allows an application to use various
available security modules without changing the interface to the security
system. The actual security model is implemented by security packages installed
on the system. For more information, see [SSPI][].

Windows implementation of SSPI is in native code, making it available only for
C/C++ applications. sspi-client module provides a JavaScript interface for
applications that need to communicate with a server using SSPI. Primary
motivitation for building this module is to help implement Windows Integrated
Authentication in [Tedious][].

This is currently only supported on Windows and for Node version > 4.0.0.

## API Documentation
Below is the API listing with brief optional descriptions. Refer to comments on
the corresponding functions and classes in code.
### sspi_client
#### SspiClient Class
This class has the core functionality implemented by the module.
##### constructor
```JavaScript
var sspiClient = new SspiClientApi.SspiClient(spn, securityPackage);
````
You may get __spn__ by invoking <code>makeSpn()</code> which takes an FQDN. If
you only have simple hostname or IP address, you may get FQDN by invoking
<code>getFqdn()</code> and then pass it to makeSpn.
##### getNextBlob
```JavaScript
SspiClient.getNextBlob(serverResponse, serverResponseBeginOffset, serverResponseLength, cb)
```
This function takes the server response and makes SSPI calls to get the client
response to send back to the server. You can use just this function to
implement client side SSPI based authentication. This will do initialization
if needed.
#### ensureInitialization
```JavaScript
ensureInitialization(cb);
```
Do initialization if needed.
#### getAvailableSspiPackageNames
```JavaScript
var availableSspiPackageNames = getAvailableSspiPackageNames();
```
Initialization must be completed before this function may be invoked.
#### getDefaultSspiPackageName
```JavaScript
var defaultPackageName = getDefaultSspiPackageName();
```
Initialization must be completed before this function may be invoked.
#### enableNativeDebugLogging
```JavaScript
enableNativeDebugging();
```
Logs detailed debug information from native code.
#### disableNativeDebugLogging
```JavaScript
disableNativeDebugLogging();
```
This together with <code>enableNativeDebugging</code> allows for enabling debug
logging for targeted sections of the application.
### fqdn
#### getFqdn
```JavaScript
getFqdn(hostidentifier, cb);
```
Resolves an IP address or hostname to an FQDN.
### make_spn
#### makeSpn
```JavaScript
var spn = makeSpn(serviceClassName, fqdn, instanceNameOrPort;
```
Puts together the parameters passed in return the Service Principal Name.
## Sample code
For a complete sample, see [Sample Code][].
## Developer Notes
This section has notes for developers to be able to build and run tests.
### Setup and Build
Install [NodeJS][]. Duh!  
<code>npm install -g node-gyp</code>  
git clone https://github.com/tvrprasad/sspi-client.git  
cd sspi-client  
npm install
### Run Tests
#### Setup
Copy [test_config.json][] to %USERPROFILE%\.sspi-client\test_config.json  
Tweak the values in the file to have the right values for yoursetup. Should be
self-explanatory.  This setup is needed for running both unit and integration
tests.
#### Unit Tests
npm run-script test
#### Integration Tests
Integration tests are currently manual but hopefully not too tedious. They test
the functionality end to end. These tests are in the directory
test\integration.
##### sspi_client_test.js
This test sets up a SSPI server and runs SSPI client to connect with it.
Follow instructions in [README_sspi_client_test.md][] to run this test.
##### sqlconnect_windows_integrated_auth.js
This test validates integration with Tedious by attempting to connect and run a
simple query for the following matrix:
1. Two instances of SQL Server, one local and one remote.
2. Supported SSPI protocols - negotiate, kerberos, ntlm.
3. TLS encryption on and off.

Follow instructions in [README_sqlconnect.md] to run this test.
##### sqlconnect_stress.js
This test validates integration with Tedious under stress by attempting to open
about 1000 connections in parallel and run a simple query on each connection,
again in parallel. The mix of connections is as below:
1. Two instances of SQL Server, one local and one remote.
2. Supported SSPI protocols - negotiate, kerberos, ntlm.
3. TLS encryption on and off.

Follow instructions in [README_sqlconnect.md] to run this test.

[SSPI]: https://msdn.microsoft.com/en-us/library/windows/desktop/aa380493(v=vs.85).aspx "SSPI Windows"
[Tedious]: https://github.com/tediousjs/tedious "Node.js implementation of TDS protocol."
[Sample Code]: https://github.com/tvrprasad/sspi-client/blob/master/test/integration/sspi-client-test.js "Sample Code"
[NodeJS]: https://nodejs.org/en/download/current/ "NodeJS Download"
[test_config.json]: https://github.com/tvrprasad/sspi-client/blob/master/test/test_config.json "Test Configuration"
[README_sspi_client_test.md]: https://github.com/tvrprasad/sspi-client/blob/master/test/integration/README_sspi_client_test.md "README_sspi_client_test.md"
[README_sqlconnect.md]: https://github.com/tvrprasad/sspi-client/blob/master/test/integration/README_sqlconnect.md "README_sqlconnect.md"
