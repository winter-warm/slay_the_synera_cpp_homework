# Synera Starter Agent Rules

## Architecture Documentation

- Before changing code, first check `architecture.md` to confirm the current module boundaries and class responsibilities.
- If a change adds, removes, moves, or renames a class, changes a class responsibility, changes dependency direction, changes resource ownership, or changes runtime data flow, update `architecture.md` in the same task.
- Keep `architecture.md` in Chinese and treat it as the source of truth for project structure.

## Qt and STL Boundary

- Use Qt types freely in GUI code, Qt signal/slot boundaries, painting, graphics scenes, widgets, and Qt file/JSON integration.
- Prefer STL in pure data models, algorithms, game state, map generation, combat rules, and core board logic.
- Preferred non-GUI types include `std::string`, `std::vector`, `std::unordered_map`, `std::unordered_set`, and `std::optional`.
- Do not introduce Qt containers such as `QVector`, `QList`, `QHash`, `QMap`, or `QSet` in `world/map`, `core`, or combat rule code unless the type directly serves a Qt API boundary.
- Keep Qt conversions local at the boundary. For example, convert `std::string` to `QString` inside GUI or file-loading code instead of storing `QString` in game data.

## Naming Boundaries

- `Map` means the macro route map between events and battles.
- `Board` means the tactical combat board.
- Do not use `map` naming for tactical board utilities; prefer `board` naming when touching tactical terrain or hex cells.

## Resource Boundaries

- Macro route map visuals live under `assets/maps`.
- Tactical board data lives with the board model at `src/core/default_board.json` and is copied to `boards/default_board.json` at runtime.
- Do not place macro map images in `src/core`.
- Data-driven battle node pools live in `src/combat/battle.json` and are copied to `battle/battle.json` at runtime.
- Do not put map battle pools back into `src/world/event/events.json`; that file is for event text, options, and event flow.
- In battle data, `map_id` names the tactical Board JSON. Macro route Map and tactical Board naming must stay separate.

## Battle Node Data

- `src/combat/battle.json` describes map battle encounters by layer, elite flag, tactical board id, and enemy placements.
- Battle layers `1`, `2`, and `3` are regular layer pools. Layers `10`, `20`, and `30` are boss pools for macro layers 1, 2, and 3.
- Enemy placement uses tactical hex `position.x` and `position.y`; compute `z` as `-x-y`.
- Map battle selection must go through `BattleRepository`, not `EventRepository`.
- Enemy scaling uses the macro `MapNode::row`: first battle row has no bonus, and each higher row adds 5% to enemy `maxhp`, `attack`, and `defense`.

## Combat And Skills

- Implement skills with trigger timing through buffs/effects. If a skill needs to happen on battle start, before/after attack, before/after being attacked, before death, or on a timed tick, model it as a buff-backed skill instead of adding timing branches to active skill execution.
- Keep helper functions scarce. Prefer direct, local code unless a helper removes meaningful duplication or clarifies genuinely complex logic.

## Qt Graphics Scene Lifecycle

- Do not mutate or clear a `QGraphicsScene` synchronously from a click handler owned by a `QGraphicsProxyWidget` inside that same scene.
- If clicking an embedded scene widget changes pages, emits state that redraws the map, or calls `scene->clear()`, defer the action with `QTimer::singleShot(0, ...)` so Qt finishes the current widget event before the proxy widget is destroyed.
- Treat delayed exits or crashes after clicking map nodes as a likely scene/proxy lifetime issue before chasing unrelated game-state logic.

## Recompile
- If it's a minor change, there's no need to recompile and verify with CMake
