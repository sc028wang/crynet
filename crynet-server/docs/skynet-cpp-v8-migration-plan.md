# Skynet 功能梳理与 `C++ + V8` 超高性能移植规划

> 生成时间：2026-04-03  
> 目标目录：`crynet-server/docs`  
> 分析范围：`/Users/alex/workspaces/crynet/skynet`

---

## 1. 文档目标

本文档用于系统梳理 `skynet` 目录下现有框架能力，并形成一份面向 **`C++ + V8` 超高性能版本** 的迁移业务清单与技术拆分方案。

核心结论：

- `skynet` 本质上是一个 **Actor 模型的高并发服务框架**，并非单一业务库；
- 它的强项在于：**消息驱动、网络接入、定时器调度、服务隔离、集群路由、脚本宿主能力**；
- 如果迁移到 `C++ + V8`，应当保留其 **运行时内核设计**，只替换/升级 **Lua 服务执行层**。

---

## 2. 总体结构概览

`skynet` 当前目录可分为 5 个层次：

1. **运行时内核**：`skynet-src/`
2. **内置服务**：`service-src/`
3. **第三方依赖**：`3rd/`
4. **测试与样例**：`test/`
5. **构建系统**：`Makefile`、`platform.mk`

### 2.1 关键入口

- `skynet-src/skynet_main.c`：主入口，加载配置，初始化环境，启动框架
- `skynet-src/skynet_start.c`：启动线程模型，包括：
  - `worker` 线程
  - `socket` 线程
  - `timer` 线程
  - `monitor` 线程

这说明 `skynet` 的核心不是脚本本身，而是一个 **多线程 + 单 Actor 串行处理** 的运行时。

---

## 3. 核心功能分析

## 3.1 Actor / Service 运行时

**关键文件：**
- `skynet-src/skynet_server.c`
- `skynet-src/skynet_handle.c`
- `skynet-src/skynet_mq.c`
- `skynet-src/skynet.h`

**核心能力：**
- 每个服务是一个独立 `context`
- 每个服务有独立消息队列
- 服务之间通过消息传递通信
- 使用 `session` 关联请求/响应
- 支持服务命名与句柄查找
- 保证单个服务回调串行执行，降低锁竞争

**业务意义：**
- 非常适合游戏服、网关服、逻辑服、战斗服、匹配服、聊天服等高并发拆分场景
- 天然适合微服务式进程内编排

---

## 3.2 消息队列与调度系统

**关键文件：**
- `skynet-src/skynet_mq.c`
- `skynet-src/skynet_start.c`

**功能特征：**
- 每服务一个消息队列
- 全局调度队列负责分发待执行服务
- 队列支持扩容
- 内置 overload 检测
- worker 按权重调度执行

**结论：**
这是后续 `C++ + V8` 版本里最应该优先保留的能力之一，因为它直接决定吞吐、尾延迟和高峰稳定性。

---

## 3.3 网络 I/O 子系统

**关键文件：**
- `skynet-src/socket_server.c`
- `skynet-src/skynet_socket.c`
- `skynet-src/socket_epoll.h`
- `skynet-src/socket_kqueue.h`
- `service-src/service_gate.c`

**现有能力：**
- TCP / UDP / IPv6
- accept / connect / close / error / warning 事件
- 高低优先级发送队列
- backpressure 警告
- 网关服务 `gate`
- 拆包/粘包处理
- 每连接绑定业务 agent 或 broker

**业务场景：**
- 登录网关
- 长连接会话管理
- 游戏实时消息
- 内网服务协议转发
- UDP 类低延迟场景

**迁移建议：**
- 网络层必须继续保留在 Native `C++`
- `V8` 只接收已解好的业务消息，不直接介入 socket 热路径

---

## 3.4 定时器系统

**关键文件：**
- `skynet-src/skynet_timer.c`
- `skynet-src/skynet_timer.h`

**能力：**
- 分层时间轮
- `timeout` 事件投递
- 定时 wakeup / coroutine 配合

