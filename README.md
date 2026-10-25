<p align="center"><a title="Jeremy Kratz, Public domain, via Wikimedia Commons" href="https://commons.wikimedia.org/wiki/File:ISO_C%2B%2B_Logo.svg"><img width="256" alt="ISO C++ Logo" src="https://upload.wikimedia.org/wikipedia/commons/thumb/1/18/ISO_C%2B%2B_Logo.svg/256px-ISO_C%2B%2B_Logo.svg.png"></a></p>

<h1 align="center">Ready Or Not Internal</h1>
<p align="center">
  An internal trainer for Ready Or Not, built on the shared <a href="https://github.com/skript023/astra-ui">Astra UI</a> menu.
  Strictly for educational purposes.
</p>

## Features

Mission actions change what the host decides, so they work in single player or as the lobby host; as a client they show a notice instead.

| Tab | Features |
| --- | --- |
| Self | Godmode, No Stun, No Suppression, No Hit Effects, Bright Vision, Infinite Ammo, Add Magazine, Instant Kill, No Recoil, No Spread, Wall Penetration, Instant Lockpick, Walk Speed |
| Mission | Mission Tracker overlay, Target Inspector, Collectibles panel, Objective Alerts, No Penalty; Auto Win, Arrest All, Surrender or Knock Out Suspects, Report All, Collect All Evidence, Unlock All Doors, Open All Doors, Disarm All Traps, Remove Door Wedges, Teleport to Next AI |
| World | Slow Motion, Damage Multiplier, Freeze AI, AI Hold Fire, Objective and Evidence Markers, Infinite Squad Items, Reset Squad Stress |
| Online Players (in Self) | Player Alerts for joins, leaves, and downed teammates; a page per player with health per limb, status, held item and ammo, distance, ping, and platform; Teleport to Player, Spectate, Mute; as host also Heal & Revive, Refill Ammo, Godmode, and Kick |
| ESP | Suspects, civilians, SWAT, other players, traps, locked and wedged doors, evidence, reportables, and collectibles, with name, line, health bar, box, corner box, 3D box, and skeleton (AI only), up to a maximum distance; a Radar panel of the same things around you; Grenade Path, the arc a grenade in your hand would fly up to what it first hits |
| Aimbot | Aimbot, silent aim, triggerbot, FOV circle |
| Teleport | Teleport forward and saved locations by category |
| Settings | Astra's shared settings (layout, theme, banner, transparency, menu keys) plus Debug, Reload, Unload, Exit |

Every toggle is saved in `settings.json` in the `ReadyOrNot Mod` folder inside the game's install folder (for example `steamapps\common\Ready Or Not\ReadyOrNot Mod`), next to `menu_settings.json`, the saved teleports, and the log. The first start copies these files once from `%APPDATA%\ReadyOrNot`, where earlier builds kept them. The Astra loader reads the same folder, so it can switch features and set per-feature hotkeys while the game runs.

Objective Markers, Evidence Markers, and Infinite Squad Items use the game's own debug subsystem. Where a shipping build left a toggle's body out, the feature writes the flag the game reads instead; check each one in game. Freeze AI, AI Hold Fire, and Damage Multiplier work on the characters and their hits instead, since their debug switches did nothing in the shipping game.

## How it runs

- The menu draws on DX11 directly and on DX12, the game's default, through D3D11On12 on the game's direct queue, which `ExecuteCommandLists` captures.
- Hooks (swapchain Present and ResizeBuffers for DX11 and DX12, ExecuteCommandLists, ProcessEvent, SetCursorPos) use [Ellohim-Hook](https://github.com/skript023/Ellohim-Hook), the same hooking library as the Valheim trainer. A hook another program still covers on unload stays in place, and the DLL stays loaded rather than crash the game.
- Everything that reads or calls the engine runs on the game thread, in C++20 coroutine tasks (`task.hpp`, `task_mgr`) resumed once per frame from `ReceiveTick` (`Engine.ActorComponent.ReceiveTick` and the player controller's and player character's Blueprints), the same task system as the Valheim trainer. The menu and commands hand work to the game thread with `async_run`, which queues a task for the next tick. The render thread only draws.
- The ESP worker (`events/entity_event.cpp`) projects targets on the game thread and hands them to the renderer through a triple buffer.
- The mission worker (`events/mission_event.cpp`) reads the mission a few times a second for the tracker, the inspector, the collectibles list, and objective alerts.
- The player worker (`events/player_event.cpp`) reads the lobby a few times a second for the Online Players pages and Player Alerts. Mute and the player ID come from the game's own player list, which exists only once the game has shown it, such as in the pause menu.

## Building

You need Visual Studio 2022 with Desktop development with C++ (it ships CMake and Ninja), and Git on `PATH`.

1. Put the OpenSSL build in `openssl/` (`openssl/include` and `openssl/lib` with `libssl.lib`, `libcrypto.lib`, `zlib.lib`). The folder is ignored by Git.
2. Open the folder in Visual Studio and build the `x64-Release` configuration, or from a Developer Command Prompt:

```bat
cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build out/build/x64-Release --target ReadyOrNot
```

The output is `version.dll`. CMake fetches its dependencies on the first configure, including the SDK and Astra UI; pass `-DFETCHCONTENT_SOURCE_DIR_SDK=<path>` or `-DFETCHCONTENT_SOURCE_DIR_ASTRA=<path>` to build against a local checkout instead.

## Bugs

- The trainer window does not show while another overlay hooks the swapchain, such as MSI Afterburner.
