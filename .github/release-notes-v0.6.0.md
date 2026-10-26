# Geometry Wars: Retro Evolved 2 PC Port v0.6.0

v0.6.0 is a major visual, performance and quality-of-life update: native HDR output, Smooth glow, and an in-game Options menu you can open without leaving the game, alongside renderer optimizations and several gameplay and usability improvements. This is an unofficial fan-made port, not affiliated with or endorsed by the game's makers or publishers.

**No game files are included.** You need your own supported Geometry Wars: Retro Evolved 2 Xbox Live Arcade package.

## Highlights

- **Native HDR output (Windows)**: an FP16 scRGB swap chain lets the game's neon highlights go brighter than SDR white on a compatible HDR display, with paper white and peak brightness controls and further HDR tuning in the in-game Options menu. Requires Windows HDR to be on for the display; Direct3D 12 only.
- **Smooth glow (Windows)**: bloom and glow gradients fade out smoothly instead of ending in the original's flat, hard-edged steps. On by default; a Graphics setting restores the original behavior.
- **In-game Options menu**: **F1**, or **L3 + R3** on a controller, opens Display, Graphics, HDR, Gameplay, Controls and Cheats while playing. Most settings apply at once; the few that still need a restart show "Some changes apply after a restart" with a **Restart now** prompt.
- **Launcher self-update**: built-in updates, available since v0.5.0, can download and install newer releases while preserving game files, saves, settings and logs.
- **Renderer and performance improvements**: additional optimizations in the Direct3D 12 command and texture-cache paths reduce CPU/GPU overhead, especially in busy scenes and at higher render resolutions.

## HDR and Smooth glow

- HDR outputs the game's own light above white instead of clamping it to SDR white; paper white and peak brightness default to the display's own values and can be tuned, and the in-game Options menu's HDR page adds highlight strength, highlight saturation and an HDR debug view.
- Smooth glow keeps the glow's precision between the game's bloom passes, instead of the original's 8-bit intermediate texture, so faint glow around stars and lights no longer ends in a flat plateau.
- Both are Windows-only (Direct3D 12); the Linux Vulkan build does not register either setting. Turning HDR on also turns Smooth glow on; changing either restarts the game once.

## In-game Options

- Pages: Display, Graphics, HDR (Windows only), Gameplay, Controls and Cheats, reading and writing the same settings as the launcher.
- Opens with **F1** or **L3 + R3** (both sticks pressed on a controller); the same chord, **B** or **Escape** closes it. **LB**/**RB** or **Q**/**E** (or Tab) change page.
- Window size, Monitor, VSync, Aspect Ratio and Graphics card remain launcher-only, since they need a new window or process to take effect.
- The Options menu, the Cheats menu (**F10**) and the Achievements overlay (**F7**) are mutually exclusive: opening one is blocked while another is open.

## Performance

- Additional Direct3D 12 renderer optimizations reduce redundant GPU state changes and command-processor work, on top of the Direct3D 12 improvements already shipped in v0.5.0.
- A deeper audio queue keeps sound in step during busy modes like Evolved and Sequence, at a small added latency cost; this is not a general audio overhaul, just a fix for falling behind in heavy scenes.
- High-refresh visual interpolation has been hardened against edge cases found since v0.5.0.

## Gameplay and cheats

- **Unlock all game modes** (Gameplay page): a reversible setting that makes every mode selectable without changing your saved progression. Turning it Off restores your real unlock state exactly as it was (or as it has since been updated by further play), including after a profile reload while the setting is on. It does not permanently unlock anything on disk.
- **Add 100 Lives**: a new one-shot action, on the in-game Options menu's Cheats page and the **F10** cheat menu, that adds 100 lives to the local player using the game's own life system. It is not a saved setting.
- **Movement Speed** cheat (Off, 2x, 4x) and a **debug bomb** hotkey are also new since v0.5.0, alongside the existing Invincible, Infinite Shield and Infinite Bombs cheats.
- **Configurable local player names**: set the name shown in-game for up to 4 local players from the launcher's Gameplay page or the in-game Options menu; save data is not affected.

## Controls and usability