**用途：**
- 业务超时控制
- 重试机制
- 定时任务
- 心跳与会话过期

**迁移建议：**
- 保持内核级定时器，不要交给 JS `setTimeout` 直接驱动核心逻辑
- JS 层可以封装，但底层事件调度仍应用 `C++` 时间轮或高性能 timer queue

---

## 3.5 集群 / Harbor 路由

**关键文件：**
- `skynet-src/skynet_harbor.c`
- `service-src/service_harbor.c`

**能力：**
- 多节点 harbor 通信
- 远程服务消息转发
- 全局名字注册与查询
- 节点断连状态上报

**业务意义：**
- 支持跨节点服务发现
- 支持水平扩展
- 支持逻辑分区部署

**迁移建议：**
- `harbor` 协议可以保留语义，但实现层建议重写为更清晰的 `C++` cluster router
- 后续可以扩展成跨机房、多租户、服务发现注册中心模式

---

## 3.6 模块系统 / 动态加载

**关键文件：**
- `skynet-src/skynet_module.c`

**能力：**
- 动态加载 `.so`
- 模块生命周期：`create/init/release/signal`
- 内置 C service 插件机制

**迁移建议：**
- 新版本中建议统一为：
  - Native service plugin (`.so/.dylib`)
  - JS service bundle (`.js/.mjs`)
- 建立统一 `ServiceFactory` 抽象

---

## 3.7 脚本宿主（Lua）

**关键文件：**
- `service-src/service_snlua.c`

**能力：**
- Lua VM 宿主
- coroutine 支持
- profile 统计
- 内存限制与 trap
- 脚本服务加载

**迁移含义：**
这部分就是未来 `V8` 替换点。

建议替换为：
- `service_snv8` 或 `service_v8js`
- 每 worker 绑定一个 `V8 Isolate`
- 每个 service 一个 `Context`
- 暴露等价 JS API：
  - `skynet.newService()`
  - `skynet.send()`
  - `skynet.call()`
  - `skynet.timeout()`
  - `skynet.fork()`

---

## 3.8 日志、监控与内存追踪

**关键文件：**
- `service-src/service_logger.c`
- `skynet-src/skynet_monitor.c`
- `skynet-src/mem_info.c`
- `skynet-src/malloc_hook.c`

**能力：**
- 文本日志
- 日志文件 reopen
- 死循环/阻塞检测
- 内存统计与告警
- CPU 时间统计

**迁移建议：**
- 新版本必须从第一天开始就有：
  - `metrics`
  - `trace`
  - `p99/p999 latency`
  - `queue depth`
  - `GC 时间`
  - `socket backlog`

---

## 4. 内置服务清单

| 服务 | 文件 | 功能 | 迁移建议 |
|---|---|---|---|
| `snlua` | `service-src/service_snlua.c` | Lua 服务宿主 | 替换为 `V8` 宿主 |
| `gate` | `service-src/service_gate.c` | 连接接入、转发、拆包 | 原生 `C++` 保留 |
| `harbor` | `service-src/service_harbor.c` | 集群路由 | `C++` 重写 |
| `logger` | `service-src/service_logger.c` | 日志输出 | 先保留，增强结构化日志 |

---

## 5. 扩展能力与外部接入清单

结合 `Makefile` 和 `test/` 可确认当前能力覆盖：

### 5.1 协议与序列化
- `bson`
- `sproto`
- `md5`
- `sha1`
- `crypt`
- `lpeg`
- 可选 `TLS`

### 5.2 数据系统
- Redis
- Redis Cluster
- MySQL
- MongoDB

### 5.3 网络应用层
- HTTP
- HTTPS
- DNS

### 5.4 共享数据能力
- `sharetable`
- `sharedata`
- `datasheet`
- `stm`
- `multicast`

这些能力从业务角度说明：`skynet` 不只是调度框架，还附带了相当完整的服务端生态基础件。

