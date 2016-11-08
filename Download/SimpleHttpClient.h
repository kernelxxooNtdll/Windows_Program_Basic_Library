#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <string>
#include <vector>
#include <map>
#include <WinSock2.h>

const int kMaxRequestSize = 4 * 1024;
const int kRecvBufSize = 4 * 1024;
const int kMaxResponseHeadSize = 40 * 1024;

std::string GetDnsServer();

class CSimpleHttpClient
{
#ifdef USE_STREAM
	friend std::ostream& operator<<(std::ostream&, const CSimpleHttpClient&);
#endif

public:
	CSimpleHttpClient(const std::string& strUrl);
	~CSimpleHttpClient();

	enum ErrorCode
	{
		NoError = 0,
		NotHttpProtocol,
		GetAddrError,
		ConnectError,
		SendError,
		RecvError,
		BadResponse,
	};

	ErrorCode GetError() const { return m_eError; }
	std::string GetHost() const { return m_strHost; }
	std::string GetHostIP() const { return m_strIP; }
	USHORT GetPort() const { return m_nPort; }

	DWORD GetDomainNameParseTime() const { return m_dwDomainNameParseTime; }
	DWORD GetTCPConnectTime() const { return m_dwTCPConnectTime; }
	DWORD GetRecvHeaderTime() const { return m_dwRecvHeaderTime; }

	std::string GetResponseHeader() const { return m_strResponseHeader; }
	std::string GetProtocol() const { return m_strProtocol; }
	DWORD GetStatus() const { return m_dwStatus; }
	std::string GetHeader(const std::string& strName) const;
	UINT GetContentLength() const { return m_nContentLength; }
	std::string GetLocation() const { return m_strLocation; }

#ifndef USE_STREAM
	int Read(char* pBuf, int nLen);
#endif

private:
	bool ParseUrl(const std::string& strUrl);
	bool PackRequeset();
	bool ConnectServer();
	bool RecvResponseHeader();
	bool ParseResponseHeader();

	bool ParseStatusLine(char* pLine);
	bool ParseHeaderLine(char* pLine);

	bool GetAddrByName(const char* pHost, struct in_addr& stAddr);

private:
	USHORT m_nPort;
	std::string m_strUrl;

	std::string m_strHost;
	std::string m_strIP;
	DWORD m_dwDomainNameParseTime;
	DWORD m_dwTCPConnectTime;
	DWORD m_dwRecvHeaderTime;

	char m_szRequest[kMaxRequestSize];
	int m_nRequestSize;
	std::string m_strResponseHeader;
	std::string m_strResponseBody;

	std::string m_strProtocol;
	DWORD m_dwStatus;
	std::map<std::string, std::string> m_mapHeader;
	UINT m_nContentLength;
	std::string m_strLocation;

	SOCKET m_sockClient;

	ErrorCode m_eError;
	int m_nRead;
};
