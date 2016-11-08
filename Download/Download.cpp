//#include "Download.h"
//
//std::vector<unsigned char>* g_pvData = NULL;
//
//std::size_t Callback(void* pbuf, std::size_t size, std::size_t count, void*)
//{
//    unsigned char* p = (unsigned char*)pbuf;
//    g_pvData->insert(g_pvData->end(), p, p + size * count);
//    return count;
//}
//
//unsigned int Download(LPCSTR pszUrl, std::vector<unsigned char>& vData)
//{
//    unsigned int uiErrStep = 0;
//    CURL* pcurl = NULL;
//    vData.clear();
//    
//    do
//    {
//        if (!(pcurl = curl_easy_init()))
//        {
//            uiErrStep = 10;
//            break;
//        }
//        if (curl_easy_setopt(pcurl, CURLOPT_URL, pszUrl) != CURLE_OK)
//        {
//            uiErrStep = 20;
//            break;
//        }
//        g_pvData = &vData;
//        if (curl_easy_setopt(pcurl, CURLOPT_WRITEFUNCTION, &Callback) != CURLE_OK)
//        {
//            uiErrStep = 30;
//            break;
//        }
//        if (curl_easy_setopt(pcurl, CURLOPT_MAXREDIRS, 9) != CURLE_OK)
//        {
//            uiErrStep = 35;
//            break;
//        }
//        curl_easy_setopt(pcurl, CURLOPT_FOLLOWLOCATION, 1);  
//        curl_easy_setopt(pcurl, CURLOPT_NOPROGRESS, 0);  
//        if (curl_easy_perform(pcurl) != CURLE_OK)
//        {
//            vData.clear();
//            uiErrStep = 40;
//            break;
//        }
//    }
//    while (0);
//
//    if (pcurl) curl_easy_cleanup(pcurl);
//    return uiErrStep;
//}
//
//unsigned int Download(LPCSTR pszUrl, LPCWSTR pwszPath)
//{
//    unsigned int uiErrStep = 0;
//    HANDLE hFile = INVALID_HANDLE_VALUE;
//
//    do
//    {
//        std::vector<unsigned char> vData;
//        if ((uiErrStep = Download(pszUrl, vData)) != 0)
//        {
//            break;  // < 50
//        }
//        if ((hFile = CreateFileW(pwszPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE)
//        {
//            uiErrStep = 60;
//            break;
//        }
//        DWORD dwBytesWritten = 0;
//        if (!WriteFile(hFile, &vData[0], (DWORD)vData.size(), &dwBytesWritten, NULL) || dwBytesWritten != vData.size())
//        {
//            uiErrStep = 70;
//            break;
//        }
//    }
//    while (0);
//
//    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
//    return uiErrStep;
//}

#include <fstream>
#include <sstream>
#include <WinSock2.h>
#include "Download.h"

unsigned Download(LPCSTR pszUrl, std::vector<unsigned char>& vData)  // 400
{
    unsigned uiErrStep = 0;
    int err = 0;
    vData.clear();

    do
    {
        WSADATA wsaData = {};
        WORD wVersonRequested = MAKEWORD(2, 2);
        err = WSAStartup(wVersonRequested, &wsaData);
        if(err != 0) 
        {
            uiErrStep = 100;
            break;
        }

        std::string strUrl = pszUrl;
        while (true)
        {
            CSimpleHttpClient client(strUrl);
            if (client.GetError())
            {
                uiErrStep = 200;
                break;
            }
            
            bool done = false;
            switch (client.GetStatus())
            {
            case 200:
                {
                    if (ExtractContents(client, vData) != 0)
                    {
                        uiErrStep = 250;
                    }
                    done = true;
                }
                break;
            case 301:
            case 302:
                {
                    strUrl = client.GetHeader("Location");
                    if (strUrl.empty())
                    {
                        uiErrStep = 300;
                        done = true;
                        break;
                    }
                }
                break;
            default:
                uiErrStep = 400;
                done = true;
            }
            if (done) break;
        }
    }
    while (0);

    if (err == 0) WSACleanup();
    return uiErrStep;
}

