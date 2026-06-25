# Synera Starter 架构说明

本文档是项目结构和类职责的约定。修改代码前应先阅读本文档；如果新增类、移动类、修改职责边界、改变资源位置或改变模块依赖方向，必须同步更新本文档。

## 总体分层

项目按职责分为六层：

- `app`：游戏流程编排、存档、记录、窗口页面切换，是 UI 与核心系统之间的协调层。
- `world`：宏观路线地图、事件、休息、海克斯科技、背包和商店等跑团/局外数据。
- `combat`：战斗规则、战斗配置、Buff/Effect、装备、AI/行动推进等纯战斗逻辑。
- `core`：战术棋盘 `Board`、六边形坐标 `Hex`，为战斗和 GUI 共用的基础模型。
- `entity`：棋盘单位模型，保留 `Unit` 的三种派生：`Character`、`Object`、`Obstacle`，以及纯数据/规则组件。
- `gui`：Qt 页面、控件、棋盘图元、HUD 和战斗场景控制器；允许依赖 `app/world/combat/core/entity`，但这些非 GUI 层不应反向依赖 GUI。

依赖方向应保持：

```text
gui -> app -> world/combat/core/entity
gui -> combat/core/entity
combat -> core/entity/world(event/equipment data types)
world -> core
entity -> core
core -> entity(仅 Board 持有 Unit/Obstacle 指针)
```

`entity` 和 `combat` 中不得 include `gui/*`。如果战斗逻辑需要触发动画，只能通过纯数据状态或事件队列，例如 `RenderComponent` 中的待播放视觉事件，由 GUI 消费。

## 命名与资源边界

- `Map` 指宏观路线地图，位于 `src/world/map`。
- `Board` 指战术战斗棋盘，位于 `src/core`。
- 战术棋盘相关工具使用 `board` 命名，不使用 `map` 命名。
- 宏观地图图片放在 `assets/maps`。
- 战术棋盘数据放在 `src/core/default_board.json` 和 `src/core/boards`，运行时复制到 `boards/`。
- 战斗节点池放在 `src/combat/battle.json`，运行时复制到 `battle/battle.json`。
- 事件文本和事件流程放在 `src/world/event/events.json`，不要把战斗节点池放回事件表。
- 障碍物图片放在 `assets/obstacles`。
- 战斗奖励宝箱等通用 UI 图片放在 `assets/ui`。

## app 层

- `AppWindow`：主窗口和页面容器。负责创建 `StartPage`、`MapPage`、`EventPage`、`BattlePage`、`CardPanel`、`EquipmentPanel`，连接页面信号与 `GameManager` 槽函数，并控制页面切换、浮层开关和全局提示。
- `GameManager`：整局游戏流程的中心控制器。负责新游戏、读档、存档、进入地图节点、执行事件动作、开始/结束战斗、发放奖励、商店刷新、角色卡合成、装备合成/穿戴、计时、自动存档和跑团记录。
- `GameState`：当前局的完整可序列化状态，包括种子、金币、生命、地图进度、当前事件、当前战斗、海克斯科技、角色卡、装备和商店状态。
- `CurrentEventState`：当前事件页面状态，记录当前事件步骤、文案、选项、选择流程和事件内临时选择。
- `EventResult`：事件执行结果，当前主要用于通知是否进入战斗。
- `BattleResult`：战斗结算结果，记录胜负。
- `RecordManager`：读写跑团历史记录。
- `RunRecord`：单次跑团记录，包含时间、结果等摘要信息。
- `SaveManager`：读写存档槽，负责 `GameState` 的持久化。

## core 层

- `Hex`：战术棋盘六边形坐标，使用 cube 坐标 `x/y/z`，要求 `x + y + z == 0`。
- `std::hash<Hex>`：使 `Hex` 可用于 `std::unordered_map` 和 `std::unordered_set`。
- `Board`：战术战斗棋盘模型。负责读取棋盘 JSON、维护格子区域、单位占位、移动/添加/移除单位、查询距离/邻居/通行性，以及持有棋盘生成的障碍物和受 Board 托管的单位。

## entity 层

### 基础单位

- `Unit`：所有战术棋盘实体的抽象基类。负责唯一 id、名称、棋盘坐标和 `update` 接口。
- `Character`：可战斗角色单位。组合属性、攻击、移动、队伍、渲染事件、Buff、装备等组件，负责战斗开始钩子、死亡判断、当前 Board 引用和目标查询。
- `Object`：可交互棋盘物体，例如战斗奖励宝箱。组合 `SpriteComponent` 和 `InteractComponent`，负责打开状态和交互后禁用。
- `Obstacle`：棋盘障碍物。组合 `SpriteComponent`，负责阻挡攻击语义、障碍图片编号和障碍显示数据。

### 组件

