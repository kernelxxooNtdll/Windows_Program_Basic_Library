#pragma once

//#include <vector>
//#include <boost/bind.hpp>
//#include <Windows.h>
//#include <curl/curl.h>
//#pragma comment(lib, "libcurl.lib")
//#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "wldap32.lib")
//
//unsigned int Download(LPCSTR pszUrl, std::vector<unsigned char>& vData);
//unsigned int Download(LPCSTR pszUrl, LPCWSTR pwszPath);

#include <utility>
#include <vector>
#include "SimpleHttpClient.h"

unsigned int Download(LPCSTR pszUrl, std::vector<unsigned char>& vData);
std::pair<unsigned, unsigned> Download(LPCSTR pszUrl, LPCWSTR pwszPath, std::string* pstrIp = NULL, unsigned* puLen = NULL);
unsigned int ExtractContents(CSimpleHttpClient& client, std::vector<unsigned char>& vData);
unsigned int ExtractContents(CSimpleHttpClient& client, LPCWSTR pwszPath);
