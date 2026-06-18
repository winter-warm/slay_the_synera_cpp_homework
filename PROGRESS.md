# Synera Starter — Progress Report

> Agent 可自主维护的进度报告。完成某个目标后，将其移到「已完成」并登记日期；新增目标追加到「待完成」。
>
> **最后更新**：2026-06-18

---

## 项目定位

Roguelike 卡牌构筑 + 自走棋 C++/Qt6 游戏，融合《杀戮尖塔》分支路线地图和自走棋六边形战术棋盘。

**技术栈**：C++17 / Qt 6 (Core+Widgets+Gui) / CMake 3.16+

---

## 架构速览

```
入口 main.cpp
  └── AppWindow (QStackedWidget)
        ├── StartPage ──信号──┐
        ├── MapPage   ──信号──┤
        ├── EventPage ──信号──┼──▶ GameManager (中央控制器)
        └── BattlePage ──信号─┘       ├── GameState    (可序列化运行状态)
                                        ├── SaveManager  (3槽 JSON 存档)
                                        ├── RecordManager (历史记录)
                                        └── BattleSystem  (战斗运行时)
```

两层地图命名严格区分：**Board**（六边形战术棋盘）≠ **Map**（宏观路线地图）。

---

## `src/app/` — 应用控制层

| 已完成 | 日期 |
|--------|------|
| AppWindow + QStackedWidget 四页面切换 | — |
| GameManager 中央流程控制（newGame/load/save/enterNode/finishBattle/finishEvent） | — |
| GameState 完整可序列化运行状态（seed/hp/gold/currentLayer/map/event/battle/shop/hextech） | — |
| SaveManager 三槽位 JSON 存读档 | — |
| RecordManager 游戏历史记录追加与读取 | — |
| EventRules 事件选项条件检查（金币/血量/卡牌持有等） | — |
| 商店经济：四槽刷新 / 稀有度概率 / 购买 / 经验升级 | — |
| 卡牌经济：持有 / 升星 / 三合一合成 / 初始卡希罗 1209 | — |
| 休息事件四种选项：Rest(+15HP) / Train(升星) / Dig(随机金币) / Forge | — |
| 战斗胜负处理：胜利推进 / 失败扣血 / 死亡游戏结束 / 通关记录 | — |
| 自动存档 + 耗时计时 | — |
| 三层 HexTech 选择（每层从 8-9 张中随机选 3 供玩家选择 1） | — |

| 待完成 |
|--------|
| [ ] Forge 发放随机装备 — `grantRandomEquipmentFromRest()` 当前弹窗占位 "装备背包将在后续实现" |
| [ ] HexTech `choose_basic_equipment` / `choose_supply` / `choose_advanced_equipment` 选择界面 |
| [ ] HexTech `map_modifier`（Golden Route 地图改造）逻辑 |
| [ ] GameState 分层（拆出 PlayerState / InventoryState 子结构） |

---

## `src/core/` — 棋盘底层

| 已完成 | 日期 |
|--------|------|
| Hex 六边形立方坐标（x+y+z=0），std::hash + qHash | — |
| Board 地形加载 / 区域划分（Player/Enemy/Neutral/Obstacle）/ 单位占位 / 通行检查 | — |
| default_board.json 默认棋盘，构建时复制到 boards/ | — |

| 待完成 |
|--------|
| [ ] 更多 `boards/*.json` 棋盘变体（当前仅 default_board） |

---

## `src/combat/` — 战斗系统

| 已完成 | 日期 |
|--------|------|
| BattleRepository：JSON 加载 battle.json，按层/精英/kind 筛选战斗配置 | — |
| BattleSystem：Board 加载 / 敌人生成 / 属性 row 缩放（每行+5%）/ 光环 Buff 应用 / 胜负检测 | — |
| BattleSystem：准备阶段角色放置确认、createRecord、onBattleStart、update 循环 | — |
| BattleSystem：死亡清理 removeDeadCharacters / finishIfTeamDead | — |
| BattleScene：QGraphicsScene 构建 / 准备阶段拖拽 / Bench ↔ Board / deploymentLimit | — |
| BattleScene：UnitItem 攻击动画（hitFlash / attackLunge / skillBurst） | — |
| BattleScene：宝箱生成 spawnRewardChest / 点击打开 / 金币弹窗 | — |
| Buff 系统：8 种触发器（OnBattleStart/BeforeAttack/AfterAttack/BeforeBeAttacked/AfterBeAttacked/BeforeHeal/BeforeAddBuff/BeforeDeath） | — |
| Buff 系统：duration / cooldown / maxtrigger / priority / BuffFactory / BuffManager | — |
| Effect 系统：5 种类型（damage/heal/attribute/control/character），effectlist.json 46 个效果 | — |
| bufflist.json：40 个 Buff 定义（角色技能 + HexTech + 通用） | — |
| board_utility：视线检查 lineOfSight / 路径搜索 findPath | — |
| battle.json：三层普通/精英/Boss 战斗配置（layer 1/2/3/10/20/30） | — |
| Equipment 类骨架：id/name/effects getter，EquipmentEffectType 枚举 | — |