//void WideString2MultiByte(const std::wstring& wszStr, std::string& szStr, UINT uCodePage)
//{
//    int nLength = WideCharToMultiByte(uCodePage, 0, wszStr.c_str(),-1,NULL,0,NULL,NULL);
//    szStr.resize(nLength);
//    char * pszDst = new char[nLength];
//    WideCharToMultiByte(uCodePage, 0, wszStr.c_str(), -1, pszDst, nLength, NULL, NULL);
//    pszDst[nLength-1] = '\0';
//    szStr = std::string(pszDst);
//    delete [] pszDst;	
//}

std::pair<unsigned, unsigned> Download(LPCSTR pszUrl, LPCWSTR pwszPath, std::string* pstrIp, unsigned* puLen)
{
    unsigned uiErrStep = 0;
    unsigned uiErrStep2 = 0;
    int err = 0;

    do
    {
        WSADATA wsaData = {};
        WORD wVersonRequested = MAKEWORD(2, 2);
        err = WSAStartup(wVersonRequested, &wsaData);
        if(err != 0) 
        {
            uiErrStep = 100;
            break;
        }

        std::string strUrl = pszUrl;
        while (true)
        {
            CSimpleHttpClient client(strUrl);
            if (pstrIp) *pstrIp = client.GetHostIP();
            if (puLen) *puLen = client.GetContentLength();
            if (uiErrStep2 = client.GetError())
            {
                uiErrStep = 200;
                break;
            }

            bool done = false;
            switch (client.GetStatus())
            {
            case 200:
                {
                    if ((uiErrStep2 = ExtractContents(client, pwszPath)) != 0)
                    {
                        uiErrStep = 250;
                    }
                    done = true;
                }
                break;
            case 301:
            case 302:
                {
                    strUrl = client.GetHeader("Location");
                    if (strUrl.empty())
                    {
                        uiErrStep = 300;
                        done = true;
                        break;
                    }
                }
                break;
            default:
                uiErrStep = 400;
                uiErrStep2 = client.GetStatus();
                done = true;
            }
            if (done) break;
        }
    }
    while (0);

    if (err == 0) WSACleanup();
    return std::make_pair(uiErrStep, uiErrStep2);
}

unsigned ExtractContents(CSimpleHttpClient& client, std::vector<unsigned char>& vData)
{
    unsigned uiErrStep = 0;
    vData.clear();

    const unsigned uiChunkSize = 4 * 1024;
    unsigned char buf[uiChunkSize] = {};
    int nRet = uiChunkSize;
    while (nRet)
    {
        nRet = client.Read((char*)buf, uiChunkSize);
        if (nRet < 0)
        {
            uiErrStep = 100;
            break;
        }
        vData.insert(vData.end(), buf, buf + nRet);
    }

    if (uiErrStep != 0) vData.clear();
    return uiErrStep;
}

unsigned ExtractContents(CSimpleHttpClient& client, LPCWSTR pwszPath)
{
    unsigned uiErrStep = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    do
    {
        hFile = CreateFileW(pwszPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            uiErrStep = 100;
            break;
        }

        const unsigned uiChunkSize = 4 * 1024;
        unsigned char buf[uiChunkSize] = {};
        int nRet = uiChunkSize;
        while (true)
        {
            nRet = client.Read((char*)buf, uiChunkSize);
            if (nRet < 0)
            {
                uiErrStep = 200;
                break;
            }
            if (nRet == 0) break;
            DWORD dwBytesWritten = 0;
            if (!WriteFile(hFile, buf, nRet, &dwBytesWritten, NULL) || dwBytesWritten != nRet)
            {
                uiErrStep = 300;
                break;
            }
        }
    }
    while (0);

    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    if (uiErrStep != 0) DeleteFileW(pwszPath);
    return uiErrStep;
}
