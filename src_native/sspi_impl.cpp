#include "node_version_support.h"
#ifdef IS_SUPPORTED_NODE_VERSION

#include "sspi_impl.h"

#include "utils.h"

#include <stdio.h>

#pragma comment(lib, "secur32.lib")

// This is in the prioritized order in terms of which package to use. The
// first package from this list that's supported by the client OS will be
// used to connect to the server.
WCHAR SspiImpl::s_supportedPackages[s_numSupportedPackages][SspiImpl::c_maxPackageNameLength] =
{
    L"Negotiate",
    L"Kerberos",
    L"NTLM"
};

// This should have 1-1 correspondence with s_supportedPackages above. This is to simplify
// returning available packages in Initialize function.
char SspiImpl::s_supportedPackagesUtf8[s_numSupportedPackages][SspiImpl::c_maxPackageNameLength] =
{
    "Negotiate",
    "Kerberos",
    "NTLM"
};

// This is the default security package to use if none specified by the app.
const WCHAR* SspiImpl::s_defaultPackage = nullptr;

// Maximum token size across all packages.
int SspiImpl::s_packageMaxTokenSize = -1;

SspiImpl::SspiImpl(const char* spn, const char* securityPackage) :
    m_spn(spn),
    m_spnMultiByte(),
    m_securityPackage(),
    m_securityPackageMultiByte(),
    m_utEnableCannedResponse(false),
    m_utForceCompleteAuth(false)
{
    DebugLog("%d: Main event loop: SspiImpl::SspiImpl: spn=%s", GetCurrentThreadId(), spn);
    SecInvalidateHandle(&m_credHandle);
    SecInvalidateHandle(&m_ctxtHandle);

    if (securityPackage != nullptr)
    {
        m_securityPackage.assign(securityPackage);
    }
}

// static
SECURITY_STATUS SspiImpl::Initialize(
    std::vector<std::string>* availablePackages,
    int* defaultPackageIndex,
    std::string* errorString)
{
    DebugLog("%d: Worker thread: SspiImpl::Initialize.\n", GetCurrentThreadId());

    errorString->assign("");
    char errorStringLocal[c_errorStringBufferSize];

    unsigned long numPackages;
    PSecPkgInfoW psecPkgInfo;

    SECURITY_STATUS securityStatus = EnumerateSecurityPackagesW(&numPackages, &psecPkgInfo);
    if (securityStatus != SEC_E_OK)
    {
        snprintf(
            errorStringLocal,
            c_errorStringBufferSize,
            "EnumerateSecurityPackagesW failed with error code: 0x%X.",
            securityStatus);

        errorString->assign(errorStringLocal);
        return securityStatus;
    }

    for (unsigned long supportedPackagesIndex = 0; supportedPackagesIndex < s_numSupportedPackages; supportedPackagesIndex++)
    {
        for (unsigned long packagesIndex = 0; packagesIndex < numPackages; packagesIndex++)
        {
            if (_wcsicmp(s_supportedPackages[supportedPackagesIndex], psecPkgInfo[packagesIndex].Name) == 0)
            {
                availablePackages->push_back(s_supportedPackagesUtf8[supportedPackagesIndex]);
                if (s_packageMaxTokenSize < static_cast<int>(psecPkgInfo[packagesIndex].cbMaxToken))
                {
                    s_packageMaxTokenSize = psecPkgInfo[packagesIndex].cbMaxToken;
                }

                if (s_defaultPackage == nullptr)
                {
                    s_defaultPackage = s_supportedPackages[supportedPackagesIndex];
                    *defaultPackageIndex = static_cast<int>(availablePackages->size() - 1);
                }
            }
        }
    }

    securityStatus = FreeContextBuffer(psecPkgInfo);
    if (securityStatus != SEC_E_OK)
    {
        snprintf(
            errorStringLocal,
            c_errorStringBufferSize,
            "FreeContextBuffer call failed with error code: 0x%X.",
            securityStatus);

        errorString->assign(errorStringLocal);
        return securityStatus;
    }

    if (s_defaultPackage == nullptr)
    {
        snprintf(
            errorStringLocal,
            c_errorStringBufferSize,
            "No supported security package (%s, %s or %s) available on client.",
            s_supportedPackagesUtf8[0],
            s_supportedPackagesUtf8[1],
            s_supportedPackagesUtf8[2]);

        errorString->assign(errorStringLocal);
        return securityStatus;
    }

    return securityStatus;
}

