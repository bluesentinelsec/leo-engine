# Leo Engine Manager — Design Document (Draft)

## 1) Vision and Goals
Leo Engine Manager is a **code‑first game project manager** that makes it fast to bootstrap, run, and package Leo Engine games from the terminal. It should be:
- **Terminal‑native** and minimal: one install, a few commands, no GUI required.
- **CI/CD and git‑friendly**: deterministic output, easy automation, sensible defaults.
- **Project‑centric**: enforce/encourage a clean project structure, with templates to jump‑start games.

### Primary goals
- **Bootstrap** a new project quickly from templates.
- **Run** the game in development mode with easy overrides.
- **Package** games for distribution in develop and release modes.
- **Cross‑platform packaging** by downloading prebuilt engine binaries.
- **Provide sane defaults for common tasks** (e.g., save locations).

## 2) Non‑Goals (for now)
- Signing/notarization for macOS
- Advanced build pipelines (e.g., compiling the engine from source)
- Full editor functionality
- Multi‑engine support

## 3) User Experience (CLI)
Installation and usage should be obvious and terse:

```bash
pip install leo-engine

leo-engine --version

mkdir my_game
cd my_game

leo-engine new --template <template> --name <game_name>

leo-engine run

leo-engine package --build-type=develop
leo-engine package --build-type=release

leo-engine package --platform linux/amd64
```

### CLI conventions
- **Subcommands**: `new`, `run`, `package`, `doctor` (optional), `templates` (optional).
- **Flags** should be explicit and stable (good for CI).
- **Defaults** should make the happy path trivial.

## 4) Project Structure
Default template should yield a “sane” structure:
```
my_game/
  bin/
    develop/
    release/
  resources/
  README.md
  LICENSE
  third_party_licenses/
  .gitignore
```

### Notes
- `bin/` contains engine runtimes; separate **develop** and **release** builds.
- `resources/` contains scripts/assets or a packed archive depending on build mode.
- Root should be clean and git‑ready.

## 5) Templates
Templates are the primary way to bootstrap structure and content.

### Template system
- Default template provides a minimal, working game (scripts + assets).
- Additional templates may provide “starter” features (e.g., camera, input, UI).

### Template distribution
- Templates may be bundled with the manager or downloaded on demand.
- A template specifies:
  - project layout
  - initial assets/scripts
  - metadata (name, description, supported engine version)

## 6) Bootstrapping (`leo-engine new`)
When creating a project, the manager should:

1. **Download prebuilt engine binaries** for the current OS/CPU platform.
   - Always fetch **develop** and **release** variants.
   - **Develop** build includes debug symbols.
   - **Release** build is optimized, no symbols.
   - Binaries are placed under `bin/develop/` and `bin/release/`.
2. **Install template contents** (resources, default scripts, assets).
3. **Project housekeeping**:
   - Write `.gitignore` with defaults.
   - Add `README.md`, `LICENSE`, `third_party_licenses/`.
   - Initialize a git repo (optional or default?).

## 7) Running (`leo-engine run`)
By default, `run` should launch in developer mode.

### Default process
```
./bin/develop/<game> -r resources/ \
  --log-level debug \
  --window-mode windowed
```

### Overrides
- Allow CLI overrides for runtime options (e.g., fullscreen, window size, log level).
- Lua should still be able to override settings inside `leo.load()`.

## 8) Packaging (`leo-engine package`)
Packaging should be turnkey and focused on shipping MVP builds.

### Build types
- **develop**: internal/artist builds.
- **release**: optimized, customer‑facing builds.

### Cross‑platform support
- Must support packaging for `windows/amd64` and `linux/amd64` initially.
- macOS is supported later (bundled app structure, no signing/notarization).
- The manager downloads prebuilt engine binaries for target platforms.

## 9) Platform Packaging Details
### Windows
1. Create `release/<game_name>/`
2. Copy runtime to `release/<game_name>/game.exe`
3. Copy resources to `release/<game_name>/resource.bin` (or directory)
4. Copy misc files: README, LICENSE, third‑party licenses
5. Optional: add Windows metadata + app icon

### Linux
- Similar to Windows, with platform‑specific naming and permissions.

### macOS
- Create a valid `.app` bundle structure
- Place binaries and resources correctly
- No signing/notarization in MVP

## 10) Save Files & Common Game Tasks
The manager should offer clear, robust guidance for:
- Save file locations
- Config directory structure
- Log files

## 11) Distribution & Updates
- The manager should download engine binaries from a release source (e.g., GitHub)
- Versioning should be explicit and stable
- Allow pinning engine versions in project config

## 12) Configuration & Project Metadata
A project config file should define:
- game name
- template
- engine version
- build outputs
- package settings

Possible file name: `leo-engine.toml` (or similar)

## 13) Error Handling & UX
- Friendly CLI errors and actionable messages
- `leo-engine doctor` command to validate environment (optional)
- Clear messaging on missing templates or engine binaries

## 14) Open Questions
Please answer these inline; we’ll iterate section‑by‑section.

1) **Name**: Do you want the final CLI name to be `leo-engine`, or should it be shorter (e.g., `leo`)?

2) **Templates**: Do you want templates bundled in the pip package, or downloaded on demand?

3) **Git init**: Should `leo-engine new` always initialize git by default, or only with `--git`?

4) **Config file**: Do you prefer `leo-engine.toml`, `leo-engine.json`, or something else?

5) **Engine version pinning**: Should new projects auto‑pin the latest version, or leave it floating?

6) **Packaging output**: Should `release/` be placed in project root, or under `dist/`?

7) **Resources**: Do you want `resources/` copied as a directory for develop builds and packed into `resource.bin` for release, or allow both modes?

8) **Windows metadata**: Is metadata/icon insertion in‑scope for the first version, or a follow‑up?

9) **macOS**: Is a raw executable acceptable initially, or do you want a minimal `.app` bundle from day one?

10) **Save data conventions**: Should the manager enforce a standard save path, or just document it?
