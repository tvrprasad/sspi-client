#pragma once

#include <Windows.h>

// This class has the core SSPI client implementation. This has no dependencies on
// V8 or libuv. All code in this class runs in the worker threads. It's upto the
// caller to ensure thread-safety.
class SspiImpl
{
public:
    static SECURITY_STATUS Initialize(
        char* errorString,
        int errorStringBufferSize);

    // Callee creates the blob.
    // Caller owns the lifetime.
    // Caller deletes blob by invoking FreeBlob().
    static SECURITY_STATUS GetNextBlob(
        const char* serverName,
        char** blob,
        int* blobLength,
        bool* isDone,
        char* errorString,
        int errorStringBufferSize);

    // Call triggered JavaScript garbage collector.
    static void FreeBlob(char* blob);


private:
    // Not implemented. This class should never be instantiated.
    SspiImpl();
    ~SspiImpl();
    SspiImpl(const SspiImpl&);
    SspiImpl& operator=(const SspiImpl&);

    static const int c_maxPackageNameLength = 32;
    static int s_numSupportedPackages;
    static char s_supportedPackages[3][c_maxPackageNameLength];
};
