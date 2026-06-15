# Synera Starter Architecture

## Core Idea

The project now has two different concepts that must stay separate:

- **Board**: the tactical battle board used inside combat.
- **Map**: the macro route map used between events and battles.

`core` is reserved for low-level tactical board primitives. Macro map logic belongs to `world/map`, and macro map images belong to `assets/maps`.

## Folder Responsibilities

```text
src/
|- main.cpp
|- app/
|- core/
|- combat/
|- world/
|- entity/
|- gui/
assets/
|- maps/
```

### `src/main.cpp`

Application entry point. It creates `QApplication`, creates `AppWindow`, shows it, and starts the Qt event loop.

It should not directly create map, battle, event, or save systems.

### `src/app`

Application-level control.

- `AppWindow`: owns the stacked pages and connects page signals to `GameManager`.
- `GameManager`: owns game flow, current `GameState`, node entering, battle/event completion, layer progression, save/load calls.
- `GameState`: serializable run state, including seed, HP, gold, current layer, current node, completed nodes, available nodes, and macro map graph.
- `SaveManager`: reads and writes JSON save files. It does not control pages or game rules.

### `src/core`

Low-level tactical board model.

- `Board`: tactical battle board data, cell zones, unit occupancy, position checks.
- `Hex`: tactical hex coordinate type.
- `default_board.json`: default tactical board layout used by combat.

`core` should not contain pages, macro map assets, application flow, or GUI scene code.

### `src/combat`

Combat systems and battle runtime.

- `BattleRepository`: loads `src/combat/battle.json` and selects a map battle encounter for the current macro node.
- `battle.json`: data-driven battle node pool. It defines `layer`, `elite`, `map_id`, and enemy placements.
- `BattleScene`: tactical battle scene UI. It builds the `QGraphicsScene`, manages preparation placement, and syncs visible units from battle state.
- `BattleSystem`: owns battle runtime state, tactical `Board`, battle copies, enemy construction, updates, death cleanup, and win/loss signals.
- `context/`: future battle context data.
- `buff/`: buff definitions, factories, and managers.
- `effect/`: combat effects such as damage, healing, control, and attribute changes.
- `equipment/`: equipment logic.
- `map_utility`: should probably be renamed to `board_utility`, because this project now uses `map` for macro maps.

Battle data is intentionally separate from event data. `events.json` owns event text and choices; `battle.json` owns map battle encounters.

`battle.json` layer rules:

- `1`, `2`, `3`: regular battle pools for macro layers 1 through 3.
- `10`, `20`, `30`: boss battle pools for macro layers 1 through 3.
- `elite: false`: normal battle node pool.
- `elite: true`: elite and boss battle node pools.

Enemy placement in `battle.json` uses tactical hex `position.x` and `position.y`; code computes `z = -x-y`. `map_id` names the tactical Board JSON under `src/core/boards` or runtime `boards`.

Enemy stat scaling is based on macro `MapNode::row`: the first battle row has no bonus, and each higher row adds 5% to enemy `maxhp`, `attack`, and `defense`. Scaling applies to constructed enemy instances only, never to character templates.

### `src/world/map`

Macro route map system.

- `MapNode`: macro node data and node type definitions.
- `MapLayer`: one independent layer with background path, fixed start/end nodes, and generated nodes.
- `MapGraph`: the full three-layer map and JSON serialization.
- `MapGenerator`: seed-based generation of three macro map layers.

Macro map backgrounds use:

```text
assets/maps/1.png
assets/maps/2.png
assets/maps/3.png
```

Runtime copies go to:

```text
build-codex/assets/maps/
```

### `src/entity`

Game entities and their gameplay components.

- `Unit`: base class for board units.
- `character/`: character model, factory, default skills, and character templates.
- `component/`: stats, attack, move, team, render, and buff components.
- `object/`: non-character object entities.
- `obstacle/`: obstacle entities.

Future inventory items probably belong under `entity/item` or a dedicated inventory folder.

