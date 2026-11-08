[Internet Download Manager (IDM)](http://www.internetdownloadmanager.com/)下载辅助工具，允许主动拉起IDM进行下载，并配置referer、User-Agent等信息。

> **本仓库是 [MotooriKashin/ef2](https://github.com/MotooriKashin/ef2) 的修改版**
> - 原项目地址：<https://github.com/MotooriKashin/ef2>　|　原作者：**MotooriKashin**
> - 原项目采用 [MIT License](LICENSE) 开源，`Copyright (c) 2021 MotooriKashin` 与许可全文完整保留；本仓库新增及修改的代码同样以 MIT 许可提供。
> - 相对原版的主要改动：修复重复链接导致 IDM 卡死、批量下载不再需要手动开始队列、重复文件名自动重命名、User-Agent / CRLF / COM 资源释放修正，以及可选的 IDM 403 错误框自动关闭；详见文末两节。
> - 原项目最后更新于 2022-07。本修改版与原作者无隶属关系，原版自身的问题请反馈到原仓库。

---
### 安装/卸载
绿色命令行程序，只需要写入注册表关联：  
1. 从本仓库 [Releases](../../releases/latest) 下载 `ef2.exe`，或按文末「编译相关」自行编译，放到任意目录。仓库内不附带预编译二进制。
2. 双击打开ef2.exe，点击安装按钮即可。  

如果觉得不好用，需要卸载：  
1. 双击打开ef2.exe，点击卸载按钮。
2. 删除ef2.exe即可。

*ef2.exe最好放到专门的目录，以免移动或丢失，移动后须重新注册。*

---
### 使用方式
#### ※ IDM导出文件
IDM官方定义了专门的文件格式，拓展名.ef2，用于导入导出下载列表，不过用起来步骤较多且不支持双击打开。本工具不仅支持双击打开ef2文件开始下载并拓展了ef2文件的配置项支持。  
ef2文件本质是一个标准的纯文本文件（**utf-8编码**），展开示例如下：

<details>
  <summary>查看ef2文件示例</summary>
  <pre>
<
https://img2.example.com/data/1102/94/IMG_0001.jpg
referer: https://img2.example.com/data/1102/94/
User-Agent: Mozilla/5.0 (Windows NT 6.1; Trident/7.0; rv:11.0) like Gecko
>
<
https://img2.example.com/data/1102/94/IMG_0002.jpg
referer: https://img2.example.com/data/1102/94/
User-Agent: Mozilla/5.0 (Windows NT 6.1; Trident/7.0; rv:11.0) like Gecko
>
<
https://img2.example.com/data/1102/94/IMG_0003.jpg
referer: https://img2.example.com/data/1102/94/
User-Agent: Mozilla/5.0 (Windows NT 6.1; Trident/7.0; rv:11.0) like Gecko
>
  </pre>
</details>

除了IDM官方提供referer、User-Agent关键词，本工具还拓展了一些关键词：

<details>
  <summary>查看拓展的关键词</summary>
  <table border="1">
    <tr>
        <td>cookies</td>
        <td>cookies 字符串</td>
    </tr>
    <tr>
        <td>postdata</td>
        <td>改用POST方法send的字符串</td>
    </tr>
    <tr>
        <td>username</td>
        <td>HTTP认证用户名</td>
    </tr>
    <tr>
        <td>password</td>
        <td>HTTP认证密钥</td>
    </tr>
    <tr>
        <td>filepath</td>
        <td>保存路径（反斜杠形式）</td>
    </tr>
    <tr>
        <td>filename</td>
        <td>文件名（包括拓展名）</td>
    </tr>
    <tr>
        <td>interactive</td>
        <td>该条目恢复显示IDM下载对话框（默认静默，一般不建议使用）</td>
    </tr>
    <tr>
        <td>errorwatch</td>
        <td>写 <code>errorwatch: 0</code> 可关闭「自动点掉IDM 403错误框」功能</td>
    </tr>
  </table>
</details>
值得注意的是ef2文件中不论单条还是多条下载数据，双击打开都会**静默直接开始下载**：不再弹出IDM的下载确认框，也不需要到IDM里手动开始队列。

同一个ef2文件里出现重复链接或重名文件时会自动重命名（如 `IMG_0001 (2).jpg`、`IMG_0001 (3).jpg`），因此不会弹出「文件已存在／是否重命名、覆盖」之类的询问框；重复点击同一个ef2文件也因为默认静默而不会把IDM卡死。重名判断会区分保存目录：文件名相同但 `filepath` 不同的条目不会被误改名。

如果确实需要IDM弹出下载对话框手动改文件名或保存位置，可在该条目中加一行 `interactive: 1` 恢复弹窗（一般不建议使用，重复链接时可能再次触发卡死）。

---
#### ※ ef2协议
为了方便从浏览器中拉起IDM，本工具自定义了一种名为ef2的协议，能像使用http协议一样使用，格式如下：  
| 协议头 | 内容 | 完整形式 |
| ----- | ----- | ----- |
| ef2:// | d3d3LmV4YW1wbGUuY29t | ef2://d3d3LmV4YW1wbGUuY29t |
| http:// | www.example.com | http://www.example.com |

**不难发现所谓的ef2就是将http协议内容部分用Base64编码一下然后将http协议头改为ef2。**  
如此一来在浏览器中点击该链接便可以拉起IDM。但我们还需拓展一下，以让这种协议不仅能传递下载链接，还要能传递referer、User-Agent等关键词以支持ef2文件类似的功能。  
于是我们将协议内容部分由单纯的网址转变为**命令行参数**形式（为方便演示暂时未使用Base64编码）
>
> `www.example.com ----参数化----> -u www.example.com`
>

<details>
  <summary>查看完整参数说明(*含空格等非法字符时请用双引号包裹*)</summary>
<table border="1">
    <tr>
        <th>参数标记</th>
        <th>参数内容</th>
        <th>说明示例</th>
    </tr>
    <tr>
      <td>u</td>
      <td>URL</td>
      <td><pre>-u https://img2.example.com/data/1102/94/IMG_0001.jpg</pre></td>
    </tr>
    <tr>
      <td>r</td>
      <td>referer</td>
      <td><pre>-r https://img2.example.com/</pre></td>
    </tr>
    <tr>
      <td>a</td>
      <td>User-Agent</td>
      <td><pre>-a "Bilibili Freedoooooom/MarkII"</pre></td>
    </tr>
    <tr>
      <td>c</td>
      <td>cookies</td>
      <td><pre>-c cookies字符串</pre></td>
    </tr>
    <tr>
      <td>d</td>
      <td>改用POST请求send的数据</td>
      <td><pre>-d send字符串</pre></td>
    </tr>
    <tr>
      <td>U</td>
      <td>HTTP认证用户名</td>
      <td><pre>-U username字符串</pre></td>
    </tr>
    <tr>
      <td>P</td>
      <td>HTTP认证密钥</td>
      <td><pre>-P password字符串</pre></td>
    </tr>
    <tr>
      <td>o</td>
      <td>保存路径（反斜杠）</td>
      <td><pre>-o F:\下载</pre></td>
    </tr>
    <tr>
        <td>s</td>
        <td>文件名（含拓展名）</td>
        <td><pre>-s 视频.mp4</pre></td>
    </tr>
    <tr>
        <td>f</td>
        <td>禁用下载对话框（**现已是默认行为**，保留仅为兼容旧用法）</td>
        <td><pre>-f</pre></td>
    </tr>
    <tr>
        <td>i</td>
        <td>恢复显示IDM下载对话框，可手动改文件名/保存位置（与默认静默相反）</td>
        <td><pre>-i</pre></td>
    </tr>
    <tr>
        <td>q</td>
        <td>稍后下载（加入默认队列，不再弹出提示框）</td>
        <td><pre>-q</pre></td>
    </tr>
</table>
</details>

注意别忘了用Base64编码一下然后添加上ef2协议头。  
这种ef2协议使用方式基本类似http、ftp等协议，可以直接在浏览器地址栏中输入、添加a标签等href属性并点击、`window.open()`等方法打开，当然也可以直接作为命令行参数传递给ef2.exe（Base64编码千万别忘记）。  
**第一次使用本方法浏览器会弹出打开外部程序请求弹窗，需要点击允许，建议勾选不再提示。**

#### ※ webSocket
本工具还支持在后台运行，通过websocket协议接受下载数据并传递给IDM。  
1. 命令行启动ef2.exe同时传递参数`ef2://websocket:${端口号}`，如`ef2://websocket:12345`。（**不需要Base64加密！**）
2. 使用websocket客户端连接`ws://localhost:${端口号}`，如`ws://localhost:12345`。
3. 使用websocket.send方法发起下载数据，格式可以是ef2协议（需要Base64编码）字符串。
4. 使用websocket.close关闭websocket连接。
5. 如果要退出后台运行，请在websocket.close前使用websocket.send方法发送字符串`fin`。

注意也可以用使用ef2协议的方式使用`ef2://websocket:${端口号}(不需要Base64加密)`后台运行。

#### ※ ef2.js
本项目自带了一个标准的es6模块ts/js文件，以`default`方式导出了一个对象，支持生成ef2协议字符串。
1. 以标准es6模块加载方式加载该模块，如`const ef2 = (await import("ef2.js")).default`。
2. 使用`encode()`方式返回编码生成ef2协议字符串，`const result = ef2.encode({url: "http://www.example.com/"})`。
3. 使用生成的ef2协议拉起IDM进行下载，如`window.open(result)`。

1.0.2版本，新添了一个异步方法`nativeMessaging`，传递端口号可以启动webSocket模式。  

具体模块定义在ts文件中有详细说明文档，在VSCode中可以获取详细提示。  
注：`toastDisabled` 现在已是默认行为；`sendToList` 仍可显式把链接放进队列稍后下载。

#### ※ 重复链接卡死问题的修复
IDM COM API 的 `SendLinkToIDM2` 在 `lFlags=0` 时会为每条链接弹出「开始下载」对话框，遇到重名或已存在的链接还会再弹「重命名／覆盖／取消」对话框。原实现批量处理ef2文件时是**同步连续调用COM**，模态框叠加后IDM与本工具互相等待：第一个框还能操作，后面的框点是／否／取消都无响应，只能结束IDM进程重启。

修复后的处理：
1. 默认 `lFlags=1`（官方定义：do not show any confirmations dialogs），不再产生任何确认框，死锁从根子上消失；
2. 不再给批量链接自动追加 `-q`，全部直接开始下载，也去掉了「请自行到IDM里开始队列」提示框；
3. 发送前统一对「保存目录 + 文件名」去重，重复项自动改名 ` (2)`、` (3)`… 并覆盖该条目原有的 `filename`；
4. 顺带修正：`-a`（User-Agent）此前从未生效（误把URL塞进了 `reserved1`）、CRLF行尾的ef2文件无法解析、COM接口指针与 `CoInitialize` 未成对释放。

`src/code.h` 的原始版本保留在 `backup/code.h.orig-20261108`，`tests/ef2_logic_test.cpp` 是不启动真实IDM的回归验证（mock COM接口，打印每条链接最终送入IDM的参数）。

#### ※ IDM 403 错误框的自动关闭
「服务器响应显示您没有权限下载此文件 / HTTP 1.1 403 Forbidden」是 **IDM** 在下载过程中自己弹的运行时错误框：ef2 不发起 HTTP 请求，`lFlags=1` 也只作用于「添加链接」阶段的确认框，管不到它；IDM 本身没有关闭这类错误框的开关。

现在发完链接后 ef2 会短时驻留 45 秒，精确识别并自动点「确定」：只匹配可见的 `#32770` 标准对话框、且子控件文本含 `403` / `Forbidden` / `Server responded` / `服务器响应` / `没有权限`，命中后按 IDOK 并补一个 WM_CLOSE 兜底。IDM 主窗口、「下载文件信息」框、「文件已存在」框都不含这些文字，不会被误关（实测 IDM 6.43 主窗口同为 `#32770`，但子控件文本是 `分类`、`查找:` 等）。

用命名互斥量保证同一时刻只有一个轮询进程，连点多个 ef2 文件不会堆积进程，已有的那个轮询进程会一并处理后续文件产生的弹窗；45 秒后自动退出。不想启用就在 ef2 文件里写一行 `errorwatch: 0`。`tests/errorwatch_test.cpp` 是造两个假对话框验证「只关 403 框、不误伤普通框」的回归测试。

边界：只覆盖双击 .ef2 文件和 ef2:// 协议两种入口，websocket / nativeMessaging 常驻模式不启用；用 `-q` 排队、很久之后才手动开始的下载，弹窗可能落在 45 秒窗口之外。若某些 IDM 版本的错误框不是标准对话框类，该功能会失效。

---
### 编译相关
| 环境 | 版本 |
| --- | --- |
| Windows 11 | 10.0.22000.1 |
| VSCode | 1.63.2 |
| MSVC | 19.30.30709 |
| Windows SDK | 10.0.22000.0 |

说明：
1. 需要在Developer Command Prompt for VS中启动VSCode，或者手动将MSVC及Windows SDK添加进系统环境变量。
2. 打开main.cpp文件然后f5启用调试，或者运行任务`C/C++: cl.exe build`直接编译发行版。
3. 任务`tsc: build - typescript/tsconfig.json`用来编译ef2.ts到js文件，输出目录为dist。
4. 本修改版已在 MSVC 14.51.36231 + Windows SDK 10.0.26100.0（Visual Studio 18 Enterprise）下实测编译通过；手动配置环境变量时须包含 `atlmfc\include`（`atlbase.h` 所在目录）。
5. `*.bat` 由 `.gitattributes` 强制 CRLF 行尾，改成 LF 会出现「命令不存在」；仓库内的编译产物与本地备份由 `.gitignore` 排除。
6. 附带两个不启动真实 IDM 的回归测试：`tests/ef2_logic_test.cpp`（mock COM，核对送入 IDM 的参数与自动重命名）和 `tests/errorwatch_test.cpp`（造两个假对话框，验证只关 403 框、不误伤普通框），单独编译运行即可。

---
### 开源许可
本项目沿用 [MIT License](LICENSE)：原版权 `Copyright (c) 2021 MotooriKashin` 与许可全文完整保留在 [LICENSE](LICENSE) 中，未作删改；本仓库新增及修改的代码同样按 MIT 许可提供。

派生自 [MotooriKashin/ef2](https://github.com/MotooriKashin/ef2)（MIT License）。

项目中参考其他源码及文章：
| 项目或文章 | 开源协议 |
| ------- | ------- |
| [idm_api](http://www.internetdownloadmanager.com/support/idm_api.html) | 未知 |
| [IDMHelper](https://github.com/unamer/IDMHelper) | [GPL-3.0 License](https://github.com/unamer/IDMHelper/blob/master/LICENSE) |
| [mingw控制台中文乱码](https://www.cnblogs.com/chouxianyu/p/11249810.html) | 未知 |
| [C++实现utf8和gbk编码字符串互相转换](https://blog.csdn.net/u012234115/article/details/83186386) | 未知 |
| [base64](https://github.com/tkislan/base64) | [MIT License](https://github.com/tkislan/base64/blob/master/LICENSE) |
| [HeadSocket](https://github.com/garbageslam/HeadSocket) | 未知 |
