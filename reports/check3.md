# CS144 Fall 2026 Checkpoint 3：TCP 发送端

## 来源与设计

在 Checkpoint 2 最终实现上合并官方 `cs144/imp` 的 `check3-startercode`。
要求见 https://cs144.github.io/assignments/check3.pdf 。保留官方测试，建立
独立 `edidada/check3` 分支和 `check3.yml`，本地不编译或运行实验。

发送端维护绝对 next/acknowledged 序号、接收窗口、FIN 状态和待确认消息队列。
push 尽量填满窗口，payload 不超过 MAX_PAYLOAD_SIZE；SYN/FIN 各占一个序号。
零窗口仅在填充窗口时按一处理，实际记录仍为零。make_empty_message 不推进序号。
累计 ACK 删除完全确认的消息，部分确认保留原消息以供重传。
新的有效 ACK 恢复初始 RTO，重置计时和连续重传数；旧/重复/未来 ACK 不重置计时。

计时只使用 tick 的毫秒参数。存在待确认消息才计时，超时只重传最早消息；
非零窗口下指数退避并增加连续重传数，零窗口探测不退避。累计时间及 RTO
采用饱和运算避免整数回绕。RST 与字节流错误状态相联系。

## 验证与未完成范围

远端工作流运行官方 check3、发送端七类测试、Checkpoint0–2 回归、
ASan/UBSan、官方速度和已有 oracle；最终结果需匹配最终提交 SHA。
未修改官方测试，也没有用模拟通过替代它们。

Checkpoint3 还要求 hands-on：同一 VM 与 Linux TCP 交互、同学连接、跨互联网
交流，以及让 webget 使用自写 TCP。当前实现提交尚未完成这些活动，后续需要
TUN 设备、网络配置和真实交互证据；公共单元测试通过不等于整个实验验收。
Checkpoint4–7 尚未完成。由 Codex 按用户自学任务编写，未虚构课堂身份或成绩。
