# Synera: Synergy Auto-Arena

> C++17 · Qt 6 · 单机 PVE 自走棋 · 高程课程作业

---

## 目录

- [Part 1 —— PA 文档完成度说明](#part-1--pa-文档完成度说明)
- [Part 2 —— 项目亮点](#part-2--项目亮点)
  - [2.1 ECS 组件模式：高解耦的角色架构](#21-ecs-组件模式高解耦的角色架构)
  - [2.2 灵活的 Buff / Effect 系统](#22-灵活的-buff--effect-系统)
  - [2.3 仿杀戮尖塔的完整玩法闭环](#23-仿杀戮尖塔的完整玩法闭环)
- [Part 3 —— AI 使用说明](#part-3--ai-使用说明)
- [系统架构](#系统架构)
- [构建运行](#构建运行)

---

## Part 1 —— PA 文档完成度说明

对照 PA 说明文档中的各项需求，完成情况如下：

### 核心玩法（100% 完成）

| PA 需求 | 完成状态 | 说明 |
|---------|:-------:|------|
| M × N 六边形棋盘（8×8） | ✅ | `Board` + `Hex` 实现完整六边形坐标系统与邻居/距离计算 |
| Bench 备战区（8 格） | ✅ | 支持拖拽布阵、上阵上限校验 |
| Unit 属性：HP / ATK / Range / Max Mana / Mana | ✅ | `StatsComponent` 完整实现，另扩展了 Defense / Shield |
| 阵营区分 PlayerCtrl / EnemyCtrl | ✅ | `TeamComponent` + `teams` 枚举 |
| 羁绊系统（traits → 上阵阈值触发） | ✅ | `Bond` 枚举含战士/法师/动物/建筑/牧师/魔女/野兽/棋灵/刺客共 9 种 |
| Prep → Combat → Resolve 三阶段 | ✅ | MapPage → BattlePage（含备战）→ 自动战斗 → 结算 |
| 自动战斗（索敌/移动/攻击/技能） | ✅ | `AttackComponent` + `MoveComponent`，含 BFS 寻路、障碍绕行 |
| GUI 完整展示 | ✅ | Qt6 全套 GUI：地图页、战斗页、事件页、开始页、HUD、卡牌面板、装备面板 |

### 进阶功能（全部完成）

| 功能 | 状态 | 说明 |
|------|:----:|------|
| 角色合成升星（3 合 1） | ✅ | `mergeOwnedCharacterCards()`，同名同星合成 |
| 装备系统（穿戴 + 合成） | ✅ | 基础装备 + 高级装备配方合成 |
| 商店系统 | ✅ | 5 刷新位 + 购买 + 刷新 + 经验/等级 |
| 存档 / 读档 | ✅ | `SaveManager` + 多槽位 |
| 通关记录 | ✅ | `RecordManager` 记录胜负及阵容 |
| 角色回收（返还金币） | ✅ | 备战区角色回收 |

### 额外实现（超出 PA 范围）

| 功能 | 说明 |
|------|------|
| 🗺️ 路线地图 | 仿杀戮尖塔的节点地图，含普通战斗/精英/Boss/事件/休息五类节点 |
| 🃏 海克斯科技 | 每层开始可选海克斯卡，提供长期 Buff / 资源 / 路线修正 |
| 📜 事件系统 | JSON 驱动的事件链，含风险收益选择、嵌套战斗、抽奖等 |
| ⚔️ Buff / Effect 双轨系统 | 10 种触发时机 + 灵活组合，支持数据驱动 |
| 🛡️ 不可选取 / 临时阵营 | 控制类 Effect，实现魅惑、潜行等机制 |
| 💀 死亡保护 / 复活 | `deathProtection`, `deathRewind` 等 Effect |
| 🔥 燃烧 / 再生 / 属性窃取 | DOT/HOT tick 类效果 |
| 📊 战斗回放信息 | `BattleScene` 完整动画 + 状态同步 |

> **总体完成度：100% 完成 PA 全部需求，并在此基础上增加了大量仿杀戮尖塔的扩展玩法。**

---

## Part 2 —— 项目亮点

### 2.1 ECS 组件模式：高解耦的角色架构

传统做法通常把所有属性塞进 `Character` 类，导致类膨胀且难以扩展。本项目采用了 **ECS（Entity-Component）组件化设计**：

```
Character (Entity)
  ├── StatsComponent      —— 属性：HP/ATK/MP/Defense/Shield/Range
  ├── AttackComponent     —— 战斗：普攻冷却/技能列表/行动锁
  ├── MoveComponent       —— 移动：寻路/障碍绕行/路径缓存
  ├── TeamComponent       —— 阵营：玩家/敌方/临时转换
  ├── RenderComponent     —— 渲染：棋子图标/卡面/动画帧
  ├── BuffComponent       —— Buff：挂载/触发/到期移除
  └── Equipment*          —— 装备槽（最多 1 件）
```

**设计优势：**

- **单一职责**：每个 Component 只管理一个维度，互不干扰。改移动逻辑不需要看战斗代码。
- **可拔插**：新角色只需要组合不同 Component，不需要修改基类。`Obstacle`（障碍物）复用 `Unit` 体系，但禁用了不需要的组件。
- **可扩展**：添加新系统（如飞行、隐身）只需新增一个 Component 或在现有组件上加一个 lock 计数器，不影响已有逻辑。
- **复制安全**：每个 Component 都提供了拷贝构造函数（`Component(const Component&, Character*)`），合成升星时能正确复制所有状态。

### 2.2 灵活的 Buff / Effect 系统

这是整个战斗系统的心脏，设计核心是 **Buff 作为容器 + Effect 作为行为** 的双层解耦：

#### Buff 层（容器）

```cpp
class buff {
    QString name;
    vector<unique_ptr<effect>> effects;  // 一个 Buff 可包含多个 Effect
    float duration;           // 持续时间（-1 = 永久）
    float cooldown;           // 冷却时间
    BuffTrigger trigger;      // 触发时机
    int maxtrigger;           // 最大触发次数
    short priority;           // 优先级
};
```

**10 种触发时机**覆盖完整战斗生命周期：

| 触发时机 | 典型用途 |
|----------|---------|
| `OnBattleStart` | 战斗开始加属性、飞行、视野 |
| `BeforeAttack` | 攻击增伤、暴击 |
| `AfterAttack` | 攻击后附加伤害、女巫化 |
| `BeforeBeAttacked` | 伤害减免、闪避、反伤 |
| `AfterBeAttacked` | 受伤后加攻、位移、模仿 |
| `BeforeHeal` | 治疗加成 |
| `AfterSkill` | 技能后效果 |
| `BeforeAddBuff` | Buff 抗性/免疫 |
| `BeforeDeath` | 死亡保护、复活 |
| `OnTurnStart` | DOT 燃烧、HOT 再生 |

#### Effect 层（行为）

```cpp
class effect {
    // 每种触发时机对应一个 std::function 回调
    AttackHandler beforeAttackFunc;      // 可通过 setBeforeAttack() 注入
    BeAttackedHandler beforeBeAttackedFunc;
    TurnHandler onTurnStartFunc;
    // ... 共 10 种 handler
};
```

**关键设计巧思：**

1. **函数注入而非继承** —— Effect 不使用虚函数多态，而是通过 `std::function` 注入行为。`effectfactory` 从 JSON 读取配置后，按 effectId 装配对应的 handler。这意味着添加新效果只需在 factory 的 switch 里加一个 case，不需要新建子类。

2. **namespace 分组管理** —— 20+ 种 Effect 按类型分到 5 个 namespace：
   - `damage_effect` —— 女巫化、燃烧伤害
   - `heal_effect` —— 反转伤害为治疗、再生
   - `attribute_effect` —— 属性修改、伤害倍率、减伤、免疫、死亡保护
   - `control_effect` —— 临时阵营（魅惑）、不可选取（潜行）
   - `character_effect` —— 角色专属：Coco 位移、Noah 减伤再生、Levitation 飞行、死亡回溯等

3. **Context 传参模式** —— 每个触发时机有独立 Context 结构体（`AttackContext`, `BeAttackedContext`, `DeathContext` 等），Effect 可以修改其中的 `cancelled`、`damage` 等字段来拦截/修改后续行为。例如 `damageImmunity` 通过设置 `cancelled = true` 完全阻止伤害。

4. **JSON 数据驱动** —— Buff 列表 (`bufflist.json`) 和 Effect 列表 (`effectlist.json`) 完全由数据定义，策划可以直接修改 JSON 来调整效果参数，无需重新编译。

5. **BuffManager 优先级排序** —— 多个 Buff 按优先级排序触发，确保关键效果（如死亡保护）在其他效果之前执行。

### 2.3 仿杀戮尖塔的完整玩法闭环

在 PA 要求的基础自走棋之上，补充了类杀戮尖塔的完整 Roguelike 爬塔体验：

```
开始 → 选择海克斯 → 推进地图节点 → 战斗/事件/休息 → Boss 层 → 下一层 → 通关/失败
```

| 系统 | 实现细节 |
|------|---------|
| **路线地图** | `MapGraph` 生成分层节点图，玩家逐节点推进，选择路线 |
| **海克斯科技** | 每层开始 3 选 1，效果涵盖长期 Buff、资源奖励、路线/掉落修正 |
| **事件系统** | JSON 驱动的事件链，支持多选项、条件判定、嵌套战斗、抽奖（`LotteryScratch`）等 18 种 Action |
| **休息节点** | 休息回血 / 训练升星 / 挖宝得装备 / 锻造合成，4 种选择 |
| **Boss 战** | 三层 Boss，击败即通关 |
| **存档系统** | 手动 + 自动存档，多槽位 |
| **通关记录** | 记录胜负、阵容、用时 |

---

## Part 3 —— AI 使用说明

本项目在开发过程中使用了 AI（Claude Code）辅助，以下是各模块的 AI 参与程度说明：

### 3.1 Entity 部分 —— 主要自己完成，AI 修 Bug

| 模块 | 文件 | AI 参与度 | 说明 |
|------|------|:--------:|------|
| Unit 基类 | `entity/unit.h/cpp` | 🟢 0% | 完全自己写 |
| Character | `entity/character/character.h/cpp` | 🟢 0% | 自己设计 ECS 组合结构 |
| 各 Component | `entity/component/*.h/cpp` | 🟢 0% | Stats / Attack / Move / Team / Render / Buff / Sprite / Interact 等全部自己实现 |
| CharacterFactory | `entity/character/characterfactory.h/cpp` | 🟢 0% | JSON 加载 + 角色模板创建 |
| DefaultSkill | `entity/character/defaultskill.h/cpp` | 🟢 0% | 默认技能逻辑自己写 |
| Object / Obstacle | `entity/object/`, `entity/obstacle/` | 🟢 0% | 自己实现 |
| EquipSkill | `entity/equipskill/` | 🟢 0% | 装备技能自己实现 |

> AI 在此模块的主要作用是 **调试和修 Bug**，例如编译错误修复、逻辑边界检查等。

### 3.2 图片素材 —— 完全 AI 生成

| 资源类型 | 路径 | 生成方式 |
|----------|------|---------|
| 角色卡面 + 棋子图 | `assets/characters/*/card.png`, `pawn.png` | AI 图像生成 |
| 事件背景图 | `assets/events/` | AI 图像生成 |
| 地图背景 | `assets/maps/` | AI 图像生成 |
| 节点图标 | `assets/nodes/` | AI 图像生成 |
| 装备图标 | `assets/equipment/` | AI 图像生成 |
| 障碍物图 | `assets/obstacles/` | AI 图像生成 |
| UI 元素 | `assets/ui/`, `assets/pages/` | AI 图像生成 |

> 所有 `assets/` 下的图片素材 100% 由 AI 生成，未使用任何外部版权素材。

### 3.3 GUI 部分 —— 大部分 AI 生成

| 模块 | 文件 | AI 参与度 | 说明 |
|------|------|:--------:|------|
| StartPage | `gui/pages/startpage.h/cpp` | 🔴 90% | AI 生成，不太懂 Qt |
| MapPage | `gui/pages/mappage.h/cpp` | 🔴 90% | AI 生成 |
| BattlePage | `gui/pages/battlepage.h/cpp` | 🔴 90% | AI 生成 |
| EventPage | `gui/pages/eventpage.h/cpp` | 🔴 90% | AI 生成 |
| BattleScene | `gui/battle/battlescene.h/cpp` | 🔴 90% | 战斗棋盘 Qt 场景，AI 生成 |
| Board UI | `gui/board/hexlayout.h/cpp`, `unititem`, `griditem`, `benchslotitem`, `propitem` | 🔴 90% | 六边形布局 + 拖拽交互，AI 生成 |
| HUD / Widgets | `gui/HUD/`, `gui/widgets/` | 🔴 90% | CardPanel, EquipmentPanel, GameHUD 等 |

> 由于对 Qt 的 Graphics View / Widget 体系不太熟悉，GUI 层绝大部分代码由 AI 生成，自己主要负责接口定义和需求描述。

### 3.4 Combat 部分 —— AI 辅助，自己搭框架

| 模块 | 自己完成 | AI 完成 |
|------|---------|---------|
| **BattleSystem** | `.h` 头文件（接口设计 + 架构） | `.cpp` 实现文件 |
| **Effect 基类** | 设计 Effect 抽象基类 + 10 种 handler 函数指针注入模式 | — |
| **Effect 派生逻辑** | 搭建 namespace 分组 + factory 框架 | 各具体 Effect handler 实现（20+ 种） |
| **Buff** | 设计 Buff 类 + BuffManager 容器 + 10 种触发时机枚举 | — |
| **BattleRepository** | 接口设计 | `.cpp` JSON 加载实现 |
| **Board Utility** | 自己完成 | — |
| **Equipment** | 接口设计 | `.cpp` JSON 加载 |
| **Context** | `combatcontext.h` 全部 Context 结构体设计 | — |

> Combat 模块的核心架构（Effect 基类、Buff 容器、Context 传参、Factory 装配）全部自己设计。由于 Effect 种类特别多（20+ 种），具体的 handler 实现交给 AI 批量生成；BattleSystem 采用"自己写 `.h` 定义接口，AI 写 `.cpp` 实现细节"的协作模式。

### 3.5 其余部分 —— 基本由 AI 完成

| 模块 | AI 参与度 | 说明 |
|------|:--------:|------|
| App 层 (`gamemanager`, `gamestate`, `savemanager`, `recordmanager`, `eventrules`) | 🔴 ~95% | 游戏流程控制基本由 AI 完成 |
| World 层 (`mapgenerator`, `mapgraph`, `maplayer`, `eventrepository`, `hextechrepository`) | 🔴 ~95% | 地图生成、事件加载、海克斯系统由 AI 完成 |
| Core 层 (`board`, `hex`) | 🟡 ~50% | Board 的部分逻辑由 AI 辅助实现 |

### AI 协作总结

```
                    自己完成              AI 完成
Entity    ████████████████████░░  ~90%     ~10%（修bug）
Assets    ░░░░░░░░░░░░░░░░░░░░░░    0%    100%（AI 生成素材）
GUI       ░░░░░░░░░░░░░░░░░░░░░░  ~10%    ~90%（不熟悉 Qt）
Combat    ██████████░░░░░░░░░░░░  ~60%    ~40%（自己搭架构）
App/World ░░░░░░░░░░░░░░░░░░░░░░   ~5%    ~95%
Core      ██████████░░░░░░░░░░░░  ~50%    ~50%
```

---

## 系统架构

项目分为六个主要层次：

```text
src/app      游戏流程、存档、窗口切换、事件和战斗回流
src/world    路线地图、事件、海克斯科技、背包/商店状态
src/combat   战斗系统、Buff/Effect、羁绊、装备
src/core     Hex 坐标和战术 Board
src/entity   Unit、Character、Object、Obstacle 和 ECS 组件
src/gui      Qt 页面、战斗场景、HUD、卡牌面板、装备面板
```

主要类：

- **`GameManager`**：游戏流程中枢，管理地图推进、事件动作、商店、装备、战斗和存档。
- **`GameState`**：可序列化的全局状态，保存生命、金币、地图、角色卡、装备、商店、海克斯和当前事件。
- **`Board` / `Hex`**：战斗棋盘和六边形坐标，负责地块、占位、移动、邻居和距离。
- **`Character`**：ECS 组合式战斗单位，由 Stats / Attack / Move / Team / Render / Buff 六个 Component 组合。
- **`battlesystem`**：战斗规则系统，负责加载战斗、生成敌人、应用羁绊/装备/海克斯、推进战斗和结算。
- **`BattleScene`**：Qt 战斗棋盘控制器，负责备战区、拖拽布阵、棋盘图元、障碍物、宝箱和动画同步。
- **`buff` / `effect`**：双层战斗效果系统，Buff 决定触发时机和持续，Effect 决定具体行为。
- **`effectfactory`**：Effect 工厂，根据 JSON 配置和 effectId 装配 std::function handler。

---

## 资源结构

```text
assets/characters   角色卡面和棋子图
assets/events       事件背景图
assets/maps         路线地图背景
assets/nodes        地图节点图标
assets/equipment    装备图标
assets/obstacles    棋盘障碍物图
assets/pages        开始页背景
assets/ui           按钮、标题、HUD、宝箱等 UI 图
```

---

## 构建运行

### 环境要求

- **Qt 6**（`Core`、`Gui`、`Widgets`、`Svg`、`Network` 模块）
- **CMake** 3.16+
- **编译器**：支持 C++17（GCC 13+ / MSVC 2022+ / MinGW-w64）

### 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/mingw_64"
cmake --build build --config Release
```

### 打包发布

```bash
cd build
/path/to/Qt/6.x.x/mingw_64/bin/windeployqt Synera_Starter.exe --release --no-translations
# 将 build 目录下 exe、dll、各插件目录及 assets/ boards/ 等数据目录打包为 zip
```

---

## .gitignore 说明

以下内容未纳入版本控制：
- `build/`、`build-codex/`、`out/` —— 编译产物和打包输出
- `Synera_starter.zip` —— 发布包
- `过程文件/`、`过程素材/` —— 开发过程文档
- `.qtcreator/` —— IDE 配置
- `*.user`、`*.o`、`*.obj`、`*.exe`、`*.dll`、`*.pdb`、`*.ilk` —— 编译和调试文件