**战斗配置统计**：

| 层级 | 普通 | 精英 | Boss |
|------|:----:|:----:|:----:|
| 第1层 | 2 | 1 | 1 |
| 第2层 | 2 | 1 | 1 |
| 第3层 | 2 | 1 | 1 |

| 待完成 |
|--------|
| [ ] **装备系统** — equipment.json 数据定义、装备池、随机发放、装备背包存储 |
| [ ] 自动战斗回合调度 — BattleSystem::update 只调 character->update，无 AI 决策/回合控制 |
| [ ] 拆分 BattleScene — ~500 行头文件混合场景渲染 + 流程控制 |
| [ ] 重命名 `map_utility` → `board_utility` |
| [ ] 每层增加战斗配置（当前每层仅 2-3 个，多轮重复感强） |

---

## `src/world/` — 世界系统

### Map 地图生成

| 已完成 | 日期 |
|--------|------|
| MapNode / MapLayer / MapGraph 完整数据结构 + JSON 序列化 | — |
| MapGenerator：种子确定性地图生成（三层独立 rng） | — |
| 每层 13-15 中间行 + 固定 Start/End 节点，共 15-17 行 | — |
| 五种节点类型：Rest / Event / NormalBattle / EliteBattle / Boss | — |
| 无 X 型交叉的逐行最近邻连线算法 | — |
| 每层独立背景 assets/maps/1.png / 2.png / 3.png | — |
| MapLayer::rebuildNextPointers 重建指针 | — |

| 待完成 |
|--------|
| [ ] HexTech map_modifier 实际改造地图（Golden Route 直线化 + 去精英） |

### Event 事件

| 已完成 | 日期 |
|--------|------|
| EventTypes 完整类型体系：17 种 EventActionType + 4 种 RestOption + BattleKind + BattleConfig | — |
| EventAction 字段完备：prompt/filter/templateId/minAmount/maxAmount/nextStepId/battle/onChooseActions | — |
| EventDefinition → EventOption → EventAction 三级数据结构 | — |
| EventRepository：JSON 加载 events.json，按 eventId + stepId 查询 | — |
| EventRequirement 条件结构 + EventRules 校验 | — |
| 多步骤事件流程：GoToStep / returnStepId after battle | — |
| chooseOwnedCard + selectionFilter + onChooseActions 二次选择流 | — |
| default 事件（fallback）+ 事件 100"林间祭坛"（完整多步骤含分支战斗） | — |

**事件覆盖率**：

| 层级 | eventId 范围 | 已有事件 | 差 |
|------|:-----------:|:------:|:--:|
| 第1层 | 15-20 | 1 (id=100) | 5-9 |
| 第2层 | 25-30 | 0 | 5-8 |
| 第3层 | 35-40 | 0 | 5-8 |
| default | — | 1 | 已够 |

| 待完成 |
|--------|
| [ ] **填充 events.json** — 每层需要至少 5 个事件，当前仅 2 个事件定义 |

### HexTech

| 已完成 | 日期 |
|--------|------|
| 25 张卡片定义（第1层 9 张 + 第2层 9 张 + 第3层 7 张） | — |
| HexTechRepository：按层查询 / choicesFor 随机 3 选 1 / findById | — |
| 长期光环 → activeAuraIds → BattleSystem::setActiveAuraIds 应用 | — |
| isLongTermHexTechEffect 判断（apply_buff / map_modifier / reward_modifier） | — |
| 即时效果 grant_gold 发放 | — |
| 三层独立叙事 intro text（启航/探索/决战） | — |

| 待完成 |
|--------|
| [ ] `choose_basic_equipment`（borrowed_blade）选择界面 |
| [ ] `choose_supply`（supply_cache）选择界面 |
| [ ] `choose_advanced_equipment`（chosen_armory）选择界面 |
| [ ] `map_modifier`（Golden Route）地图改造 |

### Bag / Aura

| 已完成 | 日期 |
|--------|------|
| OwnedCharacterCard / HexTechCardRecord / ShopOffer / ShopState 数据结构 | — |
| ActiveAura 结构 + BattleSystem 光环应用 | — |

| 待完成 |
|--------|
| [ ] 装备背包存储（EquipmentRecord） |

---

## `src/entity/` — 实体系统

| 已完成 | 日期 |
|--------|------|
| Unit 基类：id / name / position / virtual update | — |
| Character：6 组件（Stats/Attack/Move/Team/Render/Buff）/ 羁绊 / onBattleStart | — |
| CharacterFactory：template.json 加载 / create / playerTemplates / infoFor | — |
| 11 个玩家角色：希罗(初始) / 可可 / 橘雪莉 / 汉娜 / 蕾雅 / 亚里沙 / 安安 / 诺亚 / 奈叶香 / 玛格 / 梅露露★4 / 艾玛★4 | — |
| 7 个敌人角色：哥布林 / 国际象棋 6 件套（兵/车/马/象/后/王） | — |
| StatsComponent：HP/MP/attack/defense/shield/range，含临时属性叠加 | — |
| Object：宝箱等非角色实体 + ObjectItem 渲染 + 打开动画 | — |
| Obstacle：障碍实体 | — |