---

## 6. 业务清单（面向后续产品化）

下面这份清单，建议作为后续 `C++ + V8` 版本对外承诺的“业务底座能力”。

### 6.1 接入层业务
- [x] TCP 长连接接入
- [x] UDP 通讯
- [x] IPv4 / IPv6
- [x] 连接管理（accept / close / error）
- [x] 网关拆包转发
- [x] 流控与发送优先级

### 6.2 服务编排业务
- [x] Actor 服务模型
- [x] 异步消息投递
- [x] 请求-响应 session 机制
- [x] 服务命名与发现
- [x] 定时任务 / 超时回调
- [x] 协程化业务编排

### 6.3 分布式业务
- [x] 节点间路由
- [x] 远程服务调用
- [x] 全局名字服务
- [x] 集群扩缩容基础能力

### 6.4 数据共享业务
- [x] 共享只读配置表
- [x] 热更新表数据
- [x] 数据快照/查询
- [x] 发布订阅

### 6.5 外部资源接入
- [x] Redis
- [x] MySQL
- [x] MongoDB
- [x] HTTP / HTTPS
- [x] DNS

### 6.6 稳定性与运维业务
- [x] 日志系统
- [x] 内存统计
- [x] 线程/服务监控
- [x] 性能 profiling
- [x] daemon 化

---

## 7. `C++ + V8` 版本的目标架构

建议架构分为 4 层：

### 7.1 Layer 1：Native Runtime Core
由 `C++` 实现：
- Actor 调度器
- 消息队列
- timer wheel
- handle registry
- socket reactor
- cluster router
- memory pool / slab allocator

### 7.2 Layer 2：V8 Script Host
由 `V8` 承载：
- JS service runtime
- Promise / coroutine 风格桥接
- JS 模块加载
- JS service context

### 7.3 Layer 3：Built-in Native Services
- gate
- logger
- harbor
- metrics
- registry
- config center

### 7.4 Layer 4：Business JS Services
- 登录服务
- 网关代理服务
- 场景/战斗服务
- 缓存代理服务
- 任务/活动服务
- 数据同步服务

---

## 8. 迁移优先级清单

## P0：必须先完成

1. **运行时内核重建**
   - `context`
   - `message_queue`
   - `scheduler`
   - `timeout`
   - `service lifecycle`

2. **高性能网络层**
   - `epoll/kqueue`
   - TCP/UDP
   - write buffer
   - 连接管理
   - `gate` 模式

3. **V8 宿主层**
   - isolate 管理
   - context 生命周期
   - JS ↔ C++ bridge
   - JS 版 `send/call/timeout`

4. **监控基础件**
   - logger
   - metrics
   - tracing

## P1：第二阶段
- cluster/harbor
- sharetable/sharedata
- Redis/MySQL/MongoDB adapter
- HTTP/DNS
- multicast

## P2：增强阶段
- 热更新
- 更强的 debug 能力
- WASM/Native hybrid service
- 自动化压测与性能回归平台

---

## 9. 面向超高性能的关键设计原则

考虑到目标是 **超百万级瞬时访问性能**，建议新版本一开始就按以下标准设计：

### 9.1 热路径全部 Native 化
不要把这些路径放到 `V8`：
- socket 收发
- 协议拆包
- queue push/pop
- timer dispatch
- cluster route
- memory allocation fast path

### 9.2 服务执行语义保持不变
保持：
- **单服务串行执行**
- **服务之间并发执行**

这是 `skynet` 易用且稳定的根本。

### 9.3 V8 隔离策略建议
推荐：
- **每 worker 一个 `Isolate`**
- **每 service 一个 `Context`**

不建议：
- 每连接一个 isolate
- 过度频繁跨 C++ / JS 边界切换

### 9.4 内存与对象池
建议实现：
- slab allocator
- fixed-size pool
- zero-copy / ref-count message block
- 大包单独 arena

