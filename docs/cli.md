# Command-Line Interface (CLI)

This document describes the Leo Engine command-line options and their behavior.

## Usage

```
./leo-engine [options]
```

## Defaults

If no options are provided:
- Resource mount uses the default VFS order (prefers `.` when `resources/` exists, then `resources.zip`, then `resources/`).
- Script path defaults to `resources/scripts/game.lua`.
- Window mode defaults to `borderless-fullscreen`.
- Logical resolution defaults to `1280x720`.

## Options

### Version and paths

- `--version`  
  Print the engine version and exit.

- `-r, --resources, --resource <path>`  
  Set the resource root to mount.  
  - If `<path>` is a directory, the parent directory is mounted and the directory name becomes the VFS prefix.  
  - If `<path>` is a file (e.g. `resources.zip`), the archive is mounted and the basename (without extension) becomes the VFS prefix.

- `-s, --script <vfs-path>`  
  Lua entry script path inside the VFS.  
  Default (if `--resources` is set): `<prefix>/scripts/game.lua`  
  Default (if not set): `resources/scripts/game.lua`

### App identity

- `--organization, --org <name>`  
  Organization name used by PhysFS for the write directory.  
  Default: `bluesentinelsec`

- `--app-name <name>`  
  App name used by PhysFS for the write directory.  
  Default: `leo-engine`

### Window and rendering

- `--title <text>`  
  Window title.  
  Default: `Leo Engine`

- `--window-width <pixels>`  
  Window width in pixels.  
  Default: `1280`

- `--window-height <pixels>`  
  Window height in pixels.  
  Default: `720`

- `--logical-width <pixels>`  
  Logical render width.  
  Default: `1280`

- `--logical-height <pixels>`  
  Logical render height.  
  Default: `720`

- `--window-mode <mode>`  
  Window mode (case-insensitive).  
  Allowed values: `windowed`, `fullscreen`, `borderless-fullscreen`  
  Default: `borderless-fullscreen`

### Timing and simulation

- `--tick-hz <rate>`  
  Fixed update rate (ticks per second). Must be > 0.  
  Default: `60`

- `--frame-ticks, --num-frame-ticks <count>`  
  Number of frame ticks to run before exiting.  
  `0` means run until quit.  
  Default: `0`

## Examples

Run with the default resources directory and script:
```
./leo-engine
```

Mount a zip and run a script from it:
```
./leo-engine --resources resources.zip --script resources/scripts/game.lua
```

Windowed mode with explicit size and tick rate:
```
./leo-engine --window-mode windowed --window-width 1024 --window-height 576 --tick-hz 120
```
