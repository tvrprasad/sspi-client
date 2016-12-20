#pragma once

#define SECURITY_WIN32

#include <memory>
#include <Windows.h>
#include <Sspi.h>
#include <string>
#include <vector>

// This class has the core SSPI client implementation. This has no dependencies on
// V8 or libuv. All code in this class runs in the worker threads. It's upto the
// caller to ensure thread-safety.
class SspiImpl
{
public:
    SspiImpl(const char* spn, const char* securityPackage);

    static SECURITY_STATUS Initialize(
        std::vector<std::string>* availablePackages,
        int* defaultPackageIndex,
        std::string* errorString);

    // Callee creates the outBlob.
    // Caller owns the lifetime of outBlob.
    // Caller deletes outBlob by invoking FreeBlob().
    //  - Caller invoking FreeBlob() vs invoking delete directly decouples
    //    the caller and SspiImpl. This way caller does not need to know if
    //    SspiImpl did this allocation with malloc or free or some other means.
    SECURITY_STATUS GetNextBlob(
        const char* inBlob,
        int inBlobLength,
        char** outBlob,
        int* outBlobLength,
        bool* isDone,
        std::string* errorString);

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

    static HRESULT ConvertUtf8ToMultiByte(
        const char* paramName,
        const char* utf8Str,
        std::unique_ptr<WCHAR[]>* multiByteStr,
        char* errorString,
        int errorStringBufferSize);

    void DeleteCredHandle();
    void DeleteCtxtHandle();

    static const int c_maxPackageNameLength = 32;
    static const int s_numSupportedPackages = 3;
    static WCHAR s_supportedPackages[s_numSupportedPackages][c_maxPackageNameLength];
    static char s_supportedPackagesUtf8[s_numSupportedPackages][c_maxPackageNameLength];

    static const WCHAR* s_defaultPackage;
    static int s_packageMaxTokenSize;

    static const int c_errorStringBufferSize = 256;

    CredHandle m_credHandle;
    CtxtHandle m_ctxtHandle;

    std::string m_spn;
    std::unique_ptr<WCHAR[]> m_spnMultiByte;

    std::string m_securityPackage;
    std::unique_ptr<WCHAR[]> m_securityPackageMultiByte;

    // Everything below is for unit testing purposes only.
    SECURITY_STATUS UtSetCannedResponse(
        const char* inBlob,
        int inBlobLength,
        char** outBlob,
        int* outBlobLength,
        bool* isDone,
        std::string* errorString);

    bool m_utEnableCannedResponse;
    bool m_utForceCompleteAuth;
};
