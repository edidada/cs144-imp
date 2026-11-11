# CS144 Fall 2026 Checkpoint 0 实验报告

官方起点为 CS144/imp 的 837d45af2ce837c4ff9cb29bfd2cfaff2578a6a5，
要求以 https://cs144.github.io/assignments/check0.pdf 为准。独立分支 edidada/check0，
工作流 .github/workflows/check0.yml。只在 GitHub Actions 的 Ubuntu 26.04 容器编译运行。

## 实现

ByteStream 用 deque<string> 保存已接受的数据块、记录首块偏移及缓冲/累计读写计数。
push 只接受剩余容量内的前缀；peek 不复制数据；pop 跨块释放，不反复搬移整个缓冲区。
close 与数据排空分开：close 后仍可读取，缓冲清空后才 finished。保留原错误标志接口。

webget 使用课程 TCPSocket、Address、write_all 发送含 Host 和 Connection: close 的
HTTP/1.1 GET，请求完整写入后循环读到 EOF，将完整 HTTP 响应原样输出，包括二进制字节。

## 验证与范围

远端运行脚本 scripts/verify_check0.py 保留原 cmake check0（含公开 HTTP 哈希服务），
再记录 ASan/UBSan 原测试、速度基准、独立四容量各一万步队列 oracle，
以及本地回环服务的请求校验、分段响应及 2MiB 二进制响应。
预期 CTest 包括编译 fixtures 共 12 项；尚待首次远端运行，不提前登记通过。

原测试、参考及编译告警未减弱。公共检查不等于 Stanford 私有 grader；
手工邮件练习未执行，也未向任何第三方发送邮件。后续 Checkpoint 1–7 未完成。
详细命令、工具链和结果随远端工件 cs144-check0-results 保存。
