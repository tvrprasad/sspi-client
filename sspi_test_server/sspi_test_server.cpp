
//--------------------------------------------------------------------
//  This is a server-side SSPI Windows Sockets program.
//  (Adopted from an MSDN sample).

#define usPort 2000
#define SECURITY_WIN32
#define SEC_SUCCESS(Status) ((Status) >= 0)

#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include "sspi_test_server.h"

CredHandle hcred;
struct _SecHandle  hctxt;

static PBYTE g_pInBuf = NULL;
static PBYTE g_pOutBuf = NULL;
static DWORD g_cbMaxMessage;
static TCHAR g_lpPackageName[1024];

BOOL AcceptAuthSocket(SOCKET *ServerSocket);

void main()
{
	SOCKET Server_Socket;
	WSADATA wsaData;
	SECURITY_STATUS ss;
	PSecPkgInfo pkgInfo;

	//-----------------------------------------------------------------   
	//  Set the default package to negotiate.

	strcpy_s(g_lpPackageName, 1024 * sizeof(TCHAR), "Negotiate");

	//-----------------------------------------------------------------   
	//  Initialize the socket interface and the security package.

	if (WSAStartup(0x0101, &wsaData))
	{
		fprintf(stderr, "Could not initialize winsock: \n");
		cleanup();
	}

	ss = QuerySecurityPackageInfo(
		g_lpPackageName,
		&pkgInfo);

	if (!SEC_SUCCESS(ss))
	{
		fprintf(stderr,
			"Could not query package info for %s, error 0x%08x\n",
			g_lpPackageName, ss);
		cleanup();
	}

	g_cbMaxMessage = pkgInfo->cbMaxToken;

	FreeContextBuffer(pkgInfo);

	g_pInBuf = (PBYTE)malloc(g_cbMaxMessage);
	g_pOutBuf = (PBYTE)malloc(g_cbMaxMessage);

	if (NULL == g_pInBuf || NULL == g_pOutBuf)
	{
		fprintf(stderr, "Memory allocation error.\n");
		cleanup();
	}

	//-----------------------------------------------------------------   
	//  Start looping for clients.

	while (TRUE)
	{
        printf("===============================================\n");
		printf("Waiting for client to connect...\n");

		//-----------------------------------------------------------------   
		//  Make an authenticated connection with client.


		if (!AcceptAuthSocket(&Server_Socket))
		{
			fprintf(stderr, "Could not authenticate the socket.\n");
			cleanup();
		}

		if (Server_Socket)
		{
			DeleteSecurityContext(&hctxt);
			FreeCredentialHandle(&hcred);
			shutdown(Server_Socket, 2);
			closesocket(Server_Socket);
			Server_Socket = 0;
		}
	}  // end while loop

	printf("Server ran to completion without error.\n");
	cleanup();
}  // end main

BOOL AcceptAuthSocket(SOCKET *ServerSocket)
{
	SOCKET sockListen;
	SOCKET sockClient;
	SOCKADDR_IN sockIn;

	//-----------------------------------------------------------------   
	//  Create listening socket.

	sockListen = socket(
		PF_INET,
		SOCK_STREAM,
		0);

	if (INVALID_SOCKET == sockListen)
	{
		fprintf(stderr, "Failed to create socket: %u\n", GetLastError());
		return(FALSE);
	}

	//-----------------------------------------------------------------   
	//  Bind to local port.

	sockIn.sin_family = AF_INET;
	sockIn.sin_addr.s_addr = 0;
	sockIn.sin_port = htons(usPort);

	if (SOCKET_ERROR == bind(
		sockListen,
		(LPSOCKADDR)&sockIn,
		sizeof(sockIn)))
	{
		fprintf(stderr, "bind failed: %u\n", GetLastError());
		return(FALSE);
	}

	//-----------------------------------------------------------------   
	//  Listen for client.

	if (SOCKET_ERROR == listen(sockListen, 1))
	{
		fprintf(stderr, "Listen failed: %u\n", GetLastError());
		return(FALSE);
	}
	else
	{
		printf("Listening!\n\n");
	}

	//-----------------------------------------------------------------   
	//  Accept client.

	sockClient = accept(
		sockListen,
		NULL,
		NULL);

	if (INVALID_SOCKET == sockClient)
	{
		fprintf(stderr, "accept failed: %u\n", GetLastError());
		return(FALSE);
	}

	closesocket(sockListen);

	*ServerSocket = sockClient;


	return(DoAuthentication(sockClient));

}  // end AcceptAuthSocket  

