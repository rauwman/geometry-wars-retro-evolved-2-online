# Geometry Wars: Retro Evolved 2 PC Port v0.6.1

v0.6.1 is a packaging and compatibility update. It reduces false-positive antivirus detections of the Windows update helper, `trg-updater.exe`, and changes nothing in the game itself. This is an unofficial fan-made port, not affiliated with or endorsed by the game's makers or publishers.

**No game files are included.** You need your own supported Geometry Wars: Retro Evolved 2 Xbox Live Arcade package.

For everything new in v0.6.0 (HDR output, Smooth glow, the in-game Options menu and more), see the [v0.6.0 release notes](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.6.0).

## What changed

- **Windows update helper**: some antivirus engines reported heuristic (generic) detections for the v0.6.0 `trg-updater.exe`. The helper is now built from the same updater source with MSYS2 CLANG64 (clang and LLD), as a self-contained program that uses only Windows system libraries. This is the same model used by the project's other ports.
- The helper's debug symbols are kept outside the release ZIP. The shipped helper contains no debug information.
- The helper now writes a more detailed log of each update step, which helps diagnose an update that fails.
- **Game executable**: still built with its normal Visual Studio clang Release configuration. On Windows, the ReXGlue runtime and GPU plugin are the same files as in v0.6.0.
- **No gameplay changes**: gameplay, saves, settings, achievements, cheats, the launcher and the handling of game data are the same as in v0.6.0.
- **Linux**: the AppImage is rebuilt with the new version number and works the same as v0.6.0.

## Antivirus checks

Both downloads were checked with VirusTotal before publication and had **0 detections** at release preparation time:

- Windows ZIP: [VirusTotal report](https://www.virustotal.com/gui/file/7a3862ab73e5f5d9536d58c5eb9c15cad89a492d9266d6ae399089140d1b5b08)
- Linux AppImage: [VirusTotal report](https://www.virustotal.com/gui/file/15f9b5160b778bd9cd67c1ef94e95fd6831e276459b185155144f38f6d0cc7ed)

VirusTotal results can change as antivirus engines update, and a clean scan is not a security guarantee. The change in this release reduces false-positive heuristic detections through conventional compiler and packaging practices.

## Updating from v0.6.0

- **From the launcher (Windows and Linux)**: the launcher checks the project's GitHub releases once per start. When it finds v0.6.1, a banner appears in the header. Choose **UPDATE NOW**, then **RESTART TO UPDATE**.
  - The download is verified with its SHA-256 before anything is replaced.
  - Only the port's own program files are replaced. The `game` folder, `user_data`, saves, settings, logs and caches are kept.
  - On Windows, the update is installed by the `trg-updater.exe` already in your v0.6.0 folder, which also installs the new helper. The Linux AppImage replaces its own file.
- **By hand (Windows)**: close the game, then extract the ZIP's contents into your existing folder and replace the files when asked. The program files sit at the top of the ZIP. Your `game` and `user_data` folders are kept.
- **By hand (Linux)**: put the new AppImage in the same folder as the old one, so it finds the same `game` and `user_data` folders next to it.

## Windows

- Requires Windows 10 or 11 (64-bit), a DirectX 12 graphics card and the latest Microsoft Visual C++ 2015-2022 Redistributable (x64). HDR additionally needs Windows HDR turned on and a compatible display.
- Download: **GeometryWarsRetroEvolved2-v0.6.1-windows-x64.zip**. Extract it anywhere you can write to and run `geometry_wars_retro_evolved_2.exe`.
- The build is not digitally signed, so SmartScreen may warn: choose **More info**, then **Run anyway**.

## Linux / Steam Deck

- Linux and Steam Deck remain **experimental**.
- Download: **GeometryWarsRetroEvolved2Recompiled-v0.6.1-x86_64.AppImage**, a native Linux x86_64 build using Vulkan (not Proton, not Wine). Make it executable and run it. See [docs/linux.md](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/blob/main/docs/linux.md).
- Built on Ubuntu 20.04: systems with glibc 2.31 or newer are supported.

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

- The high-refresh timing issue reported in [#5](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues/5) has not been confirmed fixed by its reporter. Particles, trails and other effects are still not interpolated.
- If an update cannot be installed (for example, in a folder you cannot write to), the previous version starts again. The reason is only in the updater's log.
- Keyboard remapping: to bind a key with Shift, Ctrl or Alt, hold the modifier before you click the control. Pressing Enter while choosing a key may also start the game.
- On some Linux systems the game renders at around 30 FPS ([#1](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues/1), [#6](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues/6)); the cause has not been found yet. HDR and Smooth glow are not available on Linux.
- Crash reports in the log are written on Windows only.
- A few settings (Window size, Monitor, VSync, Aspect Ratio, Graphics card) are only in the launcher, not in the in-game Options menu.
- Online Xbox Live services are not supported, and there is no online multiplayer. The profile, scores, leaderboards and achievements are local and offline.

## Validation

- Updater tests: 175 checks, 0 failures. Update helper test against the shipped `trg-updater.exe`: 76 checks, 0 failures.
- Release folder check: 21 cases, 0 failures. The release ZIP passed the packager's own check.
- Graphics card tests: 28 checks, 0 failures. Aspect ratio tests: 695 checks, 0 failures.
- The real v0.6.0 updater installed the v0.6.1 ZIP successfully and kept user data, game data and logs.
- The Linux AppImage passed its release checks. The highest glibc symbol version it needs is GLIBC_2.30.

## Credits

- Built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) and the [TRG launcher](https://github.com/TekRantGaming/trg-launcher).
- Special thanks to [steeben](https://github.com/steeben) for [PR #7](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/pull/7), which contributed the original HDR, Smooth glow and in-game Options work shipped in v0.6.0.
- The Windows Direct3D 12 asynchronous submission work is adapted from [Unowa001/crackdown-recomp](https://github.com/Unowa001/crackdown-recomp) (BSD 3-Clause); its notice is in `licenses/THIRD-PARTY-LICENSES.txt`.

## Reporting problems

Please open an [issue](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/issues) and attach the newest `geometry_wars_retro_evolved_2_*.log` file from the `logs` folder next to the program (the launcher's **About > Open logs folder** opens it).

## Checksums

Each download has a matching `.sha256` file. Compare the SHA-256 of the file you downloaded with its `.sha256` file.

| File | SHA-256 |
| --- | --- |
| `GeometryWarsRetroEvolved2-v0.6.1-windows-x64.zip` | `7a3862ab73e5f5d9536d58c5eb9c15cad89a492d9266d6ae399089140d1b5b08` |
| `GeometryWarsRetroEvolved2Recompiled-v0.6.1-x86_64.AppImage` | `15f9b5160b778bd9cd67c1ef94e95fd6831e276459b185155144f38f6d0cc7ed` |
