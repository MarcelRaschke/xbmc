# CLAUDE.md — Kodi Development Guide for AI Assistants

## Project Overview

Kodi (formerly XBMC) is an open-source media center application written primarily in C++20. It plays videos, music, pictures, games, and more. The project targets Linux, Windows, macOS, Android, iOS, tvOS, webOS, FreeBSD, and Raspberry Pi.

- **Current version:** 22.0 (Alpha 3)
- **License:** GPL-2.0-or-later
- **Language standard:** C++20 (do NOT use C++23 features)
- **Build system:** CMake (minimum 3.18; 3.30.6 on Windows)

## Repository Structure

```
├── addons/           # Built-in addons (skins, metadata scrapers, game controllers, etc.)
├── cmake/            # CMake modules and platform-specific build scripts
│   ├── modules/      # Find modules for external dependencies (FFmpeg, Curl, etc.)
│   └── scripts/      # Platform-specific build configuration (linux, windows, darwin, android)
├── docs/             # Build guides, code guidelines, contributing guide
├── lib/              # Third-party library sources
├── media/            # Splash screens and media assets
├── project/          # Platform-specific project files (Win32, Android)
├── system/           # Runtime system files (keymaps, shaders, players)
├── tools/            # Developer tools and scripts (codegenerator, depends, etc.)
├── userdata/         # Default user configuration files
└── xbmc/             # Main source code (see below)
```

### Source Code (`xbmc/`) Major Subsystems

| Directory | Purpose |
|-----------|---------|
| `application/` | Application lifecycle, startup, player callbacks |
| `addons/` | Addon system (loading, management, binary addon interfaces) |
| `cores/` | Media playback engines (VideoPlayer, RetroPlayer, AudioEngine, paplayer) |
| `dialogs/` | UI dialog windows |
| `filesystem/` | Virtual filesystem abstraction (local, network, zip, etc.) |
| `games/` | Game subsystem and controllers |
| `guilib/` | GUI framework (windows, controls, textures, fonts) |
| `input/` | Input handling (keyboard, mouse, touch, remote) |
| `interfaces/` | External interfaces (JSON-RPC, Python, web server) |
| `music/` | Music library, database, info scanning |
| `network/` | Network services (UPnP, Zeroconf, WebServer) |
| `peripherals/` | Peripheral device management |
| `pictures/` | Picture viewer and metadata |
| `platform/` | Platform abstractions (linux, win32, darwin, android, freebsd, webos) |
| `pvr/` | PVR/Live TV subsystem |
| `rendering/` | Rendering backends (OpenGL, GLES, DirectX) |
| `settings/` | Settings framework |
| `utils/` | Utility classes (strings, logging, XML, JSON, etc.) |
| `video/` | Video library, database, info scanning |
| `view/` | View modes and sorting |
| `weather/` | Weather service integration |
| `windowing/` | Window system backends (X11, Wayland, GBM, Win32, macOS, Android) |

### Key Architectural Patterns

- **`CServiceBroker`** (`xbmc/ServiceBroker.h`): Central service locator providing access to all major subsystems. Use `CServiceBroker::GetXxx()` to access managers.
- **`CServiceManager`** (`xbmc/ServiceManager.h`): Owns and initializes all services.
- **Addon System**: Binary addons use a C API bridge (`addons/kodi-dev-kit/`). Managed by `CAddonMgr`.
- **Player Cores**: VideoPlayer (main video/audio), RetroPlayer (game emulation), paplayer (audio-only), ExternalPlayer.
- **Namespaces**: Code is organized into namespaces like `KODI`, `PVR`, `MUSIC_INFO`, `ADDON`, `GAME`, etc.

## Building

### Linux (Quick Start)

```bash
mkdir $HOME/kodi-build && cd $HOME/kodi-build
cmake $HOME/xbmc -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -- VERBOSE=1 -j$(getconf _NPROCESSORS_ONLN)
```

### Key CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_TESTING` | ON | Build test suite |
| `ENABLE_INTERNAL_GTEST` | OFF | Use internal Google Test |
| `CMAKE_BUILD_TYPE` | — | Debug, Release, RelWithDebInfo |
| `KODI_MIRROR` | `https://mirrors.kodi.tv` | Mirror for dependency downloads |
| `VERBOSE` | OFF | Show detailed build output |

Out-of-source builds are required. Do NOT build in the source directory.

### Running Tests

```bash
cd $HOME/kodi-build
cmake --build . --target kodi-test
./kodi-test
```

Tests use Google Test. Test files follow the pattern `Test*.cpp` and are located in `test/` subdirectories alongside the code they test (e.g., `xbmc/test/`, `xbmc/utils/test/`, `xbmc/network/test/`).

### Platform-Specific Build Guides

See `docs/README.<Platform>.md` for detailed instructions:
- `docs/README.Linux.md`, `docs/README.Ubuntu.md`, `docs/README.Fedora.md`, `docs/README.openSUSE.md`
- `docs/README.Windows.md`
- `docs/README.macOS.md`, `docs/README.iOS.md`, `docs/README.tvOS.md`
- `docs/README.Android.md`
- `docs/README.FreeBSD.md`, `docs/README.webOS.md`