BOOL DoAuthentication(SOCKET AuthSocket)
{
	SECURITY_STATUS   ss;
	DWORD cbIn, cbOut;
	BOOL              done = FALSE;
	TimeStamp         Lifetime;
	BOOL              fNewConversation;

	fNewConversation = TRUE;

	ss = AcquireCredentialsHandle(
		NULL,
		g_lpPackageName,
		SECPKG_CRED_INBOUND,
		NULL,
		NULL,
		NULL,
		NULL,
		&hcred,
		&Lifetime);

	if (!SEC_SUCCESS(ss))
	{
		fprintf(stderr, "AcquireCreds failed: 0x%08x\n", ss);
		return(FALSE);
	}

	while (!done)
	{
		if (!ReceiveMsg(
			AuthSocket,
			g_pInBuf,
			g_cbMaxMessage,
			&cbIn))
		{
			return(FALSE);
		}

		cbOut = g_cbMaxMessage;

		if (!GenServerContext(
			g_pInBuf,
			cbIn,
			g_pOutBuf,
			&cbOut,
			&done,
			fNewConversation))
		{
			fprintf(stderr, "GenServerContext failed.\n");
			return(FALSE);
		}
		fNewConversation = FALSE;
		if (!SendMsg(
			AuthSocket,
			g_pOutBuf,
			cbOut))
		{
			fprintf(stderr, "Sending message failed.\n");
			return(FALSE);
		}
	}

	return(TRUE);
}  // end DoAuthentication

BOOL GenServerContext(
	BYTE *pIn,
	DWORD cbIn,
	BYTE *pOut,
	DWORD *pcbOut,
	BOOL *pfDone,
	BOOL fNewConversation)
{
	SECURITY_STATUS   ss;
	TimeStamp         Lifetime;
	SecBufferDesc     OutBuffDesc;
	SecBuffer         OutSecBuff;
	SecBufferDesc     InBuffDesc;
	SecBuffer         InSecBuff;
	ULONG             Attribs = 0;

	//----------------------------------------------------------------
	//  Prepare output buffers.

	OutBuffDesc.ulVersion = 0;
	OutBuffDesc.cBuffers = 1;
	OutBuffDesc.pBuffers = &OutSecBuff;

	OutSecBuff.cbBuffer = *pcbOut;
	OutSecBuff.BufferType = SECBUFFER_TOKEN;
	OutSecBuff.pvBuffer = pOut;

	//----------------------------------------------------------------
	//  Prepare input buffers.

	InBuffDesc.ulVersion = 0;
	InBuffDesc.cBuffers = 1;
	InBuffDesc.pBuffers = &InSecBuff;

	InSecBuff.cbBuffer = cbIn;
	InSecBuff.BufferType = SECBUFFER_TOKEN;
	InSecBuff.pvBuffer = pIn;

	printf("Token buffer received (%lu bytes):\n", InSecBuff.cbBuffer);
	//PrintHexDump(InSecBuff.cbBuffer, (PBYTE)InSecBuff.pvBuffer);

	ss = AcceptSecurityContext(
		&hcred,
		fNewConversation ? NULL : &hctxt,
		&InBuffDesc,
		Attribs,
		SECURITY_NATIVE_DREP,
		&hctxt,
		&OutBuffDesc,
		&Attribs,
		&Lifetime);

	if (!SEC_SUCCESS(ss))
	{
		fprintf(stderr, "AcceptSecurityContext failed: 0x%08x\n", ss);
		return FALSE;
	}

	//----------------------------------------------------------------
	//  Complete token if applicable.

	if ((SEC_I_COMPLETE_NEEDED == ss)
		|| (SEC_I_COMPLETE_AND_CONTINUE == ss))
	{
        printf("CompleteAuthToken invoked.\n");
		ss = CompleteAuthToken(&hctxt, &OutBuffDesc);
		if (!SEC_SUCCESS(ss))
		{
			fprintf(stderr, "complete failed: 0x%08x\n", ss);
			return FALSE;
		}
	}

	*pcbOut = OutSecBuff.cbBuffer;

	//  fNewConversation equals FALSE.

    *pfDone = !((SEC_I_CONTINUE_NEEDED == ss)
        || (SEC_I_COMPLETE_AND_CONTINUE == ss));

    printf("Token buffer generated (%lu bytes): isDone=%s\n",
		OutSecBuff.cbBuffer, *pfDone ? "true" : "false");
	//PrintHexDump(
	//	OutSecBuff.cbBuffer,
	//	(PBYTE)OutSecBuff.pvBuffer);

	printf("AcceptSecurityContext result = 0x%08x\n\n", ss);

	return TRUE;

}  // end GenServerContext