- `component`：角色组件基类，保存 `Character* owner` 和启用状态。仅用于角色规则组件，不承载 GUI。
- `StatsComponent`：角色数值组件。负责生命、最大生命、防御、蓝量、攻击、护盾、射程，以及受击、治疗和死亡更新。
- `AttackComponent`：角色攻击/技能组件。负责普攻冷却、攻击间隔、技能列表、行动锁和触发攻击/技能。
- `Skill`：技能执行函数包装。保存技能 handler 和是否为 buff 技能。
- `MoveComponent`：角色移动组件。负责目标选择、寻路策略、移动状态和行动锁。
- `TeamComponent`：角色阵营组件，保存 `pc/enemy`。
- `BuffComponent`：角色 Buff 入口组件，持有并更新 `BuffManager`。
- `RenderComponent`：纯数据视觉事件组件。保存角色可见性，以及待 GUI 消费的受击闪烁、攻击前冲、技能爆发事件队列；不得持有 HUD 或 Qt 图元。
- `SpriteComponent`：通用精灵数据组件。保存可见性、主/备用图片路径、贴图尺寸和偏移，供 `Object`、`Obstacle` 等非角色物体复用。
- `InteractComponent`：通用交互状态组件。保存启用/已使用状态，提供 `tryInteract()` 和 `reset()`。

### 角色与装备技能

- `characterfactory`：从角色模板数据创建 `Character`，负责模板 id 到角色实例的转换。
- `Bond`：角色羁绊类型。
- `SkillId`：默认技能枚举。
- `defaultskill` 相关函数：创建角色默认技能，技能有时机需求时应通过 Buff/Effect 表达。
- `equipskill` 相关函数：根据装备为角色添加装备技能或 Buff。

## combat 层

### 战斗流程

- `battlesystem`：战斗规则系统。负责加载 `BattleConfig`、加载 Board、生成敌人、按备战布阵开始战斗、推进战斗 update、移除死亡角色、判断胜负并发出战斗信号。
- `CharacterPlacement`：备战布阵结果，记录来源角色、目标 Hex、装备信息。
- `CharacterRecord`：战斗系统内部持有的角色实例和其阵营/位置记录。
- `BattleRepository`：读取 `src/combat/battle.json`，根据地图节点上下文选择战斗配置。
- `BattleNodeContext`：战斗节点选择上下文，包含层数、是否精英、行数、随机种子等信息。
- `board_utility` 函数：战术棋盘辅助算法，例如直线、阻挡检测等；不负责绘制。

### Buff / Effect / Context

- `buff`：Buff 实例。保存触发时机、持续时间、效果列表，并在对应 `BuffTrigger` 时应用效果。
- `BuffTrigger`：Buff 触发时机枚举，包括战斗开始、攻击前后、受击前后、死亡前、回合/时间 tick 等。
- `BuffManager`：角色 Buff 容器。负责添加、移除、更新 Buff，并按触发时机分发上下文。
- `bufffactory`：从 buff 配置创建 `buff` 实例。
- `effect`：效果基类，定义对战斗上下文施加影响的接口。
- `effectfactory`：从 effect 配置创建具体 `effect`。
- `BattleContext`：战斗开始相关上下文。
- `AttackContext`：攻击相关上下文。
- `BeAttackedContext`：受击相关上下文。
- `HealContext`：治疗相关上下文。
- `SkillContext`：技能释放相关上下文。
- `AddBuffContext`：添加 Buff 相关上下文。
- `DeathContext`：死亡相关上下文。
- `TurnContext`：时间/回合 tick 上下文。

### 装备

- `Equipment`：装备定义。保存装备 id、分组、名称、图标、属性修改、合成配方等。
- `EquipmentGroup`：装备分组，区分基础/高级等类别。
- `EquipmentStat`：装备影响的属性类型。
- `EquipmentModifier`：单条装备属性修改。
- `EquipmentRecipe`：装备合成配方。
- `EquipmentRepository`：读取基础/高级装备 JSON，提供装备查询和装备池。

## world 层

### 事件

- `EventRepository`：读取事件 JSON，根据 `EventContext` 返回当前事件步骤，并校验事件跳转。
- `BattleKind`：战斗类型，区分普通、精英、Boss 等。
- `EventActionType`：事件动作类型。
- `RestOption`：休息节点选项。
- `EnemyPlacement`：事件或战斗配置中的敌人模板与棋盘坐标。
- `BattleConfig`：一次战斗的配置，包括战斗类型、Board id、敌人、属性倍率和返回步骤。
- `LotteryPayout`：刮刮乐奖励项。
- `EventAction`：事件选项触发的动作。
- `EventRequirement`：事件选项显示/可用条件。
- `EventOption`：事件可选项。
- `EventDefinition`：事件步骤定义。
- `EventContext`：事件读取上下文。

### 地图

- `MapNode`：宏观路线地图节点，保存节点 id、层、行、类型、角色、位置和连边。
- `MapNodeType`：宏观节点类型，如战斗、事件、休息、Boss。
- `MapNodeRole`：宏观节点角色，如开始、普通、终点等。
- `std::hash<MapNodeType>`：使地图节点类型可用于哈希容器。
- `MapLayer`：单层宏观地图配置和节点集合。
- `LayerConfig`：地图层配置。
- `MapGraph`：完整宏观路线图。负责节点层集合、序列化/反序列化和节点查询。
- `MapGenerator`：根据种子和规则生成宏观路线地图。

