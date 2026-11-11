# CS144 Fall 2026 Checkpoint 2：TCP 接收端

## 来源与实现

以 Checkpoint 1 最终提交为基础，合并官方 `cs144/imp` 的
`check2-startercode`。保留原测试，使用独立 `edidada/check2` 分支及工作流。
官方要求：https://cs144.github.io/assignments/check2.pdf 。

Wrap32::wrap 使用 uint32_t 截断实现模 2^32 加法。unwrap 在 checkpoint
所在的 2^32 周期和相邻周期中选择距离最近的有效 uint64_t，先检查边界，
避免接近零或 UINT64_MAX 时发生溢出。相同距离选择当前周期。

TCPReceiver 只在首个 SYN 到来时记录 ISN，此前丢弃普通数据；重复 SYN
不会重置已收到的数据。将序号展开到绝对序号，再扣除 SYN 占据的一个位置，
把 payload 和 FIN 交给 Reassembler。ACK 覆盖 SYN、连续数据，以及已组装的 FIN；
未填补的乱序缺口不推进 ACK。窗口取 ByteStream 可用容量并限制在 UINT16_MAX。
RST 设置输出流错误，错误状态不再处理数据，并在反馈中带上 RST。

## 验证与边界

本地不编译、不运行实验测试。GitHub Actions 负责官方 check2、五类 Wrap32
测试、七类接收器测试、Checkpoint 0/1 回归、ASan/UBSan 和官方速度检查。
最终结论需以最终提交 SHA 对应的 completed/success 工作流和 JUnit 为证据。

这是自学代码实验，尚未实现 Checkpoint 3 发送端和后续网络接口/路由器，
不声称已实现完整 TCP 连接或完成此前尚缺的 hands-on 网络实验。
未虚构课堂身份、提交成绩或现场评分。实现由 Codex 按用户任务编写。
