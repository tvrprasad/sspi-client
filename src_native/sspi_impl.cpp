#include "sspi_impl.h"

#include "utils.h"

#include <stdio.h>

#pragma comment(lib, "secur32.lib")

// This is in the prioritized order in terms of which package to use. The
// first package from this list that's supported by the client OS will be
// used to connect to the server.
char SspiImpl::s_supportedPackages[s_numSupportedPackages][SspiImpl::c_maxPackageNameLength] =
{
    "Negotiate",
    "Kerberos",
    "NTLM"
};

// This is the index of the security package to use if none specified by the app.
int SspiImpl::s_defaultPackageIndex = SspiImpl::c_invalidPackageIndex;

// Maximum token size for the default package selected.
int SspiImpl::s_defaultPackageMaxTokenSize = -1;

// For each package, indicates if the package is supported on the client platform.
bool SspiImpl::s_packageSupported[s_numSupportedPackages] = { false, false, false };

SspiImpl::SspiImpl(const char* spn) :
    m_hasCredHandle(false),
    m_hasCtxtHandle(false),
    m_spn(spn),
    m_utEnableCannedResponse(false)
{
    DebugLog("%d: Main event loop: SspiImpl::SspiImpl: spn=%s", GetCurrentThreadId(), spn);
    SecInvalidateHandle(&m_credHandle);
}

// static
SECURITY_STATUS SspiImpl::Initialize(
    char* sspiPackageName,
    int sspiPackageNameBufferSize,
    char* errorString,
    int errorStringBufferSize)
{
    DebugLog("%d: Worker thread: SspiImpl::Initialize.\n", GetCurrentThreadId());

    errorString[0] = '\0';
    sspiPackageName[0] = '\0';

    unsigned long numPackages;
    PSecPkgInfoA psecPkgInfo;

    SECURITY_STATUS securityStatus = EnumerateSecurityPackagesA(&numPackages, &psecPkgInfo);
    if (securityStatus != SEC_E_OK)
    {
        snprintf(
            errorString,
            errorStringBufferSize,
            "EnumerateSecurityPackagesA failed with error code: %ld.",
            securityStatus);

        return securityStatus;
    }

    for (unsigned long i = 0; i < s_numSupportedPackages; i++)
    {
        if (_strcmpi(s_supportedPackages[i], psecPkgInfo[i].Name) == 0)
        {
            if (s_defaultPackageIndex == c_invalidPackageIndex)
            {
                s_defaultPackageIndex = i;
                s_defaultPackageMaxTokenSize = psecPkgInfo[i].cbMaxToken;
            }

            s_packageSupported[i] = true;
        }
    }

    securityStatus = FreeContextBuffer(psecPkgInfo);
    if (securityStatus != SEC_E_OK)
    {
        snprintf(
            errorString,
            errorStringBufferSize,
            "FreeContextBuffer call failed with error code: %ld.",
            securityStatus);

        return securityStatus;
    }

    if (s_defaultPackageIndex == -1)
    {
        snprintf(
            errorString,
            errorStringBufferSize,
            "No supported security package (%s, %s or %s) available on client.",
            s_supportedPackages[0],
            s_supportedPackages[1],
            s_supportedPackages[2]);

        return securityStatus;
    }

    strncpy(sspiPackageName, s_supportedPackages[s_defaultPackageIndex], sspiPackageNameBufferSize);
    return securityStatus;
}

SECURITY_STATUS SspiImpl::GetNextBlob(
    const char* inBlob,
    int inBlobLength,
    char** outBlob,
    int* outBlobLength,
    bool* isDone,
    char* errorString,
    int errorStringBufferSize)
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
            errorString,
            errorStringBufferSize);
    }

    *outBlob = new char[s_defaultPackageMaxTokenSize];
    errorString[0] = '\0';

    TimeStamp timeExpiry;
    SECURITY_STATUS securityStatus;

    if (!m_hasCredHandle)
    {
        securityStatus = AcquireCredentialsHandleA(
            nullptr,    // Principal - logged in user.
            s_supportedPackages[s_defaultPackageIndex],     // Security package to use.
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
                errorString,
                errorStringBufferSize,
                "AcquireCredentialsHandleA failed with error code: %ld.",
                securityStatus);

            return securityStatus;
        }

        m_hasCredHandle = true;
    }

    size_t spnCopySize = m_spn.length() + 1;
    char* spnCopy = static_cast<char*>(_alloca(spnCopySize));
    strncpy(spnCopy, m_spn.c_str(), spnCopySize);

    SecBuffer inSecBuffer;
    inSecBuffer.BufferType = SECBUFFER_TOKEN;
    inSecBuffer.cbBuffer = inBlobLength;
    inSecBuffer.pvBuffer = const_cast<char*>(inBlob);

    SecBufferDesc inSecBufferDesc;
    inSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    inSecBufferDesc.pBuffers = &inSecBuffer;

    if (m_hasCtxtHandle) {
        inSecBufferDesc.cBuffers = 1;
    }
    else
    {
        inSecBufferDesc.cBuffers = 0;
    }

    SecBuffer outSecBuffer;
    outSecBuffer.BufferType = SECBUFFER_TOKEN;
    outSecBuffer.pvBuffer = *outBlob;
    outSecBuffer.cbBuffer = s_defaultPackageMaxTokenSize;

    SecBufferDesc outSecBufferDesc;
    outSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    outSecBufferDesc.cBuffers = 1;
    outSecBufferDesc.pBuffers = &outSecBuffer;

    ULONG contextAttr;

    securityStatus = InitializeSecurityContextA(
        &m_credHandle,      // Credential handle.
        m_hasCtxtHandle ? &m_ctxtHandle : nullptr,      // Context handle - input.
        spnCopy,    // Service Principal name (SPN).
        ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH | ISC_REQ_INTEGRITY | ISC_REQ_EXTENDED_ERROR,
                    // Context bit flags.
        0,          // Reserved - unused.
        SECURITY_NATIVE_DREP,       // Target data representation.
        m_hasCtxtHandle ? &inSecBufferDesc : nullptr,   // Input buffer, has data from server.
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
            errorString,
            errorStringBufferSize,
            "InitializeSecurityContextA failed with error code: %ld.",
            securityStatus);

        return securityStatus;
    }

    m_hasCtxtHandle = true;

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
                errorString,
                errorStringBufferSize,
                "CompleteAuthToken failed with error code: %ld.",
                securityStatus);

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

SspiImpl::~SspiImpl()
{
    DebugLog("%d: Garbage Collection Thread: SspiImpl::~SspiImpl.\n", GetCurrentThreadId());

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
    }
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
    char* errorString,
    int errorStringBufferSize)
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

        strncpy(errorString, "Canned Response without input data.", errorStringBufferSize);
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

        strncpy(errorString, "Canned Response with input data.", errorStringBufferSize);
        return SEC_E_TARGET_UNKNOWN;    // 0x80090303L
    }
}