### 9.5 可观测性必须前置
至少要有：
- 每服务队列深度
- 每 worker 利用率
- 每类消息吞吐
- socket backlog
- GC pause
- p50 / p95 / p99 / p999 latency

---

## 10. 建议的新目录拆分

可为 `crynet-server` 设计成如下结构：

```text
crynet-server/
  core/
    actor/
    scheduler/
      timer/
    socket/
    cluster/
    memory/
    metrics/
  startup/
    app/
  engine/
    v8/
    engine/
    bindings/
    loader/
    builtins/
  services/
    gate/
    logger/
    registry/
    harbor/
  adapters/
    redis/
    mysql/
    mongo/
    http/
  tests/
    unit/
    integration/
    benchmark/
  docs/
    skynet-cpp-v8-migration-plan.md
```

---

## 11. 第一阶段落地范围建议

为了尽快形成可运行最小闭环，建议第一期只做下面这些：

### MVP 能力
- `C++` Actor runtime
- `gate` 网络接入
- `V8` service 宿主
- `send/call/timeout/newservice`
- 基础 logger
- 单机部署

### MVP 验证指标
- 单机长连接稳定接入
- Echo / PingPong / 登录鉴权可跑通
- 1 万 ~ 10 万连接稳定
- 队列、CPU、内存指标可观测

### Phase-2 指标
- 10 万 ~ 50 万连接
- 横向节点扩展
- Redis/MySQL 实战接入
- JS 服务热更新

### Phase-3 指标
- 百万级瞬时连接/消息洪峰压测
- 压测基线稳定沉淀
- GC 与尾延迟持续优化

---

## 12. 最终结论

`skynet` 的价值不在 Lua 本身，而在它的 **高并发 Actor Runtime 设计**。

因此迁移到 `C++ + V8` 时，正确路线不是“简单语言替换”，而是：

> **保留 `skynet` 的运行时思想与调度语义，用 `C++` 重写高性能内核，用 `V8` 取代 Lua 业务执行层。**

这样既能继承原有工程模型，又能显著提升：
- 脚本生态能力
- 原生性能上限
- 可维护性
- 大规模并发场景下的可扩展性

---

## 13. 本次分析依据

已检查的关键路径包括：
- `skynet/README.md`
- `skynet/Makefile`
- `skynet/platform.mk`
- `skynet/skynet-src/*.c,*.h`
- `skynet/service-src/*.c`
- `skynet/test/*.lua`

当前结论可作为后续架构设计、模块拆分、MVP 研发排期和性能压测基线设计的直接输入。

---

## 14. 项目开发任务清单（执行版）

下面这份清单按 **里程碑 + 可交付物 + 验收标准** 拆分，建议后续开发严格按顺序推进。

## 14.1 里程碑总览

| 里程碑 | 目标 | 预计输出 |
|---|---|---|
| `M0` | 完成工程初始化与基线 | 构建系统、目录骨架、基准测试框架 |
| `M1` | 跑通原生运行时内核 | Actor、消息队列、调度器、定时器 |
| `M2` | 跑通网络接入与网关 | TCP/UDP、gate、echo/login demo |
| `M3` | 接入 `V8` 并支持 JS 服务 | isolate/context、JS API、service loader |
| `M4` | 建立可观测与压测体系 | metrics、trace、benchmark、profiling |
| `M5` | 补齐集群与外部适配 | harbor、Redis/MySQL/Mongo/HTTP |
| `M6` | 进入生产化优化 | 热更新、稳定性治理、百万级压测 |

---

## 14.2 M0：工程初始化与研发基线

### 任务清单
- [ ] `T0-1` 创建 `crynet-server` 目录骨架
- [ ] `T0-2` 建立 `CMake` 主构建系统
- [ ] `T0-3` 引入 `V8` 依赖管理方案（源码或预编译）
- [ ] `T0-4` 建立统一编译选项、告警级别、Sanitizer 配置
- [ ] `T0-5` 建立 `tests/unit`、`tests/integration`、`tests/benchmark`
- [ ] `T0-6` 建立基础 CI（至少包含 build + unit test）
- [ ] `T0-7` 建立压测脚手架与基线记录模板

