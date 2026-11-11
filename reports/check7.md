# CS144 Fall 2026 Checkpoint 7：进行中

官方要求：https://cs144.github.io/assignments/check7.pdf 。合并官方
check7-startercode，保留两份应用及原测试；独立 edidada/check7 分支、check7.yml。
由 Codex 按自学任务实现自动验证，不作为课程提交或独立完成声明。

scripts/check7_internet.sh 实现讲义单机 tiny Internet：Stanford 50.9.8.7:80、
UCLA 80.6.5.4，网关分别 50.0.0.1、80.0.0.1；路由器直连 50/8、80/8。
外层 UDP 全部走 127.0.0.1，端口为讲义 2050/9050、2080/9080。
两个端点同时发送不同的 1 MiB 随机文件，以 cmp 和 SHA256 验证双向精确交付。
抓取 Ethernet-over-UDP PCAP，检查 ARP 请求/回复及 TCP SYN；普通与
ASan/UBSan 应用分别执行。仅 GitHub Actions 运行，本地不编译运行。

验收须等待最终 SHA 的 completed/success 及工件，当前不声明通过。
尚未完成：创意程序、三位真实参与者网络、更大拓扑/同时连接及课程 PDF 报告。
单机三进程不等于三人联网。此前 Checkpoint4/私人网络 hands-on 也仍未完成。
