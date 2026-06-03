# Synera Starter Goal

## Project Goal

Build a Qt/C++17 framework for a Slay-the-Spire-like flow game with a start page, layered seed maps, events, battles, a shared HUD, and save/load.

## Current State

The starter already has a battle board, preparation drag placement, bench slots, and temporary battle character copies. The new framework wraps that work in a full application flow instead of launching the board directly.

## Architecture Principles

- Pages display state and emit signals.
- `GameManager` owns flow control and `GameState`.
- `MapGenerator` owns seed-based layered map generation.
- `SaveManager` owns JSON save/load.
- `BattleSystem` will own battle rules as the combat implementation grows.
- `GameState` is the single source for HP, gold, current layer, current node, available nodes, and completed nodes.
- `core` is reserved for low-level tactical board primitives: `Board`, `Hex`, and `default_board.json`.
- Macro route maps live under `world/map`; visual map backgrounds live under `assets/maps`.

## Phase Plan

### Phase 1: Application Flow

- Add `AppWindow` with stacked pages.
- Add `StartPage`, `MapPage`, `EventPage`, and `BattlePage`.
- Route page signals through `GameManager`.

### Phase 2: Layered Map

- Add `MapNode`, `MapLayer`, `MapGraph`, and `MapGenerator`.
- Generate three independent layers from a seed.
- Give each layer its own background path: `assets/maps/1.png`, `assets/maps/2.png`, `assets/maps/3.png`.
- Keep fixed start/end nodes unchanged for every seed.

### Phase 3: Layer Routes

- Generate 12-15 nodes per layer.
- Support rest, event, normal battle, and elite battle nodes.
- Generate layer-internal routes with no X-shaped crossings.
- Unlock only reachable next nodes.

### Phase 4: Placeholder Events and Battles

- Use `EventPage` for rest/event placeholders.
- Use `BattlePage` for normal, elite, and end-node boss placeholders.
- Advance to the next layer after clearing the end node.

### Phase 5: Save and Load

- Save `GameState` as JSON.
- Support three slots: `save1.json`, `save2.json`, and `save3.json`.
- Restore layer, node availability, completed nodes, HP, gold, and the generated map.

### Phase 6: Shared Systems

- Add `GameHud` to map, event, and battle pages.
- Keep Bag and Shop as placeholder panels for now.
- Later connect real inventory, shop stock, rewards, and character growth.

## First Version Completion Standard

- The app starts at `StartPage`.
- New game enters layer 1 map.
- The same seed generates the same map.
- Each layer displays its own background or placeholder.
- Each layer has 12-15 nodes.
- Fixed nodes do not change across seeds.
- Routes have no X-shaped crossings.
- Event/rest nodes enter `EventPage`.
- Normal/elite/end nodes enter `BattlePage`.
- Clearing an end node advances to the next layer.
- Three JSON save slots can save and restore game progress.
