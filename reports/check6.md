# CS144 Fall 2026 Checkpoint 6：IPv4 路由器

合并官方 `cs144/imp` 的 `check6-startercode`，在已验证的 NetworkInterface
上实现 Router。要求：https://cs144.github.io/assignments/check6.pdf 。
使用独立 `edidada/check6` 分支和 `check6.yml`，保留原测试。

路由项保存规范化前缀、掩码、前缀长度、下一跳及出口接口。逐项扫描选择匹配
目标地址且前缀最长的一项；/0 使用零掩码，避免将 32 位整数右/左移 32 位。
/32 保留全部位。大于 32 的前缀长度显式拒绝；同长度匹配保留最早加入项。
每次调用清空各接口的待收数据报：无路由或 TTL≤1 时丢弃，否则 TTL 减一、
重算校验和，再通过出口 NetworkInterface 交付。直连网络的下一跳是最终目的 IP。
查询每包 O(路由项数)，存储 O(路由项数)，符合教学规模；未实现动态路由协议。

仅在 GitHub Actions 构建运行官方 check6/模拟拓扑测试、此前单元回归、
ASan/UBSan、速度及既有 TCP 真实网络回归；本地仅做静态检查。
最终提交的 completed/success CI 和 JUnit 工件才是实际验收依据。
官方路由测试使用模拟以太网，不把它称为真实跨主机路由器部署。
未实现 ICMP 错误响应，课程明确允许静默丢弃。

Checkpoint4 的一小时测量及同学私人路径、此前 hands-on 和 Checkpoint7
尚未完成。由 Codex 按用户自学任务编写，不声称已取得课程评分或完整课程验收。