### 海克斯科技、背包、商店

- `HexTechDefinition`：海克斯科技卡定义。
- `HexTechEffect`：海克斯科技效果定义。
- `HexTechRepository`：读取海克斯科技 JSON 并提供按 id/层查询。
- `ActiveAura`：当前激活的光环/全局效果记录。
- `HexTechCardRecord`：玩家已选择的海克斯科技卡记录。
- `OwnedCharacterCard`：玩家拥有的角色卡记录。
- `OwnedEquipment`：玩家拥有的装备实例记录。
- `ShopOffer`：商店单个商品记录。
- `ShopState`：商店等级、经验、刷新计数和商品列表。

## gui 层

### 页面

- `StartPage`：开始页面，提供新游戏、继续、历史、设置、退出等入口。
- `MapPage`：宏观路线地图页面。负责绘制地图背景、节点、连线、当前节点状态，并把点击节点请求发给上层。
- `EventPage`：事件页面。负责事件背景、文本、选项、海克斯科技选择、休息选项、角色选择等事件 UI。
- `BattlePage`：战斗页面。负责 `GameHud`、`QGraphicsView`、开始按钮、结算按钮、装备拖放、胜利宝箱奖励结算，并连接 `BattleScene` 与 `battlesystem`。

### 战斗棋盘 GUI

- `BattleScene`：战斗棋盘 UI/controller。负责创建和维护 `QGraphicsScene`、棋盘格、备战席、角色图元、障碍物图元、奖励宝箱、拖拽布阵、战斗动画和奖励动画。它属于 GUI 层，不属于 combat 规则层。
- `GridItem`：战术棋盘单个 hex 的图元。负责区域颜色、hover、高亮和可投放状态显示。
- `BenchSlotItem`：备战席槽位图元。负责 hover 和投放状态显示。
- `UnitItem`：角色单位图元。负责角色 pawn/HUD 绘制、血盾蓝条、拖拽信号，并消费角色可见性。
- `PropItem`：棋盘物体图元。用同一套逻辑展示 `Object` 和 `Obstacle`，负责读取 `SpriteComponent`、加载图片、fallback 绘制、宝箱点击打开动画。
- `HexLayout`：hex 坐标与 scene 坐标转换工具，负责多边形生成和点击位置反查。
- `CharacterHUD`：角色 pawn 图片和占位徽章绘制辅助类，仅由 GUI 使用。

### 通用控件

- `GameHud`：顶部/底部全局 HUD。显示金币、生命、时间和常用按钮，并发出保存、背包、商店、装备、返回开始页请求。
- `CardPanel`：角色卡面板。负责展示玩家角色卡、星级、合成/升级交互和详情。
- `EquipmentPanel`：装备面板。负责装备展示、详情、拖拽、合成和卸装入口。
- `EquipmentItemWidget`：装备面板内部装备格子控件，负责单个装备绘制、拖拽和合成投放。
- `EquipmentSlotPlaceholder`：装备面板内部占位格子控件，负责空位视觉。

## 关键协作流程

### 进入战斗

1. `GameManager` 根据宏观地图节点通过 `BattleRepository` 得到 `BattleConfig`。
2. `battlesystem::loadBattle()` 加载战术 `Board` 和敌人配置。
3. `AppWindow` 切到 `BattlePage`。
4. `BattlePage` 将 `battlesystem::mutableBoard()` 交给 `BattleScene`。
5. `BattleScene` 构建棋盘、备战席、角色、障碍物和可交互物体图元。
6. 玩家布阵后，`BattlePage::preparedPlacements()` 返回 `CharacterPlacement` 给 `GameManager`。
7. `GameManager::startPreparedBattle()` 调用 `battlesystem::startFromPreparedUnits()`。

### 战斗动画

1. combat/entity 层只修改纯数据，例如 `RenderComponent::requestHitFlash()`。
2. `battlesystem` 发出 `boardChanged` 或战斗状态变化信号。
3. `BattlePage` 调用 `BattleScene::syncFromBattleSystem()`。
4. `BattleScene` 在同步角色图元时消费 `RenderComponent` 的待播放事件，并创建 Qt 动画。

### 胜利奖励宝箱

1. `BattlePage::beginSettlement(true)` 请求 `BattleScene::spawnRewardChest()`。
2. `BattleScene` 在 Board 空格上添加 `Object`，并用 `PropItem` 显示宝箱。
3. 玩家点击宝箱后，`Object::interact()` 防止重复触发，`Object::open()` 切换打开状态。
4. `BattleScene` 播放宝箱爆开动画并通知 `BattlePage`。
5. `BattlePage` 计算金币/装备掉落，发信号给 `GameManager` 发放奖励。

## 修改守则

- 修改前先确认本文档描述的职责边界。
- 新增、移动、删除类时，必须更新“类职责”对应条目。
- 改变模块依赖方向时，必须更新“总体分层”和相关边界说明。
- 新增 JSON 数据表、资源目录或运行时复制规则时，必须更新“命名与资源边界”。
- 如果临时为了修 bug 违反分层，应在同一改动中加 TODO 或立即补回边界，不要让反向依赖沉淀。
