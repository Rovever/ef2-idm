/* ef2 逻辑回归验证（不会启动真实 IDM）
 * 用 mock 的 ICIDMLinkTransmitter2 顶替 CoCreateInstance，
 * 完整跑一遍 Code::ef2File，记录每条链接最终送入 IDM 的参数。
 * 编译：见 tests/README 说明或直接用项目根的环境变量配置 cl.exe。
 */
#include <Windows.h>
#include <objbase.h>
#include <atlbase.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

struct CallRecord
{
    std::string url;
    std::string name;
    std::string ua;
    long flags;
};
static std::vector<CallRecord> g_calls;

/* 先让系统头正常声明，再替换调用点，避免污染 windows.h / comip.h */
HRESULT WINAPI MyCoCreateInstance(REFCLSID, LPUNKNOWN, DWORD, REFIID, void **);
#define CoCreateInstance MyCoCreateInstance
#include "../src/code.h"
#undef CoCreateInstance

static std::string BstrToUtf8(BSTR b)
{
    if (!b) return std::string();
    UINT n = SysStringLen(b);
    if (!n) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)b, n, NULL, 0, NULL, NULL);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)b, n, &s[0], len, NULL, NULL);
    return s;
}

class MockIDM : public ICIDMLinkTransmitter2
{
public:
    STDMETHODIMP QueryInterface(REFIID, void **ppv) { *ppv = this; return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 0; }
    STDMETHODIMP SendLinkToIDM(BSTR, BSTR, BSTR, BSTR, BSTR, BSTR, BSTR, BSTR, long) { return S_OK; }
    STDMETHODIMP SendLinkToIDM2(BSTR url, BSTR, BSTR, BSTR, BSTR, BSTR, BSTR path, BSTR name,
                                long flags, VARIANT reserved1, VARIANT)
    {
        CallRecord rec;
        rec.url = BstrToUtf8(url);
        rec.name = BstrToUtf8(name);
        rec.flags = flags;
        if (reserved1.vt == VT_BSTR) rec.ua = BstrToUtf8(reserved1.bstrVal);
        g_calls.push_back(rec);
        return S_OK;
    }
    STDMETHODIMP SendLinksArray(BSTR, VARIANT *) { return S_OK; }
};
static MockIDM g_mock;
HRESULT WINAPI MyCoCreateInstance(REFCLSID, LPUNKNOWN, DWORD, REFIID, void **ppv)
{
    *ppv = static_cast<ICIDMLinkTransmitter2 *>(&g_mock);
    return S_OK;
}

static const char *kSample =
    "<\r\n"
    "https://img.example.com/data/IMG_0001.jpg\r\n"
    "referer: https://img.example.com/data/\r\n"
    "User-Agent: Mozilla/5.0 TestAgent\r\n"
    ">\r\n"
    "<\r\n"
    "https://img.example.com/data/IMG_0001.jpg\r\n"
    ">\r\n"
    "<\r\n"
    "https://other.example.com/data/IMG_0001.jpg\r\n"
    ">\r\n"
    "<\r\n"
    "https://img.example.com/data/IMG_0001.jpg\r\n"
    "filename: \xE8\x87\xAA\xE5\xAE\x9A\xE4\xB9\x89\xE5\x90\x8D\xE5\xAD\x97.mp4\r\n"
    ">\r\n"
    "<\r\n"
    "https://img.example.com/data/IMG_0001.jpg\r\n"
    "filename: \xE8\x87\xAA\xE5\xAE\x9A\xE4\xB9\x89\xE5\x90\x8D\xE5\xAD\x97.mp4\r\n"
    ">\r\n"
    "<\r\n"
    "https://cdn.example.com/v/%E4%B8%AD%E6%96%87%20clip.mp4?token=abc\r\n"
    ">\r\n";

int main()
{
    const char *path = "tests/sample_dup.ef2";
    { std::ofstream f(path, std::ios::binary); f.write(kSample, (std::streamsize)strlen(kSample)); }

    Code::ef2File(path);

    std::cout << "=== [1] params actually passed to IDM COM ===" << std::endl;
    for (size_t i = 0; i < g_calls.size(); ++i)
    {
        std::cout << (i + 1) << ") flags=" << g_calls[i].flags
                  << "  name=[" << g_calls[i].name << "]"
                  << "  ua=[" << g_calls[i].ua << "]"
                  << "  url=" << g_calls[i].url << std::endl;
    }

    std::cout << "\n=== [2] file name inference ===" << std::endl;
    const char *urls[] = {
        "https://x.com/a/b/pic.jpg?token=1#frag",
        "https://x.com/a/b/%E4%B8%AD%E6%96%87%20clip.mp4?token=1",
        "https://x.com/a/b/",
        "https://x.com/a%3Ab%2Ac.dat"};
    for (int i = 0; i < 4; ++i)
        std::cout << urls[i] << "  ->  [" << Code::FileNameFromUrl(urls[i]) << "]" << std::endl;

    std::vector<std::string> lines;
    lines.push_back("-u https://a.com/f.zip ");
    lines.push_back("-o \"D:\\A\" -u https://b.com/f.zip ");
    lines.push_back("-o \"D:\\B\" -u https://c.com/f.zip ");
    lines.push_back("-s same.mp4 -u https://d.com/x ");
    lines.push_back("-s same.mp4 -u https://e.com/y ");
    lines = Code::DeduplicateLines(lines);
    std::cout << "\n=== [3] dedup grouped by directory ===" << std::endl;
    for (size_t i = 0; i < lines.size(); ++i)
        std::cout << (i + 1) << ") " << lines[i] << std::endl;
    return 0;
}
