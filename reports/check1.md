# CS144 Fall 2026 Checkpoint 1：字节流重组

## 来源与范围

基于 `cs144/imp` 的 `check1-startercode`，合入已有 Checkpoint 0 实现。
保留官方测试，新增独立 `edidada/check1` 分支和 `check1.yml`。
这是自学实验；未填写虚构 SUNet ID 或课堂合作信息。

## 实现

用容量固定的环形字节数组和位图保存尚不能写入的乱序数据。
绝对索引对容量取模；接收窗口为 `[bytes_pushed, bytes_pushed + available_capacity)`。
窗口长度不超过容量，因此不同待重组索引不会占用同一个槽。
已写入的前缀和窗口外的数据丢弃；重叠部分保留首个已收字节，不重复计数。
缺口填上后，一次将连续前缀推入 ByteStream，并清除对应位图。

EOF 单独记录其绝对位置，即使同一片段的数据因容量限制被截断，仍保留结束位置。
只有全部字节写入后才关闭输出；零容量的空 EOF 也能立即关闭。
索引上界运算防止 uint64_t 溢出。pending 数通过位图计算，没有额外计数状态。

存储开销 O(capacity)，插入 O(输入窗口内字节数 + 新拼接字节数)，
pending 查询 O(capacity)。与区间树相比不需要逐片段节点和区间合并，但很大的容量
会预先占用对应数组，pending 查询也不适合当作频繁的业务操作。

## 验证

构建和测试只在 GitHub Actions 执行，本地仅做静态检查。
工作流运行官方 check1、七类重组测试、Checkpoint 0 回归、ASan/UBSan，
以及官方重组和字节流速度测试（最低速度由官方测试断言）。
实际结果以最终提交 SHA 的 completed/success 工作流和 JUnit 工件为准。

课程 hands-on 要求的私人 WireGuard 网络、同学协作 ping 和原始协议实验
尚未完成，不能用公开 C++ 测试通过替代这些要求。后续 Checkpoint 2–7 未在本报告验收。

实现由 Codex 根据用户的全课程实验任务编写，并使用官方测试进行远端验证。
