# CKafka

## 1.Kafka ACK

acks参数控制生产者发送消息的确认机制，也就是控制生产者发送消息，Broker端确认成功写入的条件

消息写入kafka 过程:

对于某个topic-partition 

1.生产者将消息发给该分区的leader broker

2.leader broker将消息追加本地日志 更新内存/页缓存（不等磁盘fsync

3.leader把数据复制给follower

4.follower 复制完成后返回leader

5.leader根据acks规则决定什么时候返回生产者 suc/fal

acks = 0

生产者不等任何broker响应，发出去就当成功

acks = 1

leader写入本地日志后就返回成功，不等follower复制

（**已 ack 仍可能丢**：如果 leader 写入后立刻宕机，而 follower 还没复制到，这条消息可能不在新 leader 上 → 丢失）

acks = all

leader需要等待ISR集合里所有的副本都确认写入才返回成功

（ISR是与leader同步的一组副本集合，如果落后太多会被踢出）

（min.insync.replicas 指的是ISR中至少的副本数量，ack =all 时候，min.insync.replicas > 1,这样至少两个副本写成功才ack，leader宕机不丢）

消息重复问题

网络抖动导致生产者没有收到ack 生产者认为失败并重试，其实broker写入成功了，生产者重试写入，导致消息重复

解决

enable.idempotence=true  acks=all 幂等producer会给每个分区的消息带序列号，broker可以识别并去重

##### 项目里如何ack？

使用librdkafka 异步 producer，`produce()` 只代表进入客户端队列，为了确认 broker 端是否按 acks 语义确认写入，我注册了 Delivery Report 回调并在发送线程里持续 `poll()` 驱动回调执行。运行过程中我统计 `produced_ok / delivered_ok / delivered_fail`，并监控 `outq_len` 和 `QUEUE_FULL` 次数衡量背压。退出时 `flush()` 并输出最终投递成功率和失败原因日志，因此可以用数据证明消息真实投递情况，而不是只停留在“入队成功”。

`acks=all` 只能保证 broker 端在 ISR 副本确认后才返回成功，但仍可能因为 producer 崩溃导致消息没发出去，或者因为 min.insync.replicas 配置过低导致只剩 leader 也会 ack。我们会用 delivery report 确认最终投递结果，并配合 replication.factor>=3、min.insync.replicas>=2 来避免“假成功”。对关键数据进一步用本地 WAL 做兜底，失败时可阻塞重试或降级落盘。