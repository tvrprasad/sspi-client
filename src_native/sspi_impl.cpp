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

// This is the default security package to use if none specified by the app.
const char* SspiImpl::s_defaultPackage = nullptr;

// Maximum token size across all packages.
int SspiImpl::s_packageMaxTokenSize = -1;

SspiImpl::SspiImpl(const char* spn, const char* securityPackage) :
    m_hasCredHandle(false),
    m_hasCtxtHandle(false),
    m_spn(spn),
    m_securityPackage(),
    m_utEnableCannedResponse(false),
    m_utForceCompleteAuth(false)
{
    DebugLog("%d: Main event loop: SspiImpl::SspiImpl: spn=%s", GetCurrentThreadId(), spn);
    SecInvalidateHandle(&m_credHandle);

    if (securityPackage != nullptr)
    {
        m_securityPackage.assign(securityPackage);
    }
}

// static
SECURITY_STATUS SspiImpl::Initialize(
    std::vector<std::string>* availablePackages,
    int* defaultPackageIndex,
    char* errorString,
    int errorStringBufferSize)
{
    DebugLog("%d: Worker thread: SspiImpl::Initialize.\n", GetCurrentThreadId());

    errorString[0] = '\0';

    unsigned long numPackages;
    PSecPkgInfoA psecPkgInfo;

    SECURITY_STATUS securityStatus = EnumerateSecurityPackagesA(&numPackages, &psecPkgInfo);
    if (securityStatus != SEC_E_OK)
    {
        snprintf(
            errorString,
            errorStringBufferSize,
            "EnumerateSecurityPackagesA failed with error code: 0x%X.",
            securityStatus);

        return securityStatus;
    }

    for (unsigned long supportedPackagesIndex = 0; supportedPackagesIndex < s_numSupportedPackages; supportedPackagesIndex++)
    {
        for (unsigned long packagesIndex = 0; packagesIndex < numPackages; packagesIndex++)
        {
            if (_strcmpi(s_supportedPackages[supportedPackagesIndex], psecPkgInfo[packagesIndex].Name) == 0)
            {
                availablePackages->push_back(psecPkgInfo[packagesIndex].Name);
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
            errorString,
            errorStringBufferSize,
            "FreeContextBuffer call failed with error code: 0x%X.",
            securityStatus);

        return securityStatus;
    }

    if (s_defaultPackage == nullptr)
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

    *outBlob = new char[s_packageMaxTokenSize];
    errorString[0] = '\0';

    TimeStamp timeExpiry;
    SECURITY_STATUS securityStatus;

    if (!m_hasCredHandle)
    {
        const char* securityPackage;
        if (m_securityPackage.empty())
        {
            securityPackage = s_defaultPackage;
        }
        else
        {
            securityPackage = m_securityPackage.c_str();
        }

        securityStatus = AcquireCredentialsHandleA(
            nullptr,    // Principal - logged in user.
            const_cast<char*>(securityPackage),     // Security package to use.
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
                "AcquireCredentialsHandleA failed with error code: 0x%X.",
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
    outSecBuffer.cbBuffer = s_packageMaxTokenSize;

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
            "InitializeSecurityContextA failed with error code: 0x%X.",
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
                "CompleteAuthToken failed with error code: 0x%X.",
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
