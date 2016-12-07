# Manual Integration Test

This is a manual integration test. Below are the steps:

- Load sspi_test_server\sspi_test_server.sln in Visual Stuido and build.
- Open two console windows.
- Run sspi_test_server.exe you build above.
- Run 'node integration\sspi-client-test.js' in the other console window.

## Expected Output

### Server console window:

You should see something like this:

===============================================
Waiting for client to connect...
Listening!

Token buffer received (128 bytes):
Token buffer generated (349 bytes): isDone=false
AcceptSecurityContext result = 0x00090312

Token buffer received (121 bytes):
Token buffer generated (29 bytes): isDone=true
AcceptSecurityContext result = 0x00000000

===============================================
Waiting for client to connect...
Listening!

### Client console window:

You should see something like this:

Connected to server.
spn:  MSSQLSvc/prasadtammana.redmond.corp.microsoft.com:2000

Security status = 590610
Token buffer generated ( 128 bytes): isDone= false
Token buffer received ( 349 bytes):

Security status = 590610
Token buffer generated ( 121 bytes): isDone= false
Token buffer received ( 29 bytes):

Security status = 0
Token buffer generated ( 0 bytes): isDone= true

Authentication succeeded. Security package =  Negotiate