### 可交付物
- `CMakeLists.txt`
- 标准化目录结构
- 可执行 `Debug/Release` 构建
- 基准测试入口程序

### 验收标准
- 本地 macOS 可成功编译
- 可生成核心二进制与测试二进制
- benchmark 框架可执行并输出结果文件

---

## 14.3 M1：运行时内核 MVP

### 任务清单
- [ ] `T1-1` 实现 `ServiceContext` 抽象
- [ ] `T1-2` 实现 `HandleRegistry`（服务 ID / name 映射）
- [ ] `T1-3` 实现 `Message` 数据结构与消息类型系统
- [ ] `T1-4` 实现每服务独立消息队列
- [ ] `T1-5` 实现全局调度队列与 worker 调度器
- [ ] `T1-6` 实现 session / response 机制
- [ ] `T1-7` 实现 timer wheel / timeout 投递
- [ ] `T1-8` 实现 service 生命周期：`create/init/start/stop/release`
- [ ] `T1-9` 实现基础配置加载与 bootstrap 启动流程
- [ ] `T1-10` 实现 logger 初版

### 可交付物
- 纯 `C++` 运行时可启动
- 可创建多个 service 并互发消息
- 可完成 timeout 回调与 request/response

### 验收标准
- 单元测试覆盖：handle、queue、timer、session
- Echo 型 actor demo 跑通
- 10 万级消息投递无异常退出

---

## 14.4 M2：网络层与 Gate 服务

### 任务清单
- [ ] `T2-1` 实现 `epoll/kqueue` reactor 抽象
- [ ] `T2-2` 实现 TCP listen/connect/accept/close
- [ ] `T2-3` 实现 UDP / IPv6 支持
- [ ] `T2-4` 实现 write buffer / backpressure 控制
- [ ] `T2-5` 实现 socket 事件投递到 Actor
- [ ] `T2-6` 实现 `gate` 服务 MVP
- [ ] `T2-7` 实现协议拆包器（支持 length-header）
- [ ] `T2-8` 实现 `echo server` / `login gateway` 示例

### 可交付物
- `socket` 子系统
- `gate` 原生服务
- 连接接入 demo

### 验收标准
- 单机稳定 accept 大量连接
- 能完成 echo/pingpong/login demo
- 有初步吞吐与延迟数据

---

## 14.5 M3：`V8` 宿主与 JS 服务运行时

### 任务清单
- [ ] `T3-1` 实现 `V8Platform` / `IsolatePool`
- [ ] `T3-2` 实现每 worker 一个 `Isolate` 策略
- [ ] `T3-3` 实现每 service 一个 `Context`
- [ ] `T3-4` 实现 JS 模块加载器
- [ ] `T3-5` 实现 JS API：`send/call/timeout/newService`
- [ ] `T3-6` 实现 JS Promise/协程风格桥接
- [ ] `T3-7` 实现 `service_v8` 宿主服务
- [ ] `T3-8` 提供 `hello service`、`echo service`、`timer service` 示例

### 可交付物
- JS 服务可以注册、启动、收发消息
- 可以从 JS 调用原生 runtime API

### 验收标准
- JS 示例服务可正常启动
- `send/call/timeout` 行为与预期一致
- 运行过程中无明显 isolate/context 泄漏

---

## 14.6 M4：可观测性、测试与压测体系

### 任务清单
- [ ] `T4-1` 实现 metrics 指标采集
- [ ] `T4-2` 实现 queue depth / worker 利用率统计
- [ ] `T4-3` 实现 socket backlog / send queue 指标
- [ ] `T4-4` 接入 tracing / span 追踪能力
- [ ] `T4-5` 记录 `V8 GC` 时间与内存占用
- [ ] `T4-6` 建立 unit/integration/benchmark 回归流程
- [ ] `T4-7` 建立 1 万、5 万、10 万连接压测脚本
- [ ] `T4-8` 输出第一版性能基线报告

