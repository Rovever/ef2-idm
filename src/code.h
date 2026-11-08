#ifndef CODE_H
#define CODE_H
#include "stdfx.h"
#include "base64.h"
#include <map>
#include <set>
#include <sstream>
#include <cctype>
#include <cstring>

#import "IDM/IDManTypeInfo.tlb"
#include "IDM/IDManTypeInfo.h"
#include "IDM/IDManTypeInfo_i.c"
class Code
{
public:
    /* 是否启用「自动关闭 IDM 403 错误框」看门狗（ef2 文件里可写 errorwatch: 0 关闭） */
    static bool &WatchEnabled()
    {
        static bool enabled = true;
        return enabled;
    };
    static std::string Utf8ToGbk(const char *src_str)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
        wchar_t *wszGBK = new wchar_t[len + 1];
        memset(wszGBK, 0, len * 2 + 2);
        MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
        len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
        char *szGBK = new char[len + 1];
        memset(szGBK, 0, len + 1);
        WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
        std::string strTemp(szGBK);
        if (wszGBK)
            delete[] wszGBK;
        if (szGBK)
            delete[] szGBK;
        return strTemp;
    };
    static std::string GbkToUtf8(const char *src_str)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
        wchar_t *wstr = new wchar_t[len + 1];
        memset(wstr, 0, len + 1);
        MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
        len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
        char *str = new char[len + 1];
        memset(str, 0, len + 1);
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
        std::string strTemp = str;
        if (wstr)
            delete[] wstr;
        if (str)
            delete[] str;
        return strTemp;
    };
    static std::vector<std::string> StringToArray(std::string str)
    {
        std::string::size_type pos;
        std::vector<std::string> result;
        std::string pattern = " ";
        std::string check = "\"";
        str += pattern;
        int size = str.size();
        for (int i = 0; i < size; i++)
        {
            if (str.substr(i, 1) == check && i + 1 < size)
            {
                i++;
                pos = str.find(check, i + 1);
            }
            else
            {
                pos = str.find(pattern, i);
            }
            if (pos < size)
            {
                std::string s = str.substr(i, pos - i);
                result.push_back(s);
                i = pos + pattern.size() - 1;
            }
        }
        return result;
    };

    /* ------------------------------------------------------------------
     * 重复链接 / 自动重命名
     * IDM 在 lFlags=0 时会为每条链接弹出「开始下载」对话框，遇到重名或
     * 已存在的链接还会再弹「覆盖/重命名」对话框。ef2.exe 又是同步连续
     * 调用 COM，模态框叠加后 IDM 与 ef2.exe 互相等待，形成死锁：
     * 第一个框能操作，后续框点任何按钮都无响应，只能结束 IDM 进程。
     * 因此下面做两件事：
     *   1) 默认 lFlags |= 1，彻底不弹任何确认框，直接开始下载；
     *   2) 同一个 ef2 文件里重复的链接 / 重名文件自动改名 (2)、(3)…
     * ------------------------------------------------------------------ */
