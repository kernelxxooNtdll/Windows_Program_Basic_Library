#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <Shlwapi.h>
#include "SimpleHttpClient.h"
#include <iphlpapi.h>

void TrimString(std::string& str)
{
    std::string::size_type pos = str.find_last_not_of(' ');
    if (pos != std::string::npos) 
    {
        str.erase(pos + 1);
        pos = str.find_first_not_of(' ');
        if (pos != std::string::npos)
        {
            str.erase(0, pos);
        }
    }
    else
    {
        str.erase(str.begin(), str.end());
    }
}

CSimpleHttpClient::CSimpleHttpClient(const std::string& strUrl)
	: m_eError(NoError)
	, m_sockClient(INVALID_SOCKET)
	, m_dwStatus(0)
	, m_nContentLength((UINT)-1)
	, m_nRead(0)
	, m_dwDomainNameParseTime(0)
	, m_dwTCPConnectTime(0)
	, m_dwRecvHeaderTime(0)
{
	ParseUrl(strUrl)
		&& PackRequeset()
		&& ConnectServer()
		&& RecvResponseHeader();
}

CSimpleHttpClient::~CSimpleHttpClient()
{
	if (INVALID_SOCKET != m_sockClient)
	{
		closesocket(m_sockClient);
	}
}

std::string CSimpleHttpClient::GetHeader(const std::string& strName) const
{
	std::map<std::string, std::string>::const_iterator it = m_mapHeader.find(strName);
	if (m_mapHeader.end() == it)
	{
		return std::string();
	}

	return it->second;
}

bool CSimpleHttpClient::ParseUrl(const std::string& strUrl)
{
	std::string str;
	std::string::size_type pos = strUrl.find("//");
	if (std::string::npos != pos)
	{
		// Ö»Ö§³ÖHTTP
		if (0 != StrCmpNIA(strUrl.c_str(), "http://", 7))
		{
			m_eError = NotHttpProtocol;
			return false;
		}
		str = strUrl.substr(7);
	}
	else
	{
		str = strUrl;
	}

	pos = str.find('/');
	m_strHost = str.substr(0, pos);
	if (std::string::npos != pos)
	{
		m_strUrl = str.substr(pos);
	}
	else
	{
		m_strUrl = "/";
	}

	pos = m_strHost.find(':');
	if (std::string::npos != pos)
	{
		std::string strPort = m_strHost.substr(pos + 1);
		m_nPort = atoi(strPort.c_str());
		m_strHost.erase(pos);
	}
	else
	{
		m_nPort = 80;
	}

	return true;
}

bool CSimpleHttpClient::PackRequeset()
{
	m_nRequestSize = sprintf_s(m_szRequest, kMaxRequestSize,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Connection: keep-alive\r\n"
		"Pragma: no-cache\r\n"
		"Cache-Control: no-cache\r\n"
// 		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
// 		"Accept-Encoding: gzip, deflate, sdch\r\n"
// 		"Accept-Language: zh-CN,zh;q=0.8,en;q=0.6\r\n"
		"User-Agent: Mozilla/5.0\r\n"
		"\r\n",
		m_strUrl.c_str(), m_strHost.c_str());
	return true;
}

bool CSimpleHttpClient::ConnectServer()
{
	m_sockClient = socket(AF_INET, SOCK_STREAM, 0);
	
	SOCKADDR_IN addrSrv;
	DWORD dwNow = GetTickCount();
	if (!GetAddrByName(m_strHost.c_str(), addrSrv.sin_addr))
	{
		m_eError = GetAddrError;
		return false;
	}
	m_dwDomainNameParseTime = GetTickCount() - dwNow;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(m_nPort);

	m_strIP = inet_ntoa(addrSrv.sin_addr);

	dwNow = GetTickCount();
	if (SOCKET_ERROR == connect(m_sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)))
	{
		m_eError = ConnectError;
		closesocket(m_sockClient);
		m_sockClient = INVALID_SOCKET;
		return false;
	}
	m_dwTCPConnectTime = GetTickCount() - dwNow;

	return true;
}

bool CSimpleHttpClient::RecvResponseHeader()
{
	if (SOCKET_ERROR == send(m_sockClient, m_szRequest, m_nRequestSize, 0))
	{
		m_eError = SendError;
		return false;
	}

	while (m_strResponseHeader.size() < kMaxResponseHeadSize)
	{
		char szBuf[kRecvBufSize];
		DWORD dwNow = GetTickCount();
		int nRecv = recv(m_sockClient, szBuf, kRecvBufSize, 0);
		if (0 == m_dwRecvHeaderTime)
		{
			m_dwRecvHeaderTime = GetTickCount() - dwNow;
		}

		if (SOCKET_ERROR == nRecv)
		{
			m_eError = RecvError;
			return false;
		}
		if (0 == nRecv)
		{
			m_eError = BadResponse;
			return false;
		}

		m_strResponseHeader.append(szBuf, nRecv);
		if (ParseResponseHeader())
		{
			break;
		}
		if (NoError != m_eError)
		{
			return false;
		}
	}

	return true;
}

