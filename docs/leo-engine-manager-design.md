# Leo Engine Manager — Design Document 

## 1) Vision and Goals
Leo Engine Manager is a code‑first game project manager that makes it fast and easy to bootstrap, run, and package Leo Engine games from the terminal. It should be:
- **Terminal‑native** and minimal: easily installed with Pip, as few as one command to put a window and sprite on the screen, and no GUI required.
- **CI/CD and git‑friendly**: deterministic output, easy automation, sensible defaults.
- **Project‑centric**: enforce/encourage a clean project structure, with templates to jump‑start games.

### Primary goals
- **Bootstrap** a new project quickly from templates.
- **Run** the game during development (windowed, debug log level), with CLI overrides for experimentation and verification.
- **Package** games for distribution.
- **Cross‑platform packaging** by downloading prebuilt engine binaries.

## 2) Non‑Goals (for now)
- Signing/notarization for macOS
- Full editor functionality (developers are expected to bring their own editor, though we have first class support for Tiled maps)

## 3) User Experience (CLI)
Installation and usage should be obvious and terse:

```bash
# this installs the leo-engine project manager
pip install leo-engine

# verify installation worked
leo-engine --version

# create project directory
mkdir my_game
cd my_game

# bootstrap game (we'll likely add more CLI arguments)
# treat all CLI args as mandatory during development
# we'll roll back as we develop more
leo-engine new --template <template> --name <game_name>

# run the game during active development
leo-engine run

# package game for distribution
leo-engine package --platform linux/amd64
```

### CLI conventions
- **Subcommands**: `new`, `run`, `package`.
- **Flags** should be explicit and stable (good for CI).
- **All errors fatal** treat all errors as fatal during development (fail fast and be transparent).
- **Defaults** should make the happy path trivial.

## 4) Project Structure
Default template should yield a “sane” structure:
```
my_game/
  bin/
  resources/
  README.md
  LICENSE
  third_party_licenses/
  .gitignore
```

## 5) Templates
Templates are the primary way to bootstrap structure and content.
- Default template provides a minimal, working game (scripts + assets).
- Additional templates may provide “starter” features (e.g., camera, input, UI).

## 6) Bootstrapping (`leo-engine new`)
When creating a project, the manager should:

1. **Download prebuilt leo-engine-runtime binary** for the current OS/CPU platform.
   - Runtime is placed under `bin`.
2. **Download template contents** as zip archive, unpack in current working directory, yielding folder called 'resources'. This folder contains the template's lua scripts and media files. 
3. **Project housekeeping**:
   - Write `.gitignore` with defaultas (TBD).
   - Add `README.md`, `LICENSE`, `third_party_licenses/`.
   - Initialize a git repo.

## 7) Running (`leo-engine run`)
The leo-engine manager will invoke the leo-engine runtime under bin/.
By default, the manager will invoke the runtime in windowed mode with debug console log level.
All leo-engine runtime CLI flags can be overriden from the manager, so for example, the caller can test the game in fullscreen mode, or specify the path to an alternate resources folder or archive.


## 8) Packaging (`leo-engine package`)
Packaging should be turnkey and focused on shipping MVP builds.

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
