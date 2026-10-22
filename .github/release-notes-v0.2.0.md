## Geometry Wars: Retro Evolved 2 v0.2.0 - Friends Leaderboard on upstream v0.5.0

This is an unofficial fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled). It is based on **upstream v0.5.0** and adds an optional friends leaderboard and a Linux / Steam Deck performance fix. It is not affiliated with or endorsed by the game's makers or publishers. **No game files are included.** You need your own Geometry Wars: Retro Evolved 2 Xbox Live Arcade package.

### New in this release

- **Everything from upstream v0.5.0**: high frame rates that keep the original game speed (interpolated frames above 60 FPS), Direct3D 12 performance work, keyboard remapping, controller navigation in the launcher, Exit game in the main menu, better logging and crash reports. See the [v0.5.0 release notes](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.5.0).
- **New version scheme.** The release version is now this fork's own version (v0.2.0). The upstream version it is based on is shown under About ("Based on upstream") and named in the release notes. Files are called `...-v0.2.0-windows-x64.zip` and `...-v0.2.0-x86_64.AppImage`.
- **Launcher self-update from this repository.** The launcher checks this repository's releases once per start and shows a banner when a newer release exists. Nothing is installed until you choose **UPDATE NOW** and then **RESTART TO UPDATE**; the download is verified with its SHA-256, and your game files, saves, settings and logs are kept. Updating from v0.1.x to v0.2.0 is still done by hand.
- **Linux / Steam Deck at 60 to 90 FPS.** The native Linux build ran at about 30 FPS on a Steam Deck. The cause was in the ReXGlue runtime (it searched `/proc/self/maps` thousands of times a second under a global lock). The fix is patches 0048 and 0049 and it is also offered to the SDK: [rexglue-sdk#523](https://github.com/rexglue/rexglue-sdk/issues/523). It is tested on one Steam Deck only.

### Friends leaderboard (unchanged)

An optional **Online** page in the launcher: enter the server address and invite code you got from the person who runs the server, pick a name and your best scores are shared with your friends in the game's own Overall, My score and Friends screens. A game in which a cheat was in effect is not uploaded. Without a server or a connection the game shows your own scores, as before. A server is **not** included: it is a separate project, [gw2-leaderboard](https://github.com/rauwman/gw2-leaderboard). The leaderboard is vibe-coded and has had no independent review, see the README.

### Downloads

- **GeometryWarsRetroEvolved2-v0.2.0-windows-x64.zip**: Windows 10 or 11 (64-bit), DirectX 12 graphics card, latest Microsoft Visual C++ 2015-2022 Redistributable (x64). Extract anywhere you can write to and run `geometry_wars_retro_evolved_2.exe`. To update from an older version, extract over the old folder; your `game` and `user_data` folders are kept. The build is not digitally signed, so SmartScreen may warn: choose More info, then Run anyway.
- **GeometryWarsRetroEvolved2Recompiled-v0.2.0-x86_64.tar.gz**: native Linux x86_64 build using Vulkan (not Proton, not Wine), glibc 2.31 or newer. Unpack it and start the AppImage with a double click (a bare AppImage download first needs "Is executable" under Properties > Permissions). It needs the system `libcurl.so.4` for the leaderboard. See [docs/linux.md](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/blob/main/docs/linux.md).
- **GeometryWarsRetroEvolved2Recompiled-v0.2.0-x86_64.AppImage**: the same AppImage on its own, used by the launcher's self-update.

Each download has a matching `.sha256` file. Compare the SHA-256 of the file you downloaded with it.

### Known limitations

- The default settings file in these downloads has no server address and no invite code; you enter them on the Online page.
- Linux and Steam Deck remain experimental and are tested on one Steam Deck. The Linux self-update replaces the AppImage file itself and is new in this version.
- Everything in [upstream's known limitations](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.5.0) applies as well.

### Reporting problems

Please open an issue and attach the newest `geometry_wars_retro_evolved_2_*.log` file from the `logs` folder next to the program (the launcher's About > Open logs folder opens it). Problems that are not about the leaderboard or the Linux fix may be upstream's; this is a hobby fork without any support promise.
