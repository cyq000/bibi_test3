#include "BinanceAPI.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <sstream>

using json = nlohmann::json;

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// Helper: convert UTF-8 std::string to wide string
static std::wstring utf8_to_wstring(const std::string& s) {
    if (s.empty()) return {};
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w;
    w.resize(sz);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], sz);
    return w;
}

// Simple WinHTTP GET implementation (synchronous)
static std::string winhttp_get(const std::string& url) {
    std::wstring urlw = utf8_to_wstring(url);
    URL_COMPONENTS uc;
    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = (DWORD)-1;
    uc.dwHostNameLength = (DWORD)-1;
    uc.dwUrlPathLength = (DWORD)-1;

    if (!WinHttpCrackUrl(urlw.c_str(), (DWORD)urlw.length(), 0, &uc)) {
        throw std::runtime_error("WinHttpCrackUrl failed");
    }

    std::wstring scheme(uc.lpszScheme, uc.dwSchemeLength);
    std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
    std::wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
    INTERNET_PORT port = uc.nPort;

    HINTERNET hSession = WinHttpOpen(L"bibi-agent/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) throw std::runtime_error("WinHttpOpen failed");

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); throw std::runtime_error("WinHttpConnect failed"); }

    DWORD flags = 0;
    if (_wcsicmp(scheme.c_str(), L"https") == 0) flags = WINHTTP_FLAG_SECURE;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), nullptr,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); throw std::runtime_error("WinHttpOpenRequest failed"); }

    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!ok) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); throw std::runtime_error("WinHttpSendRequest failed"); }

    ok = WinHttpReceiveResponse(hRequest, nullptr);
    if (!ok) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); throw std::runtime_error("WinHttpReceiveResponse failed"); }

    std::string response;
    DWORD dwSize = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::vector<char> buffer(dwSize);
        DWORD downloaded = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &downloaded)) break;
        response.append(buffer.data(), downloaded);
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

BinanceAPI::BinanceAPI() {}

std::string BinanceAPI::httpGet(const std::string& url) {
    return winhttp_get(url);
}

#else // POSIX / non-Windows: keep libcurl

#include <curl/curl.h>

// libcurl callback: write to memory
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

BinanceAPI::BinanceAPI() {
    curl_global_init(CURL_GLOBAL_ALL);
}

std::string BinanceAPI::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("CURL init failed");

    std::string readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // ignore certs
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return readBuffer;
}

#endif


//std::vector<BianOIData> parseJson(const std::string& raw) {
//    std::vector<BianOIData> out;
//
//    json j = json::parse(raw);
//
//    for (auto& item : j) {
//        BianOIData d;
//        d.symbol = item["symbol"];
//        d.OI = stod(item["sumOpenInterest"].get<std::string>());
//        d.OIV = stod(item["sumOpenInterestValue"].get<std::string>());
//        d.ts = item["timestamp"];
//
//        // 推算价格
//        d.price = (d.OIV / d.OI);
//
//        out.push_back(d);
//    }
//
//    return out;
//}

// 获取符合条件的合约交易对
std::vector<std::string> BinanceAPI::getSymbolsWithVolume(long minVol, long maxVol) {
    std::vector<std::string> symbols;
    std::string url = "https://fapi.binance.com/fapi/v1/ticker/24hr";
    std::string resp = httpGet(url);

    auto data = json::parse(resp);
    for (auto& item : data) {
        try {
            std::string symbol = item["symbol"];
            double volume = std::stod(item["quoteVolume"].get<std::string>());

            // 只监控 USDT 结算的交易对
            if (symbol.size() > 4 && symbol.substr(symbol.size() - 4) == "USDT") {
                if (volume >= minVol && volume <= maxVol) {
                    symbols.push_back(symbol);
                }
            }
        } catch (...) {
            continue;
        }
    }

    //LOG_INFO("筛选出 " + std::to_string(symbols.size()) + " 个合约交易对");
    return symbols;
}

// 拉取 open interest 数据
std::string BinanceAPI::fetchOpenInterestData(const std::string &symbol, const std::string &period, int limit)
{
    // 例https://fapi.binance.com/futures/data/openInterestHist?symbol=BTCUSDT&period=4h&limit=30

    std::ostringstream url;
    url << "https://fapi.binance.com/futures/data/openInterestHist?symbol="
        << symbol << "&period=" << period << "&limit=" << limit;

    std::string resp = httpGet(url.str());

    if (resp.empty())
    {
        LOG_WARN("fetchOpenInterestData empty response for " + symbol + " " + period);
        std::cerr << "Error: empty response for " << symbol << " " << period << std::endl;
        return {}; // 返回空结果，不解析
    }

    try
    {
        // // 解析 JSON → 数据结构
        // std::vector<BianOIData> data = parseJson(resp);
        // return data;

        return resp;
    }
    catch (const json::parse_error &e)
    {
        LOG_WARN("fetchOpenInterestData JSON parse error for " + symbol + " " + period + ": " + e.what());
        std::cerr << "JSON parse error: " << e.what()
                  << " | Response: " << resp.substr(0, 200) << std::endl;
        return {};
    }
}