SECURITY_STATUS SspiImpl::GetNextBlob(
    const char* inBlob,
    int inBlobLength,
    char** outBlob,
    int* outBlobLength,
    bool* isDone,
    std::string* errorString)
{
    DebugLog("%d: Worker thread: SspiImpl::GetNextBlob.\n", GetCurrentThreadId());

    if (m_utEnableCannedResponse)
    {
        return UtSetCannedResponse(
            inBlob,
            inBlobLength,
            outBlob,
            outBlobLength,
            isDone,
            errorString);
    }

    errorString->assign("");

    char errorStringLocal[c_errorStringBufferSize];

    // Lifetime owned by caller. See comments in the header file for details.
    *outBlob = new char[s_packageMaxTokenSize];
    TimeStamp timeExpiry;
    SECURITY_STATUS securityStatus;

    if (!SecIsValidHandle(&m_credHandle))
    {
        securityStatus = ConvertUtf8ToMultiByte(
            "spn",
            m_spn.c_str(),
            &m_spnMultiByte,
            errorStringLocal,
            c_errorStringBufferSize);

        if (securityStatus != S_OK)
        {
            errorString->assign(errorStringLocal);
            return securityStatus;
        }

        const WCHAR* securityPackage;
        if (m_securityPackage.empty())
        {
            securityPackage = s_defaultPackage;
        }
        else
        {
            securityStatus = ConvertUtf8ToMultiByte(
                "securityPackage",
                m_securityPackage.c_str(),
                &m_securityPackageMultiByte,
                errorStringLocal,
                c_errorStringBufferSize);

            if (securityStatus != S_OK)
            {
                errorString->assign(errorStringLocal);
                return securityStatus;
            }

            securityPackage = m_securityPackageMultiByte.get();
        }

        securityStatus = AcquireCredentialsHandleW(
            nullptr,    // Principal - logged in user.
            const_cast<WCHAR*>(securityPackage),     // Security package to use.
            SECPKG_CRED_OUTBOUND,   // Client credential token sent to server.
            nullptr,    // Locally unique user identifier.
            nullptr,    // Auth data - use default credentials.
            nullptr,    // pGetKeyFn - unused.
            nullptr,    // pGetKeyArgument - unused.
            &m_credHandle,    // Credential handle.
            &timeExpiry);

        if (securityStatus != SEC_E_OK)
        {
            snprintf(
                errorStringLocal,
                c_errorStringBufferSize,
                "AcquireCredentialsHandleW failed with error code: 0x%X.",
                securityStatus);

            errorString->assign(errorStringLocal);
            return securityStatus;
        }
    }

    SecBuffer inSecBuffer;
    inSecBuffer.BufferType = SECBUFFER_TOKEN;
    inSecBuffer.cbBuffer = inBlobLength;
    inSecBuffer.pvBuffer = const_cast<char*>(inBlob);

    SecBufferDesc inSecBufferDesc;
    inSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    inSecBufferDesc.pBuffers = &inSecBuffer;
    inSecBufferDesc.cBuffers = 1;

    SecBuffer outSecBuffer;
    outSecBuffer.BufferType = SECBUFFER_TOKEN;
    outSecBuffer.pvBuffer = *outBlob;
    outSecBuffer.cbBuffer = s_packageMaxTokenSize;

    SecBufferDesc outSecBufferDesc;
    outSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    outSecBufferDesc.cBuffers = 1;
    outSecBufferDesc.pBuffers = &outSecBuffer;

    ULONG contextAttr;

    securityStatus = InitializeSecurityContextW(
        &m_credHandle,      // Credential handle.
        SecIsValidHandle(&m_ctxtHandle) ? &m_ctxtHandle : nullptr,      // Context handle - input.
        const_cast<WCHAR*>(m_spnMultiByte.get()),    // Service Principal name (SPN).
        ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH | ISC_REQ_INTEGRITY | ISC_REQ_EXTENDED_ERROR,
                    // Context bit flags.
        0,          // Reserved - unused.
        SECURITY_NATIVE_DREP,       // Target data representation.
        SecIsValidHandle(&m_ctxtHandle) ? &inSecBufferDesc : nullptr,   // Input buffer, has data from server.
        0,          // Reserved - unused.
        &m_ctxtHandle,      // Context handle - output.
        &outSecBufferDesc,  // Output buffer, data to send to server.
        &contextAttr,       // Context attributes - unused.
        &timeExpiry);

    if (securityStatus != SEC_E_OK
        && securityStatus != SEC_I_CONTINUE_NEEDED
        && securityStatus != SEC_I_COMPLETE_AND_CONTINUE
        && securityStatus != SEC_I_COMPLETE_NEEDED)
    {
        snprintf(
            errorStringLocal,
            c_errorStringBufferSize,
            "InitializeSecurityContextW failed with error code: 0x%X.",
            securityStatus);

        errorString->assign(errorStringLocal);
        return securityStatus;
    }

    *isDone = securityStatus != SEC_I_CONTINUE_NEEDED
        && securityStatus != SEC_I_COMPLETE_AND_CONTINUE;

    if (m_utForceCompleteAuth
        || securityStatus == SEC_I_COMPLETE_NEEDED
        || securityStatus == SEC_I_COMPLETE_AND_CONTINUE)
    {
        securityStatus = CompleteAuthToken(&m_ctxtHandle, &outSecBufferDesc);
        if (securityStatus != SEC_E_OK)
        {
            snprintf(
                errorStringLocal,
                c_errorStringBufferSize,
                "CompleteAuthToken failed with error code: 0x%X.",
                securityStatus);

            errorString->assign(errorStringLocal);
            return securityStatus;
        }
    }

    *outBlobLength = outSecBuffer.cbBuffer;

    return 0;
}

