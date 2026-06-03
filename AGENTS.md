# Synera Starter Agent Rules

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

## Recompile
- If it's a minor change, there's no need to recompile and verify with CMake