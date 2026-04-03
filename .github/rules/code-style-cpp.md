# crynet-server 项目规则 (v2.1 - 最终完备版)

## 📌 项目基础
- **项目名称**: crynet (全小写)
- **根目录**: `crynet-server`
- **核心标准**: C++20 (Coroutines, Concepts, Ranges), Skynet 启发

## 📂 目录结构规范
Agent 必须严格按照以下层级创建文件，禁止创建平铺在根目录的文件：
`<模块>/<子模块>/includes/` -> `.h / .hpp`
`<模块>/<子模块>/sources/`  -> `.cpp`

**包含路径：** `#include "模块/子模块/includes/文件名.h"`

## 🌐 双语注释标准
所有类、接口、公共函数必须包含：
/**
 * @cn
 * 中文描述 (简明扼要说明意图)
 *
 * @en
 * English description (Technical and precise)
 */

## 🛠️ 技术约束与性能准则
- **命名空间**: `namespace crynet::<模块> { ... }`
- **命名风格**: 
    - 类: `PascalCase` (接口加 `I` 前缀)
    - 变量/函数: `snake_case`
    - 成员变量: `m_` 前缀
    - 布尔值: `b` 前缀 (如 `bIsRunning`)
- **并发约束**:
    - **无锁优先**: 核心调度路径严禁 `std::mutex`。
    - **禁止阻塞**: 严禁调用任何会导致线程挂起的系统调用 (如 `sleep`, `wait`)。
    - **协程驱动**: 所有异步操作必须 Awaitable。
- **内存规范**: 
    - 禁用 `new/delete`。
    - 严格区分所有权 (unique_ptr) 与访问权 (raw pointer/reference)。
    - 核心消息路径需考虑内存对齐 (Cache-line alignment)。

## 🤖 Agent 行为准则
1. **原子化生成**: 每次生成功能必须成对提供 `.h` 和 `.cpp`。
2. **前置声明**: 只要能用前置声明解决的，严禁在 `.h` 中 `#include`。
3. **确定性**: 涉及模板时必须使用 `std::concepts` 进行编译期约束。