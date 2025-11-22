#include "FeishuNotifier.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// convert UTF-8 to wide string
static std::wstring utf8_to_wstring(const std::string& s) {
    if (s.empty()) return {};
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w;
    w.resize(sz);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], sz);
    return w;
}

// Simple synchronous POST using WinHTTP. Returns true on success.
static bool winhttp_post(const std::string& url, const std::string& payload, const std::string& contentType = "application/json") {
    std::wstring urlw = utf8_to_wstring(url);
    URL_COMPONENTS uc;
    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = (DWORD)-1;
    uc.dwHostNameLength = (DWORD)-1;
    uc.dwUrlPathLength = (DWORD)-1;

    if (!WinHttpCrackUrl(urlw.c_str(), (DWORD)urlw.length(), 0, &uc)) return false;

    std::wstring scheme(uc.lpszScheme, uc.dwSchemeLength);
    std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
    std::wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
    INTERNET_PORT port = uc.nPort;

    HINTERNET hSession = WinHttpOpen(L"bibi/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = 0;
    if (_wcsicmp(scheme.c_str(), L"https") == 0) flags = WINHTTP_FLAG_SECURE;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), nullptr,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::wstring header = L"Content-Type: " + utf8_to_wstring(contentType);
    BOOL ok = WinHttpSendRequest(hRequest, header.c_str(), (DWORD)header.length(),
                                 (LPVOID)payload.c_str(), (DWORD)payload.size(), (DWORD)payload.size(), 0);
    if (!ok) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    ok = WinHttpReceiveResponse(hRequest, nullptr);
    if (!ok) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    // Optionally we could read response body; for now just check status
    DWORD status = 0; DWORD len = sizeof(status);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &len, NULL);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return (status >= 200 && status < 300);
}

bool FeishuNotifier::sendMessage(const std::string& text) {
    std::lock_guard<std::mutex> lk(mu_);
    if (webhook_.empty()) {
        std::cerr << "[ERROR] Feishu webhook not set!" << std::endl;
        return false;
    }

    std::string payload = "{\"msg_type\":\"text\",\"content\":{\"text\":\"" + text + "\"}}";
    try {
        return winhttp_post(webhook_, payload, "application/json");
    } catch (...) {
        return false;
    }
}

#else

#include <curl/curl.h>

bool FeishuNotifier::sendMessage(const std::string& text) {
    std::lock_guard<std::mutex> lk(mu_);
    if (webhook_.empty()) {
        std::cerr << "[ERROR] Feishu webhook not set!" << std::endl;
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string payload = "{\"msg_type\":\"text\",\"content\":{\"text\":\"" + text + "\"}}";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, webhook_.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[ERROR] curl_easy_perform() failed: "
                  << curl_easy_strerror(res) << std::endl;
        return false;
    }
    return true;
}

#endif