### `src/gui`

All UI and rendering code.

- `pages/`: full-screen app pages.
- `widgets/`: reusable UI widgets such as `GameHud`.
- `board/`: tactical battle board graphics items.
- `HUD/`: current unit/object/obstacle HUD classes used by board items.

`gui` should not own game flow. Pages should display state and emit signals.

## Page Responsibilities

### `StartPage`

Initial page.

- New game with seed input.
- Load one of three save slots.
- Settings placeholder.

It emits requests; it does not create maps or battles.

### `MapPage`

Macro map page.

- Displays the current `MapLayer`.
- Draws background, nodes, and route lines.
- Enables only currently available nodes.
- Emits `nodeSelected(nodeId)`.

### `EventPage`

Event page placeholder.

- Displays a placeholder event.
- Can finish event.
- Can trigger a placeholder battle.

Needs a future `EventManager`.

### `BattlePage`

Battle page shell.

- Hosts `BattleScene`.
- Provides placeholder start/finish battle controls.
- Emits `battleFinished`.

It should eventually delegate real battle resolution to `BattleSystem`.

### `GameHud`

Shared page HUD.

- Displays HP and gold.
- Provides Bag, Shop, and Save buttons.

Bag and Shop are placeholders for now.

## Runtime Resource Layout

Source layout:

```text
src/core/default_board.json
assets/maps/1.png
assets/maps/2.png
assets/maps/3.png
```

Runtime layout after build:

```text
boards/default_board.json
assets/maps/1.png
assets/maps/2.png
assets/maps/3.png
```

The tactical board loader should read from `boards/default_board.json`.

The macro map page should read background images from `assets/maps/*.png`.

## Current Issues

1. `BattleScene` is still too large.

   It currently mixes scene rendering, preparation placement, battle copy creation, and phase switching.

2. `BattleSystem` is not connected yet.

   Real battle rules, target selection, skill execution, win/loss checks, and rewards still need to move there.

3. `EventPage` has no real event model.

   Add `world/event/Event` and `EventManager` before implementing real event content.

4. Bag and Shop are placeholders.

   Add inventory and shop state before building real panels.

5. `GameState` is still minimal.

   It needs owned characters, bag items, shop state, event flags, and run status.

6. `combat/map_utility` is semantically confusing.

   Rename it to `board_utility` if it is about tactical board math or terrain.

7. `gui/HUD` and `gui/widgets/GameHud` are easy to confuse.

   `gui/HUD` is unit-level HUD on the battle board. `GameHud` is the page-level HUD. Consider renaming `gui/HUD` to `gui/board/overlays`.

## Missing Systems

### Battle Data

Recommended additions:

```text
src/combat/battlecontext.h
src/combat/battleresult.h
src/combat/battlephase.h
```

These should describe one combat encounter, its units, phase, rewards, and result.

### Event System

Recommended additions:

```text
src/world/event/event.h
src/world/event/eventmanager.h/.cpp
src/world/event/eventresult.h
```

This should let event pages show data-driven content instead of hardcoded placeholder text.

Event data should not contain normal map battle pools. Special event battles should either request a battle through `BattleRepository` or reference future explicit battle ids.

### Player and Inventory State

Recommended additions:

```text
src/app/playerstate.h
src/entity/item/item.h
src/entity/item/bag.h/.cpp
src/world/shop/shopstate.h
```

This should move HP, gold, owned characters, items, and shop state into clearer substructures.

### Real Panels

Recommended additions:

```text
src/gui/widgets/bagpanel.h/.cpp
src/gui/widgets/shoppanel.h/.cpp
```

These should replace the current placeholder message boxes.

## Naming Rules

- Use **Board** for tactical combat terrain.
- Use **Map** only for macro route maps.
- Keep macro map assets in `assets/maps`.
- Keep tactical board JSON with `Board`, currently `src/core/default_board.json`.
- Keep page navigation in `app`, not in `gui/pages`.
- Keep battle rules in `combat`, not in `gui`.