- Graphics card selection on Windows was simplified.
- Ctrl+S now saves launcher settings even while a text field, such as a player name, has keyboard focus.
- The launcher no longer warns about the optional SDL game controller database when it is simply absent.

## Windows

- Requires Windows 10 or 11 (64-bit) and a DirectX 12 graphics card.
- HDR additionally requires Windows HDR to be turned on for the display and a compatible HDR display; it is Windows-only.
- Download: **GeometryWarsRetroEvolved2-v0.6.0-windows-x64.zip**. Extract anywhere you can write to and run `geometry_wars_retro_evolved_2.exe`. Updating from v0.5.0 can be done from the launcher's self-update.

## Linux / Steam Deck

- Linux and Steam Deck remain **experimental**. HDR and Smooth glow are not available there; the in-game Options menu is still available on its other pages (Display, Graphics, Gameplay, Controls, Cheats).
- Download: **GeometryWarsRetroEvolved2Recompiled-v0.6.0-x86_64.AppImage**: native Linux x86_64 build using Vulkan (not Proton, not Wine). Make it executable and run it. See [docs/linux.md](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/blob/main/docs/linux.md).

## Required game version

- Your own package, this exact game version:

| | |
| --- | --- |
| Game | Geometry Wars: Retro Evolved 2, Xbox Live Arcade |
| Title ID | `584108FF` |
| Media ID | `191FF297` |
| Game version | `0.0.1.2` |
| `default.xex` SHA-256 | `6D5D9980A1D17DC74B9CCF540CEC16C2E144E7CEE3A74938E78C237B8ACEEB88` |

## Known limitations

- The high-refresh timing issue reported in [#5](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues/5) has not been confirmed fixed by its reporter; this release further hardens that path. Particles, trails and other effects are still not interpolated.
- If an update cannot be installed (for example in a folder you cannot write to), the previous version starts again; the reason is only in the updater's log.
- Keyboard remapping: to bind a key with Shift, Ctrl or Alt, hold the modifier before you click the control. Pressing Enter while choosing a key may also start the game.
- Linux and Steam Deck remain **experimental**. Some Linux systems render around 30 FPS ([#1](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues/1), [#6](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues/6)); the cause is not found yet. HDR and Smooth glow are not available on Linux.
- Crash reports in the log are written on Windows only.
- A few settings (Window size, Monitor, VSync, Aspect Ratio, Graphics card) remain launcher-only and are not in the in-game Options menu.
- Online Xbox Live services are not supported, and there is no online multiplayer. The profile, scores, leaderboards and achievements are local and offline.

## Credits

- Built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) and the [TRG launcher](https://github.com/TekRantGaming/trg-launcher).
- Special thanks to [steeben](https://github.com/steeben) for [PR #7](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/pull/7), which contributed the original HDR, Smooth glow and in-game Options work that was adapted to the current renderer and launcher architecture.
- The Windows Direct3D 12 asynchronous submission work shipped in v0.5.0 is adapted from [Unowa001/crackdown-recomp](https://github.com/Unowa001/crackdown-recomp) (BSD 3-Clause); its notice is in `licenses/THIRD-PARTY-LICENSES.txt`.

## Reporting problems

Please open an [issue](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues) and attach the newest `geometry_wars_retro_evolved_2_*.log` file from the `logs` folder next to the program (the launcher's **About > Open logs folder** opens it).

## Checksums

Each download has a matching `.sha256` file. Compare the SHA-256 of the file you downloaded with its `.sha256` file.

| File | SHA-256 |
| --- | --- |
| `GeometryWarsRetroEvolved2-v0.6.0-windows-x64.zip` | `f8715ac9ea2355dac23e1e2e318a2a468a02f3eb7a4ed5e3017d8e48a8306ac5` |
| `GeometryWarsRetroEvolved2Recompiled-v0.6.0-x86_64.AppImage` | `5d3082de5089a085e5f20fdfeb5b2dd8677645e6a247777ade5f9b33ec8fbd09` |

These are the checksums of the artifacts currently built in the local `dist/` folder. Re-verify them against the files actually attached to the published GitHub release before announcing it, in case the release build is repeated.