bool CSimpleHttpClient::ParseResponseHeader()
{
	std::string::size_type pos = m_strResponseHeader.find("\r\n\r\n");
	if (std::string::npos == pos)
	{
		return false;
	}

	m_strResponseBody = m_strResponseHeader.substr(pos + 4);
	m_strResponseHeader = m_strResponseHeader.substr(0, pos + 4);

	std::string strTmp = m_strResponseHeader;
	char* p = &strTmp[0];
	char* q = strstr(p, "\r\n");
	*q = '\0';
	if (!ParseStatusLine(p))
	{
		m_eError = BadResponse;
		return false;
	}
	p = q + 2;

	while (1)
	{
		q = strstr(p, "\r\n");
		if (p == q)
		{
			break;
		}
		*q = '\0';
		if (!ParseHeaderLine(p))
		{
			m_eError = BadResponse;
			return false;
		}
		p = q + 2;
	}
	return true;
}

bool CSimpleHttpClient::ParseStatusLine(char* pLine)
{
	char sz[32] = {0};
	sscanf(pLine, "%s %d", sz, &m_dwStatus);
	m_strProtocol = sz;
	return 0 != m_dwStatus;
}

bool CSimpleHttpClient::ParseHeaderLine(char* pLine)
{
	char* p = strchr(pLine, ':');
	if (NULL == p)
	{
		return false;
	}
	*p++ = '\0';

	std::string strKey = pLine;
	std::string strValue = p;
	TrimString(strKey);
	TrimString(strValue);
	m_mapHeader[strKey] = strValue;

	if (0 == _stricmp(strKey.c_str(), "Content-Length"))
	{
		m_nContentLength = atoi(strValue.c_str());
	}
	else if (0 == _stricmp(strKey.c_str(), "Location"))
	{
		m_strLocation = strValue;
	}
	return true;
}

bool CSimpleHttpClient::GetAddrByName(const char* pHost, struct in_addr& stAddr)
{
	struct addrinfo *answer = NULL, hint;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;

	int ret = getaddrinfo(pHost, NULL, &hint, &answer);
	if (ret != 0 || NULL == answer)
	{
		return false;
	}

	stAddr = ((struct sockaddr_in *)answer->ai_addr)->sin_addr;
	return true;
}

#ifndef USE_STREAM
int CSimpleHttpClient::Read(char* pBuf, int nLen)
{
	int nRet = 0;

	if (m_nRead < m_strResponseBody.size())
	{
		nRet = min(m_strResponseBody.size() - m_nRead, nLen);
		memcpy(pBuf, &m_strResponseBody[m_nRead], nRet);
	}
	else
	{
		if (m_nRead >= m_nContentLength)
		{
			return 0;
		}

		nRet = recv(m_sockClient, pBuf, nLen, 0);
		if (nRet <= 0)
		{
			return nRet;
		}
	}

	m_nRead += nRet;
	return nRet;
}
#endif

#ifdef USE_STREAM
std::ostream& operator<<(std::ostream& os,const CSimpleHttpClient& hc)
{
	size_t nRecvTotal = hc.m_strResponseBody.size();

	os << hc.m_strResponseBody;

	while (nRecvTotal < hc.m_nContentLength)
	{
		char szBuf[kRecvBufSize];
		int nRecv = recv(hc.m_sockClient, szBuf, kRecvBufSize, 0);
		if (0 >= nRecv)
		{
			break;
		}

		nRecvTotal += nRecv;
		std::string str(szBuf, nRecv);
		os << str;
	}
	return os;
}
#endif

std::string GetDnsServer() 
{

    FIXED_INFO * FixedInfo;
    ULONG    ulOutBufLen;
    IP_ADDR_STRING * pIPAddr;
    std::string strDns;

    FixedInfo = (FIXED_INFO *) GlobalAlloc( GPTR, sizeof( FIXED_INFO ) );
    ulOutBufLen = sizeof( FIXED_INFO );

    if( ERROR_BUFFER_OVERFLOW == GetNetworkParams( FixedInfo, &ulOutBufLen ) ) {
        GlobalFree( FixedInfo );
        FixedInfo = (FIXED_INFO *) GlobalAlloc( GPTR, ulOutBufLen );
    }

    if ( 0 == GetNetworkParams( FixedInfo, &ulOutBufLen ) ) {
        strDns = FixedInfo -> DnsServerList.IpAddress.String;
    }
    return strDns;
}
