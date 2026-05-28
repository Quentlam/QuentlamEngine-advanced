# QuentlamEngine MOD 开发指南

## 一、概述

QuentlamEngine 通过 Lua 5.4 提供 MOD 脚本支持。MOD 本质上是一个包含 `manifest.json` 清单文件和 `scripts/main.lua` 入口脚本的目录，放置在引擎的 `mods/` 文件夹下即可被自动发现和加载。

**优势**：
- 无运行时依赖（~15KB Lua VM vs .NET 运行时 10-15MB）
- 天然支持 Android NDK 交叉编译
- 脚本热重载友好，MOD 可在不重新编译引擎的情况下开发调试

## 二、目录结构

```
mods/
  my-mod/                  # MOD 根目录（目录名即 MOD ID）
    manifest.json          # MOD 元信息清单（必需）
    scripts/
      main.lua            # 入口脚本（必需）
      utils.lua           # 其他脚本（可选）
    assets/
      textures/           # MOD 资源
      data/               # MOD 数据
```

## 三、manifest.json

MOD 的元信息文件，引擎通过它发现 MOD 并解析依赖关系。

```json
{
  "id": "my-awesome-mod",
  "name": "My Awesome Mod",
  "author": "YourName",
  "version": "1.0.0",
  "description": "Adds a super cool feature",
  "entry": "scripts/main.lua",
  "dependencies": [],
  "content": []
}
```

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `id` | string | 是 | MOD 唯一标识符，建议使用 kebab-case |
| `name` | string | 是 | 显示名称 |
| `author` | string | 否 | 作者 |
| `version` | string | 是 | 版本号 |
| `description` | string | 否 | 描述 |
| `entry` | string | 否 | 入口脚本路径，相对于 MOD 根目录，默认 `scripts/main.lua` |
| `dependencies` | string[] | 否 | 依赖的其他 MOD ID |
| `content` | string[] | 否 | 资源路径列表（供内容管线使用） |

## 四、入口脚本 main.lua

### 4.1 生命周期函数

引擎会在特定时机调用 MOD 脚本中的四个生命周期函数：

```lua
-- mod:load()
-- MOD 首次加载时调用（引擎启动时）
-- 在此处初始化 MOD 状态、注册事件监听
function mod:load()
    QL_Log("My Awesome Mod loaded!")
    -- 初始化逻辑
end

-- mod:update(dt)
-- 每帧调用，dt 为距上一帧的秒数
-- 在此处处理需要每帧更新的逻辑
function mod:update(dt)
    if self.isActive then
        -- 每帧逻辑
    end
end

-- mod:onSave()
-- 存档时调用
-- 返回一个 Lua 表，其中包含需要持久化的 MOD 状态
function mod:onSave()
    return {
        isActive = self.isActive,
        counter = self.counter
    }
end

-- mod:onLoad(data)
-- 读档时调用，data 为 onSave 返回的 Lua 表
-- 在此处恢复 MOD 状态
function mod:onLoad(data)
    if data then
        self.isActive = data.isActive
        self.counter = data.counter or 0
    end
end
```

### 4.2 推荐写法

将生命周期函数封装在一个 `mod` 表中，并返回该表：

```lua
local mod = {
    id = "my-awesome-mod",
    name = "My Awesome Mod",
    isActive = true,
    counter = 0
}

function mod:load()
    QL_Log("My Awesome Mod loaded!")
    QL_Log("Engine version: " .. QL.VERSION)
    QL_Log("Engine name: " .. QL.ENGINE_NAME)
end

function mod:update(dt)
    self.counter = self.counter + dt
end

function mod:onSave()
    return {
        isActive = self.isActive,
        counter = self.counter
    }
end

function mod:onLoad(data)
    if data then
        self.isActive = data.isActive
        self.counter = data.counter or 0
    end
end

return mod
```

## 五、引擎 API

### 5.1 调试日志

```lua
QL_Log("info message")       -- 普通日志（Info 级别）
QL_LogWarn("warning message") -- 警告日志（Warn 级别）
QL_LogError("error message") -- 错误日志（Error 级别）
```

### 5.2 引擎常量

```lua
QL.VERSION      -- 引擎版本字符串，如 "1.0.0"
QL.ENGINE_NAME  -- 引擎名称，"QuentlamEngine"
```

### 5.3 扩展 API（计划中）

以下 API 将在后续版本中提供绑定：

```lua
-- 物品系统
Inventory.AddItem(itemId, count)
Inventory.RemoveItem(itemId, count)
Inventory.HasItem(itemId) -> bool

-- 游戏时钟
GameClock.GetDay() -> int
GameClock.GetSeason() -> string
GameClock.AdvanceDay()

-- 事件总线
EventBus.Subscribe(eventName, callback)
EventBus.Fire(eventName, data)
EventBus.Unsubscribe(eventName)

-- NPC 系统
Npc.GetNpc(npcId) -> NpcHandle
Npc.SetRelationship(npcId, value)
Npc.GetRelationship(npcId) -> int
```

## 六、开发流程

### 6.1 创建 MOD

1. 在 `Sandbox/mods/`（或游戏构建输出的 `mods/`）下创建 MOD 目录
2. 编写 `manifest.json`
3. 编写 `scripts/main.lua`
4. 启动引擎，观察控制台日志确认 MOD 被发现

### 6.2 调试

- **日志优先**：使用 `QL_Log` 输出关键变量和流程节点
- **增量测试**：先写最简 `load()` + `QL_Log`，确认引擎正常加载后再添加逻辑
- **版本控制**：将 MOD 目录纳入独立的 git 仓库管理

### 6.3 发布

1. 确认 `manifest.json` 的 `id` 和 `version` 填写正确
2. 移除调试用的 `QL_Log`
3. 将整个 MOD 目录打包分发给用户
4. 用户将 MOD 目录放入游戏的 `mods/` 文件夹

## 七、依赖管理

引擎通过 `manifest.json` 中的 `dependencies` 字段管理 MOD 加载顺序。

```json
{
  "id": "mod-b",
  "dependencies": ["mod-a"]
}
```

- MOD 依赖按拓扑排序确定加载顺序
- 循环依赖（`A 依赖 B，B 依赖 A`）会在加载时被检测并拒绝
- 缺失依赖的 MOD 会被记录错误但不会阻塞引擎启动

## 八、限制与注意事项

1. **不要在 MOD 中直接创建 C++ 对象**：Lua 只能通过引擎暴露的 API 操作世界
2. **存档兼容性**：修改 `onSave` 返回的表结构时，注意向后兼容（老存档可能没有新字段）
3. **性能**：每帧更新的逻辑应尽量轻量，复杂计算使用 `update(dt)` 累加或定时执行
4. **编码**：所有文件使用 UTF-8 编码