// static
void SspiImpl::FreeBlob(char* blob)
{
    DebugLog("%d: Garbage Collection Thread: SspiImpl::FreeBlob.\n", GetCurrentThreadId());
    delete[] blob;
}

// static
HRESULT SspiImpl::ConvertUtf8ToMultiByte(
    const char* paramName,
    const char* utf8Str,
    std::unique_ptr<WCHAR[]>* multiByteStr,
    char* errorString,
    int errorStringBufferSize)
{
    int retval = MultiByteToWideChar(
        CP_UTF8,                    // Code page UTF8.
        MB_ERR_INVALID_CHARS,       // Fail on invalid characters.
        utf8Str,                    // UTF8 string.
        -1,                         // Indicates null-termination.
        nullptr,                    // Output buffer, ignored when getting required buffer size.
        0);                         // Output buffer size, in characters set to 0 to get required buffer size.

    if (!retval) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        snprintf(
            errorString,
            errorStringBufferSize,
            "MultiByteToWideChar failed to get required buffer size for '%s'. Error code: 0x%X.",
            paramName,
            hr);

        return hr;
    }

    // retval includes null space for terminating null character.
    multiByteStr->reset(new WCHAR[retval]);

    retval = MultiByteToWideChar(
        CP_UTF8,                    // Code page UTF8.
        MB_ERR_INVALID_CHARS,       // Fail on invalid characters.
        utf8Str,                    // UTF8 string.
        -1,                         // Indicates null-termination.
        multiByteStr->get(),        // Output buffer.
        retval);                    // Output buffer size, in characters set to 0 to get required buffer size.

    if (!retval) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        snprintf(
            errorString,
            errorStringBufferSize,
            "MultiByteToWideChar failed to convert UTF8 to WideChar for '%s'. Error code: 0x%X.",
            paramName,
            hr);

        return hr;
    }

    return S_OK;
}

void SspiImpl::DeleteCredHandle()
{
    if (SecIsValidHandle(&m_credHandle))
    {
        SECURITY_STATUS securityStatus = FreeCredentialHandle(&m_credHandle);
        if (securityStatus != SEC_E_OK)
        {
            DebugLog(
                "%d: FreeCredentialHandle failed with error code: %ld.\n",
                GetCurrentThreadId(),
                securityStatus);
        }

        SecInvalidateHandle(&m_credHandle);
    }
}

void SspiImpl::DeleteCtxtHandle()
{
    if (SecIsValidHandle(&m_ctxtHandle))
    {
        SECURITY_STATUS securityStatus = DeleteSecurityContext(&m_ctxtHandle);
        if (securityStatus != SEC_E_OK)
        {
            DebugLog(
                "%d: DeleteSecurityContext failed with error code: %ld.\n",
                GetCurrentThreadId(),
                securityStatus);
        }

        SecInvalidateHandle(&m_ctxtHandle);
    }
}

SspiImpl::~SspiImpl()
{
    DebugLog("%d: Garbage Collection Thread: SspiImpl::~SspiImpl.\n", GetCurrentThreadId());
    DeleteCtxtHandle();
    DeleteCredHandle();
}

void SspiImpl::UtEnableCannedResponse(bool enable)
{
    m_utEnableCannedResponse = enable;
}

void SspiImpl::UtForceCompleteAuth(bool force)
{
    m_utForceCompleteAuth = force;
}

SECURITY_STATUS SspiImpl::UtSetCannedResponse(
    const char* inBlob,
    int inBlobLength,
    char** outBlob,
    int* outBlobLength,
    bool* isDone,
    std::string* errorString)
{
    if (!inBlobLength)
    {
        const int c_outBlobLength = 25;
        *outBlob = new char[c_outBlobLength];
        *outBlobLength = c_outBlobLength;
        for (int i = 0; i < c_outBlobLength; i++)
        {
            (*outBlob)[i] = i;
        }

        *isDone = true;

        errorString->assign("Canned Response without input data.");
        return SEC_E_INTERNAL_ERROR;    // 0x80090304L
    }
    else
    {
        *outBlob = new char[inBlobLength];
        *outBlobLength = inBlobLength;
        for (int i = 0; i < inBlobLength; i++)
        {
            (*outBlob)[i] = inBlob[i];
        }

        *isDone = false;

        errorString->assign("Canned Response with input data.");
        return SEC_E_TARGET_UNKNOWN;    // 0x80090303L
    }
}

#endif  // IS_SUPPORTED_NODE_VERSION