void PrintHexDump(DWORD length, PBYTE buffer)
{
	DWORD i, count, index;
	CHAR rgbDigits[] = "0123456789abcdef";
	CHAR rgbLine[100];
	char cbLine;

	for (index = 0; length;
		length -= count, buffer += count, index += count)
	{
		count = (length > 16) ? 16 : length;

		sprintf_s(rgbLine, 100, "%4.4x  ", index);
		cbLine = 6;

		for (i = 0;i<count;i++)
		{
			rgbLine[cbLine++] = rgbDigits[buffer[i] >> 4];
			rgbLine[cbLine++] = rgbDigits[buffer[i] & 0x0f];
			if (i == 7)
			{
				rgbLine[cbLine++] = ':';
			}
			else
			{
				rgbLine[cbLine++] = ' ';
			}
		}
		for (; i < 16; i++)
		{
			rgbLine[cbLine++] = ' ';
			rgbLine[cbLine++] = ' ';
			rgbLine[cbLine++] = ' ';
		}

		rgbLine[cbLine++] = ' ';

		for (i = 0; i < count; i++)
		{
			if (buffer[i] < 32 || buffer[i] > 126)
			{
				rgbLine[cbLine++] = '.';
			}
			else
			{
				rgbLine[cbLine++] = buffer[i];
			}
		}

		rgbLine[cbLine++] = 0;
		printf("%s\n", rgbLine);
	}
}  // end PrintHexDump


BOOL SendMsg(
	SOCKET s,
	PBYTE pBuf,
	DWORD cbBuf)
{
	if (0 == cbBuf)
		return(TRUE);

	//----------------------------------------------------------------
	//  Send the size of the message.

	if (!SendBytes(
		s,
		(PBYTE)&cbBuf,
		sizeof(cbBuf)))
	{
		return(FALSE);
	}

	//----------------------------------------------------------------    
	//  Send the body of the message.

	if (!SendBytes(
		s,
		pBuf,
		cbBuf))
	{
		return(FALSE);
	}

	return(TRUE);
} // end SendMsg    

BOOL ReceiveMsg(
	SOCKET s,
	PBYTE pBuf,
	DWORD cbBuf,
	DWORD *pcbRead)
{
	DWORD cbRead;
	DWORD cbData;

	//-----------------------------------------------------------------
	//  Retrieve the number of bytes in the message.

	if (!ReceiveBytes(
		s,
		(PBYTE)&cbData,
		sizeof(cbData),
		&cbRead))
	{
		return(FALSE);
	}

	if (sizeof(cbData) != cbRead)
	{
		return(FALSE);
	}

	//----------------------------------------------------------------
	//  Read the full message.

	if (!ReceiveBytes(
		s,
		pBuf,
		cbData,
		&cbRead))
	{
		return(FALSE);
	}

	if (cbRead != cbData)
	{
		return(FALSE);
	}

	*pcbRead = cbRead;

	return(TRUE);
}  // end ReceiveMsg    

BOOL SendBytes(
	SOCKET s,
	PBYTE pBuf,
	DWORD cbBuf)
{
	PBYTE pTemp = pBuf;
	int cbSent, cbRemaining = cbBuf;

	if (0 == cbBuf)
	{
		return(TRUE);
	}

	while (cbRemaining)
	{
		cbSent = send(
			s,
			(const char *)pTemp,
			cbRemaining,
			0);
		if (SOCKET_ERROR == cbSent)
		{
			fprintf(stderr, "send failed: %u\n", GetLastError());
			return FALSE;
		}

		pTemp += cbSent;
		cbRemaining -= cbSent;
	}

	return TRUE;
}  // end SendBytes

BOOL ReceiveBytes(
	SOCKET s,
	PBYTE pBuf,
	DWORD cbBuf,
	DWORD *pcbRead)
{
	PBYTE pTemp = pBuf;
	int cbRead, cbRemaining = cbBuf;

	while (cbRemaining)
	{
		cbRead = recv(
			s,
			(char *)pTemp,
			cbRemaining,
			0);
		if (0 == cbRead)
		{
			break;
		}

		if (SOCKET_ERROR == cbRead)
		{
			fprintf(stderr, "recv failed: %u\n", GetLastError());
			return FALSE;
		}

		cbRemaining -= cbRead;
		pTemp += cbRead;
	}

	*pcbRead = cbBuf - cbRemaining;

	return TRUE;
}  // end ReceivesBytes

void cleanup()
{
	if (g_pInBuf)
		free(g_pInBuf);

	if (g_pOutBuf)
		free(g_pOutBuf);

	WSACleanup();
	exit(0);
}

