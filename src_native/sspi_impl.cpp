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
    m_spn(spn),
    m_securityPackage(),
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

    const int c_errorStringBufferSize = 256;
    char errorStringLocal[c_errorStringBufferSize];

    unsigned long numPackages;
    PSecPkgInfoA psecPkgInfo;

    SECURITY_STATUS securityStatus = EnumerateSecurityPackagesA(&numPackages, &psecPkgInfo);
    if (securityStatus != SEC_E_OK)
    {
        snprintf(
            errorStringLocal,
            c_errorStringBufferSize,
            "EnumerateSecurityPackagesA failed with error code: 0x%X.",
            securityStatus);

        errorString->assign(errorStringLocal);
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
            s_supportedPackages[0],
            s_supportedPackages[1],
            s_supportedPackages[2]);

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

    const int c_errorStringBufferSize = 256;
    char errorStringLocal[c_errorStringBufferSize];

    // Lifetime owned by caller. See comments in the header file for details.
    *outBlob = new char[s_packageMaxTokenSize];
    TimeStamp timeExpiry;
    SECURITY_STATUS securityStatus;

    if (!SecIsValidHandle(&m_credHandle))
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
                errorStringLocal,
                c_errorStringBufferSize,
                "AcquireCredentialsHandleA failed with error code: 0x%X.",
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

    securityStatus = InitializeSecurityContextA(
        &m_credHandle,      // Credential handle.
        SecIsValidHandle(&m_ctxtHandle) ? &m_ctxtHandle : nullptr,      // Context handle - input.
        const_cast<char*>(m_spn.c_str()),    // Service Principal name (SPN).
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
            "InitializeSecurityContextA failed with error code: 0x%X.",
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

void::SspiImpl::DeleteCredHandle()
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
