#include "sspi_impl.h"

#include "utils.h"

int SspiImpl::s_numSupportedPackages = 0;
char SspiImpl::s_supportedPackages[3][SspiImpl::c_maxPackageNameLength];

// static
SECURITY_STATUS SspiImpl::Initialize(
    char* errorString,
    int errorStringBufferSize)
{
    DebugLog("%d: Worker thread: SspiImpl::Initialize.\n", GetCurrentThreadId());

    strncpy(s_supportedPackages[0], "Negotiate", c_maxPackageNameLength);
    strncpy(s_supportedPackages[1], "Kerberos", c_maxPackageNameLength);
    strncpy(s_supportedPackages[2], "NTLM", c_maxPackageNameLength);

    s_numSupportedPackages = 3;

    SECURITY_STATUS securityStatus = SEC_E_OK;
    errorString[0] = '\0';

    return securityStatus;
}

// static
SECURITY_STATUS SspiImpl::GetNextBlob(
    const char* serverName,
    char** blob,
    int* blobLength,
    bool* isDone,
    char* errorString,
    int errorStringBufferSize)
{
    DebugLog("%d: Worker thread: SspiImpl::GetNextBlob. serverName=%s.\n", GetCurrentThreadId(), serverName);

    *blobLength = 25;
    *blob = new char[*blobLength];
    for (int i = 0; i < *blobLength; i++)
    {
        (*blob)[i] = i;
    }

    *isDone = true;

    SECURITY_STATUS securityStatus = SEC_E_INTERNAL_ERROR;
    strncpy(errorString, "ErrorOutOfPaper", errorStringBufferSize);

    return securityStatus;
}

// static
void SspiImpl::FreeBlob(char* blob)
{
    DebugLog("%d: Garbage Collection Thread: SspiImpl::FreeBlob.\n", GetCurrentThreadId());
    delete[] blob;
}
