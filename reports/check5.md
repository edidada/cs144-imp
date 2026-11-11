# CS144 Fall 2026 Checkpoint 5：网络接口与 ARP

合并官方 `cs144/imp` 的 `check5-startercode`，实现 NetworkInterface。
要求：https://cs144.github.io/assignments/check5.pdf 。独立分支
`edidada/check5` 与工作流 `check5.yml`，保留原测试。

映射表按下一跳 IPv4 地址保存 MAC 与剩余有效时间；命中时发送封装后的 IPv4
以太网帧。未知地址按下一跳排队，只在首次请求时广播 ARP，五秒内不重复请求；
五秒期满丢弃未解析队列，后续发送可以重新请求。ARP 请求和响应均学习映射，
有效期三十秒，刷新时重新计时；学习后按顺序发送等待中的数据。
仅处理发给本接口或广播的帧，解析有效 IPv4 后向上交付；对查询自身 IP 的
ARP 请求返回应答。倒计时更新避免大 tick 引发加法溢出。

平均每次映射查询/更新 O(log n)，tick 遍历所有缓存和未解析下一跳。
未解析目标的队列最多保留五秒，但该教学实现没有逐目标字节容量限制，
不应将其当作已经具备生产环境的抗队列耗尽能力。

构建与测试仅在 GitHub Actions 执行。本地只检查源码和 Git 差异。
远端执行官方 check5、网络接口 ASan/UBSan 测试、此前代码和速度回归，
并再次验证已有自写 TCP 对 Linux/公网和配置丢包的传输。
结果以最终提交 SHA 的 completed/success 工作流、JUnit 与日志为准。
本次不是物理 NIC 驱动测试；Checkpoint4 测量及 Checkpoint6–7 尚未完成。
由 Codex 按用户自学任务编写，未虚构课堂身份或成绩。