## Code Style and Conventions

### Formatting (enforced by `.clang-format`)

- **Indent:** 2 spaces (no tabs)
- **Line length:** 100 columns max
- **Braces:** Allman style (opening brace on new line)
- **Pointer/reference alignment:** Left (`char* a`, not `char *a`)
- **Namespace indentation:** None
- **Include order:** Own header first, then Kodi platform-independent, Kodi platform-specific, C/C++ system headers, external libraries (sorted alphabetically within groups)
- **Include guards:** `#pragma once`

Run `clang-format` (version 9.0+) on new files. CI will provide format patches for PRs.

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | `C` prefix + PascalCase | `CFileItem`, `CVideoDatabase` |
| Interfaces | `I` prefix + PascalCase | `ILogger`, `IPlayer` |
| Structs | PascalCase (no prefix) | `InfoChar` |
| Methods | PascalCase | `GetCurrentPlaylist()` |
| Member variables | `m_` prefix + camelCase | `m_currentSong`, `m_bActive` |
| Static members | `ms_` prefix + camelCase | `ms_instance` |
| Global variables | `g_` prefix + camelCase | `g_application` |
| Local variables | camelCase | `itemCount` |
| Constants | UPPER_SNAKE_CASE | `MY_CONSTANT` |
| Enums | PascalCase name, UPPER_SNAKE_CASE values | `enum class Dummy { VALUE_X }` |
| Namespaces | UPPER_SNAKE_CASE | `MUSIC_INFO`, `PVR` |
| Files | PascalCase matching class (without prefix) | `FileItem.h`, `FileItem.cpp` |

### C++ Best Practices (from `docs/CODE_GUIDELINES.md`)

- Use `nullptr` instead of `NULL`
- Use `{}` brace initialization for fundamental types: `int x{3};`
- Declare variables close to first use
- Use range-based `for` loops where reasonable
- Use prefix increment (`++i`) in traditional loops
- Mark overriding virtual functions with `override` (no `virtual` keyword)
- Use default member initialization: `bool m_fooBar{false};`
- Prefer `std::string_view` over `std::string` for function parameters and constants
- Use C++ style casts (`static_cast<>`, `dynamic_cast<>`), not C-style casts
- Use C++ wrappers for C headers (`<cstring>` not `<string.h>`)
- Prefer `constexpr` over `const` for compile-time constants
- Use smart pointers (`std::shared_ptr`, `std::unique_ptr`) for memory management
- Data members should be `private`; use accessor methods

### Logging

Use `CLog::Log()` with fmt library formatting:
```cpp
CLog::Log(LOGDEBUG, "Player: started playback of {}", filename);
```
Levels: `LOGDEBUG`, `LOGINFO`, `LOGWARNING`, `LOGERROR`, `LOGFATAL`

Do NOT use `printf`, `std::cout`, or old-style `%s` formatting.

### Documentation

Use Doxygen comments (Qt style) for new classes and functions:
```cpp
/*!
 * \brief Short description.
 * \param name Parameter description.
 * \return Return value description.
 */
```

### License Headers

All source files require SPDX headers:
```cpp
/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
```

## Commit and PR Conventions

### Commit Messages

Format: `[Component(s)] Short description` or `Component(s): Short description`

Examples:
- `[Python][xbmcgui] Fix x problem`
- `[VideoPlayer] Add support for new codec`
- `SmartPlaylist: use standard algorithms`

Each commit should be a meaningful, logical unit. Squash WIP commits before submitting.

### Pull Request Guidelines

- Create topic branches; never work on `master` directly
- Code against the `master` branch
- One topic branch per change
- Separate cosmetic/formatting changes from functional changes (unless clang-format requires it)
- Jenkins CI must be green on all platforms
- Apply clang-format patches requested by Jenkins
- At least one team member approval required before merge

## CI / GitHub Workflows

- `.github/workflows/` contains CI workflows (documentation, Sonarqube, translations)
- Jenkins (external) handles the main CI build and test pipeline across all platforms
- `.github/CODEOWNERS` defines per-component review assignments

### Key Code Owners

| Area | Maintainer |
|------|-----------|
| PVR/Live TV | @ksooo |
| RetroPlayer/Games/Input/Peripherals | @garbear |
| Music | @DaveTBlake |
| Linux platform/GBM/EGL | @lrusak |
| Wayland | @yol |

## Clang-Tidy Checks (`.clang-tidy`)

The project enforces these automated checks:
- `modernize-make-shared`, `modernize-make-unique`, `modernize-use-default-member-init`, `modernize-use-emplace`
- `performance-*` (string find, range copy, unnecessary copies, move semantics, vector operations)
- `readability-container-contains`, `readability-container-data-pointer`, `readability-container-size-empty`

## Quick Reference

- **Main entry point:** `xbmc/application/Application.cpp`
- **Service locator:** `xbmc/ServiceBroker.h`
- **Code guidelines:** `docs/CODE_GUIDELINES.md`
- **Contributing guide:** `docs/CONTRIBUTING.md`
- **Git workflow:** `docs/GIT-FU.md`
- **Build guides:** `docs/README.md` (index with links to all platforms)
