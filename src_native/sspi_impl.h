#pragma once

#define SECURITY_WIN32

#include <Windows.h>
#include <Sspi.h>
#include <string>

// This class has the core SSPI client implementation. This has no dependencies on
// V8 or libuv. All code in this class runs in the worker threads. It's upto the
// caller to ensure thread-safety.
class SspiImpl
{
public:
    explicit SspiImpl(const char*);

    static SECURITY_STATUS Initialize(
        char* sspiPackageName,
        int sspiPackageNameBufferSize,
        char* errorString,
        int errorStringBufferSize);

    // Callee creates the outBlob.
    // Caller owns the lifetime of outBlob.
    // Caller deletes outBlob by invoking FreeBlob().
    SECURITY_STATUS GetNextBlob(
        const char* inBlob,
        int inBlobLength,
        char** outBlob,
        int* outBlobLength,
        bool* isDone,
        char* errorString,
        int errorStringBufferSize);

    // Call triggered by JavaScript garbage collector.
    static void FreeBlob(char* blob);

    ~SspiImpl();

    // Everything below is for unit testing purposes only.
    void UtEnableCannedResponse(bool enable);
    void UtForceCompleteAuth(bool force);
private:
    // Not implemented. This class should never be instantiated.
    SspiImpl(const SspiImpl&);
    SspiImpl& operator=(const SspiImpl&);

    static const int c_maxPackageNameLength = 32;
    static const int s_numSupportedPackages = 3;
    static char s_supportedPackages[s_numSupportedPackages][c_maxPackageNameLength];
    static bool s_packageSupported[s_numSupportedPackages];

    static const int c_invalidPackageIndex = -1;
    static int s_defaultPackageIndex;
    static int s_defaultPackageMaxTokenSize;

    CredHandle m_credHandle;
    bool m_hasCredHandle;

    CtxtHandle m_ctxtHandle;
    bool m_hasCtxtHandle;

    std::string m_spn;

    // Everything below is for unit testing purposes only.
    SECURITY_STATUS UtSetCannedResponse(
        const char* inBlob,
        int inBlobLength,
        char** outBlob,
        int* outBlobLength,
        bool* isDone,
        char* errorString,
        int errorStringBufferSize);

    bool m_utEnableCannedResponse;
    bool m_utForceCompleteAuth;
};