### 可交付物
- 可观测面板数据源
- benchmark 报告
- 压测脚本与结果归档

### 验收标准
- 能稳定输出 p50/p95/p99/p999
- 能识别消息堆积、GC 抖动、网络瓶颈
- 每次改动后可自动回归性能

---

## 14.7 M5：集群与外部适配层

### 任务清单
- [ ] `T5-1` 实现 cluster router / harbor 协议
- [ ] `T5-2` 实现远程服务发现与 global name registry
- [ ] `T5-3` 实现 `Redis` adapter
- [ ] `T5-4` 实现 `MySQL` adapter
- [ ] `T5-5` 实现 `MongoDB` adapter
- [ ] `T5-6` 实现 `HTTP/DNS` adapter
- [ ] `T5-7` 实现 sharetable / config snapshot 初版

### 可交付物
- 多节点通信 demo
- 数据库接入 demo
- 配置共享/读取能力

### 验收标准
- 两节点消息路由打通
- Redis/MySQL 至少具备生产可用最小接口
- 基础容错与超时处理到位

---

## 14.8 M6：生产化与高性能优化

### 任务清单
- [ ] `T6-1` 优化内存池、对象池、零拷贝消息块
- [ ] `T6-2` 优化锁竞争与跨线程投递开销
- [ ] `T6-3` 优化 isolate 调度和 JS/native 边界成本
- [ ] `T6-4` 加入热更新机制
- [ ] `T6-5` 加入崩溃恢复、异常隔离与熔断策略
- [ ] `T6-6` 完成 10 万 / 50 万 / 百万级压测计划
- [ ] `T6-7` 输出《性能优化报告》与《上线手册》

### 可交付物
- 高性能优化版 runtime
- 热更新与稳定性方案
- 压测结论报告

### 验收标准
- 高峰压测下无明显队列雪崩
- GC、锁竞争、网络抖动具备可解释性
- 具备灰度上线条件

---

## 14.9 当前建议的实际开发顺序

建议我们按下面顺序直接开工：

1. [ ] 先完成 `T0-1 ~ T0-6`：把工程骨架搭起来
2. [ ] 再完成 `T1-1 ~ T1-10`：拿到纯 `C++` Actor MVP
3. [ ] 再完成 `T2-1 ~ T2-8`：跑通网络接入和 gate
4. [ ] 再完成 `T3-1 ~ T3-8`：接入 `V8`，让 JS 服务可运行
5. [ ] 最后推进 `T4/T5/T6`：做压测、集群和生产化

---

## 14.10 第一批立即开工任务（建议本周）

### Sprint-1
- [ ] 初始化 `CMake` 与目录结构
- [ ] 建立核心公共库：`actor` / `scheduler` / `socket` / `v8` / `metrics`
- [ ] 实现 `Message`、`ServiceContext`、`HandleRegistry`
- [ ] 实现基础消息队列与调度器
- [ ] 完成一个纯 `C++` echo actor demo

### Sprint-2
- [ ] 接入 timer wheel
- [ ] 跑通 TCP listen/accept/read/write
- [ ] 完成 gate MVP
- [ ] 输出第一版单机压测数据

### Sprint-3
- [ ] 接入 `V8`
- [ ] 完成 JS 服务宿主
- [ ] 暴露 `send/call/timeout/newService`
- [ ] 完成 JS echo/login demo

---

## 14.11 项目管理建议

开发过程中建议每个任务都补齐 4 个字段：

- **负责人**
- **状态**（todo / doing / done / blocked）
- **输出物**
- **验收结果**

建议后续再单独维护一份：
- `docs/development-roadmap.md`
- `docs/benchmark-baseline.md`
- `docs/api-design.md`

这样项目推进会更稳，也更方便多人协作。
