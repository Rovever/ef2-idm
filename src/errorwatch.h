#ifndef ERRORWATCH_H
#define ERRORWATCH_H
#include <Windows.h>

/* 自动关掉 IDM 的「服务器响应显示您没有权限下载此文件 / 403 Forbidden」错误框。
 *
 * 说明：ef2.exe 本身不发起 HTTP 请求，只是通过 COM 把链接交给 IDM，
 * 这类错误框是 IDM 在下载过程中自己弹的，IDM COM API 的 lFlags=1
 * （不显示确认对话框）只作用于「添加链接」阶段，管不到运行时错误框，
 * IDM 自身也没有关闭这类错误框的开关。
 *
 * 因此只在进程退出前短时间轮询窗口，精确匹配后才关：
 *   1) 可见的顶层标准对话框（类名 #32770）；
 *   2) 子控件文本包含 403 / Forbidden / Server responded / 服务器响应 / 没有权限；
 *   3) 命中后先按 IDOK（等同点「确定」），再补一个 WM_CLOSE 兜底。
 * IDM 主窗口虽然也是 #32770，但子控件里没有这些文字，不会被误关；
 * 「下载文件信息」「文件已存在」等确认框同样不含这些文字。
 *
 * 用命名互斥量保证同一时刻只有一个轮询进程，连续点多个 ef2 文件不会堆积进程；
 * 已有的那个轮询进程是全局枚举窗口的，会一并处理后续文件产生的弹窗。
 * 默认只驻留 45 秒后自动退出；ef2 文件里写一行 errorwatch: 0 可完全关闭。
 */
class ErrorWatch
{
    struct Match
    {
        bool hit;
    };
    static BOOL CALLBACK ChildProc(HWND h, LPARAM l)
    {
        Match *m = (Match *)l;
        wchar_t buf[512] = {0};
        GetWindowTextW(h, buf, 512);
        /* 中文用 Unicode 转义，避免源文件编码影响匹配 */
        const wchar_t *keys[] = {
            L"403",
            L"Forbidden",
            L"Server responded",
            L"\u670D\u52A1\u5668\u54CD\u5E94", /* 服务器响应 */
            L"\u6CA1\u6709\u6743\u9650"        /* 没有权限 */
        };
        for (int i = 0; i < 5; ++i)
        {
            if (wcsstr(buf, keys[i]))
            {
                m->hit = true;
                return FALSE;
            }
        }
        return TRUE;
    };
    static BOOL CALLBACK EnumProc(HWND h, LPARAM)
    {
        if (!IsWindowVisible(h))
            return TRUE;
        wchar_t cls[64] = {0};
        GetClassNameW(h, cls, 64);
        if (wcscmp(cls, L"#32770") != 0)
            return TRUE;
        Match m = {false};
        EnumChildWindows(h, ChildProc, (LPARAM)&m);
        if (m.hit)
        {
            HWND ok = GetDlgItem(h, IDOK);
            if (ok)
                PostMessageW(h, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)ok);
            PostMessageW(h, WM_CLOSE, 0, 0); /* 万一 IDOK 没被处理 */
        }
        return TRUE;
    };

public:
    static void Run(int seconds = 45)
    {
        HANDLE mtx = CreateMutexW(NULL, TRUE, L"Local\\ef2_ErrorWatch");
        if (!mtx)
            return;
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(mtx);
            return;
        }
        ULONGLONG end = GetTickCount64() + (ULONGLONG)seconds * 1000;
        while (GetTickCount64() < end)
        {
            EnumWindows(EnumProc, 0);
            Sleep(250);
        }
        ReleaseMutex(mtx);
        CloseHandle(mtx);
    };
};

#endif // ERRORWATCH_H