private:
    static std::string LowerCase(std::string s)
    {
        for (size_t i = 0; i < s.size(); ++i)
            s[i] = (char)tolower((unsigned char)s[i]);
        return s;
    };
    static int HexValue(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    static std::string UrlDecode(const std::string &src)
    {
        std::string out;
        out.reserve(src.size());
        for (size_t i = 0; i < src.size(); ++i)
        {
            if (src[i] == '%' && i + 2 < src.size())
            {
                int h = HexValue(src[i + 1]), l = HexValue(src[i + 2]);
                if (h >= 0 && l >= 0)
                {
                    out.push_back((char)((h << 4) | l));
                    i += 2;
                    continue;
                }
            }
            out.push_back(src[i]);
        }
        return out;
    };
    static void TrimCR(std::string &s)
    {
        /* ef2 文件可能是 CRLF 行尾，不去掉的话 "<" / ">" 永远匹配不上 */
        while (!s.empty() && (s[s.size() - 1] == '\r' || s[s.size() - 1] == '\n'))
            s.erase(s.size() - 1);
    };
public:
    static std::string FileNameFromUrl(const std::string &url)
    {
        std::string s = url;
        size_t p = s.find('#');
        if (p != std::string::npos) s.erase(p);
        p = s.find('?');
        if (p != std::string::npos) s.erase(p);
        p = s.find_last_of('/');
        if (p != std::string::npos) s.erase(0, p + 1);
        s = UrlDecode(s);
        const std::string bad = "\\/:*?\"<>|";
        for (size_t i = 0; i < s.size(); ++i)
            if (bad.find(s[i]) != std::string::npos) s[i] = '_';
        while (!s.empty() && (s[0] == ' ' || s[0] == '.')) s.erase(0, 1);
        while (!s.empty() && (s[s.size() - 1] == ' ' || s[s.size() - 1] == '.')) s.erase(s.size() - 1);
        if (s.empty()) s = "download";
        if (s.size() > 180) s = s.substr(s.size() - 180);
        return s;
    };
    static std::string UniqueFileName(const std::string &base, int n)
    {
        std::string stem = base, ext;
        size_t dot = base.find_last_of('.');
        if (dot != std::string::npos && dot > 0 && base.size() - dot <= 12)
        {
            stem = base.substr(0, dot);
            ext = base.substr(dot);
        }
        std::ostringstream os;
        os << stem << " (" << n << ")" << ext;
        return os.str();
    };
    static void InspectArgs(const std::string &line, std::string &url, std::string &dir, std::string &name)
    {
        std::vector<std::string> argv = StringToArray(line);
        for (size_t i = 0; i < argv.size(); ++i)
        {
            /* 后出现的同名参数以最后一次为准，和 SendLinkToIDM 保持一致 */
            if (argv[i] == "-u" && i + 1 < argv.size()) url = argv[++i];
            else if (argv[i] == "-o" && i + 1 < argv.size()) dir = argv[++i];
            else if (argv[i] == "-s" && i + 1 < argv.size()) name = argv[++i];
        }
    };
    /* 对目标「保存目录 + 文件名」冲突的链接追加 -s "xxx (2).ext" */
    static std::vector<std::string> DeduplicateLines(const std::vector<std::string> &lines)
    {
        std::vector<std::string> out = lines;
        std::set<std::string> used;
        for (size_t i = 0; i < out.size(); ++i)
        {
            std::string url, dir, name;
            InspectArgs(out[i], url, dir, name);
            if (url.empty()) continue;
            std::string base = name.empty() ? FileNameFromUrl(url) : name;
            std::string prefix = LowerCase(dir) + "\\";
            std::string finalName = base;
            if (used.count(prefix + LowerCase(base)) > 0)
            {
                int seq = 2;
                do
                {
                    finalName = UniqueFileName(base, seq++);
                } while (seq < 10000 && used.count(prefix + LowerCase(finalName)) > 0);
                if (!out[i].empty() && out[i][out[i].size() - 1] != ' ')
                    out[i] += " ";
                out[i] += "-s \"" + finalName + "\" ";
            }
            used.insert(prefix + LowerCase(finalName));
        }
        return out;
    };

    static void ef2Protocol(std::string lpLink)
    {
        std::string temp, result;
        lpLink.erase(0, 6);
        Base64::Decode(lpLink, &temp);
        result = Utf8ToGbk(temp.c_str());
        if (result.find("-u ") != std::string::npos)
        {
            /* 默认静默直接下载；-q 仍可显式排队，但不再弹「请手动开始队列」提示 */
            SendLinkToIDM(result);
        }
        else
        {
            MessageBox(NULL, TEXT("非法ef2链接！请查阅相关文档：https://github.com/MotooriKashin/ef2"), NULL, MB_ICONERROR);
        }
    };
    static void ef2File(std::string path)
    {
        std::ifstream inFile;
        inFile.open(path.c_str());
        if (!inFile)
        {
            MessageBoxW(NULL, L"无法打开该ef2文件！", L"ef2解析", MB_ICONERROR);
            return;
        }
        std::vector<std::string> lines; /* 先全部收集，统一去重后再发送 */
        std::string temp;
        std::string result = "";
        while (!inFile.eof())
        {
            getline(inFile, temp);
            TrimCR(temp);
            if (temp == "<")
            {
                if (result != "")
                    lines.push_back(result);
                result = "";
                getline(inFile, temp);
                TrimCR(temp);
                while (temp != ">" && !inFile.eof())
                {
                    if (temp.find("referer") != std::string::npos)
                    {
                        result.append("-r ");
                        result.append(temp.substr(9));
                        result.append(" ");
                    }
                    else if (temp.find("User-Agent") != std::string::npos)
                    {
                        result.append("-a \"");
                        result.append(temp.substr(12));
                        result.append("\" ");
                    }
                    else if (temp.find("cookies") != std::string::npos)
                    {
                        result.append("-c \"");
                        result.append(temp.substr(9));
                        result.append("\" ");
                    }
                    else if (temp.find("postdata") != std::string::npos)
                    {
                        result.append("-d \"");
                        result.append(temp.substr(10));
                        result.append("\" ");
                    }
                    else if (temp.find("username") != std::string::npos)
                    {
                        result.append("-U \"");
                        result.append(temp.substr(10));
                        result.append("\" ");
                    }
                    else if (temp.find("password") != std::string::npos)
                    {
                        result.append("-P \"");
                        result.append(temp.substr(10));
                        result.append("\" ");
                    }
                    else if (temp.find("filepath") != std::string::npos)
                    {
                        result.append("-o \"");
                        result.append(temp.substr(10));
                        result.append("\" ");
                    }
                    else if (temp.find("filename") != std::string::npos)
                    {
                        result.append("-s \"");
                        result.append(temp.substr(10));
                        result.append("\" ");
                    }
                    else if (temp.find("://") != std::string::npos)
                    {
                        result.append("-u ");
                        result.append(temp);
                        result.append(" ");
                    }
                    else if (temp.find("interactive:") != std::string::npos)
                    {
                        /* 少数场景仍想手动改文件名/保存目录时，用这一行恢复IDM对话框 */
                        result.append("-i ");
                    }
                    else if (temp.find("errorwatch: 0") != std::string::npos)
                    {
                        /* 不想自动关闭IDM的403错误框时，用这一行关掉看门狗 */
                        WatchEnabled() = false;
                    }
                    getline(inFile, temp);
                    TrimCR(temp);
                }
            }
        }
        if (result != "")
            lines.push_back(result);
        inFile.close();

        if (lines.empty())
        {
            MessageBox(NULL, TEXT("该ef2文件似乎并不包含有效下载链接！？\r\n请检查文件内容或者反馈到：https://github.com/MotooriKashin/ef2/issues"), TEXT("ef2解析"), MB_ICONERROR);
            return;
        }

        lines = DeduplicateLines(lines);

        int failed = 0;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string fin = Utf8ToGbk(lines[i].c_str());
            /* 不再追加 -q：全部直接开始下载，无需到 IDM 手动启动队列 */
            if (SendLinkToIDM(fin, true) != 0)
                ++failed;
            else if (i + 1 < lines.size())
                Sleep(30); /* 给 IDM 留出消化上一条链接的时间，避免连续 RPC 堆积 */
        }
        if (failed > 0)
        {
            std::wstring msg = L"共 ";
            msg += std::to_wstring(lines.size());
            msg += L" 条链接，其中 ";
            msg += std::to_wstring(failed);
            msg += L" 条拉起 IDM 失败。\r\n请检查 IDM 是否正常运行、COM 组件是否已注册。";
            MessageBoxW(NULL, msg.c_str(), L"ef2解析", MB_ICONWARNING);
        }
    };
    /* quiet=true 时内部不弹任何 MessageBox，批量调用结束后由调用方汇总提示 */
    static int CALLBACK SendLinkToIDM(std::string str, bool quiet = false)
    {
        std::string su, sa, sc, sd, sr, sU, sP, so, ss;
        /* 1 = do not show any confirmations dialogs
         * 默认静默：不弹「开始下载」、也不弹「文件已存在/重命名/覆盖」，
         * 从根子上避免重复链接时 IDM 模态框叠加导致的死锁。
         * 2 = add to queue only, do not start downloading（仅显式 -q 时启用） */
        long f = 1;

        std::vector<std::string> argv = StringToArray(str);
        int argc = (int)argv.size();
        for (int i = 0; i < argc; i++)
        {
            if (!strcmp(argv[i].c_str(), "-u") && (i + 1 != argc)) su = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-a") && (i + 1 != argc)) sa = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-c") && (i + 1 != argc)) sc = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-d") && (i + 1 != argc)) sd = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-r") && (i + 1 != argc)) sr = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-U") && (i + 1 != argc)) sU = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-P") && (i + 1 != argc)) sP = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-o") && (i + 1 != argc)) so = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-s") && (i + 1 != argc)) ss = argv[++i];
            else if (!strcmp(argv[i].c_str(), "-f")) f |= 1;
            else if (!strcmp(argv[i].c_str(), "-i")) f &= ~1; /* 恢复IDM对话框 */
            else if (!strcmp(argv[i].c_str(), "-q")) f |= 2;
        }
        if (su.empty())
        {
            if (!quiet)
                MessageBox(NULL, TEXT("未发现有效下载链接！"), TEXT("SendLinkToIDM"), MB_ICONWARNING);
            return -1;
        }

        /* 先解析成 std::string 再构造 BSTR：同名参数以最后一个为准，
         * 自动重命名追加的 -s 才能覆盖 ef2 文件里原有的 filename */
        CComBSTR u, a, c, d, r, U, P, o, s;
        if (!su.empty()) u.Append(su.c_str());
        if (!sa.empty()) a.Append(sa.c_str());
        if (!sc.empty()) c.Append(sc.c_str());
        if (!sd.empty()) d.Append(sd.c_str());
        if (!sr.empty()) r.Append(sr.c_str());
        if (!sU.empty()) U.Append(sU.c_str());
        if (!sP.empty()) P.Append(sP.c_str());
        if (!so.empty()) o.Append(so.c_str());
        if (!ss.empty()) s.Append(ss.c_str());

        int ret = -1;
        ICIDMLinkTransmitter2 *pIDM = NULL;
        HRESULT cohr = CoInitialize(NULL);
        if (FAILED(cohr))
        {
            if (!quiet)
                MessageBox(NULL, TEXT("拉起IDM失败！COM初始化异常！"), TEXT("SendLinkToIDM"), MB_ICONWARNING);
            return -1;
        }
        HRESULT hr = CoCreateInstance(CLSID_CIDMLinkTransmitter, NULL, CLSCTX_LOCAL_SERVER, IID_ICIDMLinkTransmitter2, (void **)&pIDM);
        if (S_OK == hr && pIDM)
        {
            VARIANT var1, var2;
            VariantInit(&var1);
            VariantInit(&var2);
            /* 官方文档：reserved1.vt=VT_BSTR 时用于指定 User-Agent。
             * 原实现误把 URL 塞进了 reserved1，-a 参数其实从未生效。 */
            if (a.Length() > 0)
            {
                var1.vt = VT_BSTR;
                var1.bstrVal = a;
            }
            var2.vt = VT_EMPTY;

            hr = pIDM->SendLinkToIDM2(u, r, c, d, U, P, o, s, f, var1, var2);
            if (S_OK == hr)
                ret = 0;
            else if (!quiet)
                MessageBox(NULL, TEXT("拉起IDM失败！IDM返回信息异常！"), TEXT("SendLinkToIDM"), MB_ICONWARNING);
            pIDM->Release();
        }
        else if (!quiet)
        {
            MessageBox(NULL, TEXT("拉起IDM失败！未找到IDM的COM组件！"), TEXT("SendLinkToIDM"), MB_ICONWARNING);
        }
        CoUninitialize();
        return ret;
    };
};

#endif // CODE_H