| 待完成 |
|--------|
| [ ] AttackComponent 自动攻击 AI（目标选择/攻击决策/技能释放）完整性确认 |

---

## `src/gui/` — 界面层

| 已完成 | 日期 |
|--------|------|
| StartPage：新建游戏（种子输入）/ 三槽读档 | — |
| MapPage：三层地图渲染 / 背景 / 节点图标按钮 / 路线连线 / 可用节点高亮 / 当前节点指示 | — |
| EventPage：普通事件（背景 + 文本 + 选项按钮 + 条件禁用）/ HexTech 选择 / 休息选择 / 锻炼选择 | — |
| EventPage：ownedCardSelection 二次选择界面 | — |
| BattlePage：承载 BattleScene / 开始战斗 / 结束战斗 | — |
| CardPanel：Shop 模式（购买/经验条/概率）+ Bag 模式（查看持有/拖拽合成动画）+ Training 模式（升星选择） | — |
| GameHud：HP / 金币 / Bag / Shop / Save 按钮，所有页面共享 | — |
| Board Items：UnitItem（拖拽/HUD/动画）/ GridItem（hover/drop 高亮）/ BenchSlotItem / ObjectItem（宝箱打开） | — |
| HUD：CharacterHUD / ObjectHUD / ObstacleHUD | — |
| 战斗单位动画：hitFlash / attackLunge / skillBurst / 宝箱 rewardBurst | — |

| 待完成 |
|--------|
| [ ] Settings 页面 — 当前 `QMessageBox::information(this, "Settings", "Settings placeholder.")` 占位 |
| [ ] 装备选择 UI — 配合事件和 HexTech 的装备发放 |
| [ ] 历史记录查看页面 — `history.png` 按钮已绘制但无对应页面 |
| [ ] 装备背包视图 — CardPanel 新增 Equipment 模式 |
| [ ] 重命名 `gui/HUD/` → `gui/board/overlays/` |

---

## `assets/` — 资源

| 已完成 | 日期 |
|--------|------|
| 地图背景：assets/maps/1.png / 2.png / 3.png | — |
| 节点图标：6 种（battle/boss/elite/event/rest/start + current_ring） | — |
| 角色资源：18 个角色 card.png + pawn.png | — |
| HexTech 卡片：25 张 cardImagePath 资源 | — |
| UI 元素：按钮（start_battle/newgame/exit/bag/shop/save）、HUD 图标（coin/hp）、标题、卡牌底板 | — |
| 战斗特效：battle_chest_closed.png / battle_chest_open.png | — |
| 休息事件：rest_campfire.png + 4 个行动图标（rest/train/dig/forge） | — |

| 待完成 |
|--------|
| [ ] 事件背景图补充（当前大部分事件无专属背景） |
| [ ] `assets/events/` 5 个文件 `git add` |
| [ ] `assets/ui/` 4 个文件 `git add`（battle_chest_closed/open + history + shop_exp_bar） |

---

## 总体进度：75%

```
████████████████████░░░░
```

---

## 待办清单

### P0 — 阻塞交付

| # | 目标 | 状态 |
|---|------|:----:|
| 1 | 装备系统：equipment.json 数据 + Equipment 类补全 + 装备背包 + 锻造 UI | ❌ |
| 2 | events.json 填充至每层 ≥5 个事件 | ❌ |

### P1 — 功能完整

| # | 目标 | 状态 |
|---|------|:----:|
| 3 | HexTech 装备选择 UI（borrowed_blade / supply_cache / chosen_armory） | ❌ |
| 4 | Settings 页面 | ❌ |
| 5 | Golden Route map_modifier 地图改造 | ❌ |
| 6 | 历史记录查看页面 | ❌ |

### P2 — 重构

| # | 目标 | 状态 |
|---|------|:----:|
| 7 | 拆分 BattleScene | ❌ |
| 8 | 重命名 `map_utility` → `board_utility` | ❌ |
| 9 | 重命名 `gui/HUD/` → `gui/board/overlays/` | ❌ |
| 10 | GameState 分层 | ❌ |
| 11 | 创建 `combat/battlecontext.h` / `battleresult.h` | ❌ |

### P3 — 内容丰富

| # | 目标 | 状态 |
|---|------|:----:|
| 12 | 每层增加战斗配置 | ❌ |
| 13 | 补充事件背景图 | ❌ |
| 14 | 更多棋盘变体 `boards/*.json` | ❌ |

---

## Agent 维护说明

**完成一个目标后**：
1. 将该目标从「待完成」移到「已完成」表格，填上日期
2. 将 P0-P3 清单中对应条目的 `❌` 改为 `✅`
3. 更新模块进度条和总体进度条
4. 更新顶部日期

**新增目标时**：
1. 追加到对应模块的「待完成」列表
2. 必要时在 P0-P3 清单中新增条目

**进度条参考**：
```
0%   ░░░░░░░░░░    50%  █████░░░░░    90%  █████████░
15%  ██░░░░░░░░    75%  ███████░░░    100% ██████████
85%  ████████░░
```
