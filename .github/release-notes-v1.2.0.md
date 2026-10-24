## Geometry Wars: Retro Evolved 2 v1.2.0 - Global Leaderboard on upstream v0.6.0

This is an unofficial fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled). It is based on **upstream v0.6.0** and keeps the global leaderboard of v1.1.0.

### New in this release

- **Everything from upstream v0.6.0:** native HDR output and Smooth glow, the in-game Options menu (**F1**, or **L3 + R3** on a controller), renderer and performance improvements, the Unlock all game modes option, the 100 lives and Movement Speed cheats, steadier audio and configurable local player names. The [v0.6.0 notes](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.6.0) list them all.
- **Your online name and the new player names.** With an online account, player 1 is shown under your online name; it replaces the **Player 1** name of the Options while you are signed in. Players 2 to 4 keep the names you set.
- **The global leaderboard, friends by name and online-only scores** work as in v1.1.0. Nothing changes for your account, your scores or your friends.
- **Linux / Steam Deck:** the build carries the fork's two Linux runtime patches (the 30 FPS fix) next to upstream's new ones.

### Updating

The launcher offers the update itself (**UPDATE NOW**) from v1.1.0 on. From v1.0.0, extract the ZIP over your old folder; your `game` and `user_data` folders are kept.

### Downloads

- **GeometryWarsRetroEvolved2-v1.2.0-windows-x64.zip**: Windows 10 or 11 (64-bit), DirectX 12 graphics card, latest Microsoft Visual C++ 2015-2022 Redistributable (x64). Extract anywhere you can write to and run `geometry_wars_retro_evolved_2.exe`. To update from an older version, extract over the old folder; your `game` and `user_data` folders are kept. The build is not digitally signed, so SmartScreen may warn: choose More info, then Run anyway.
- **GeometryWarsRetroEvolved2Recompiled-v1.2.0-x86_64.tar.gz** (Linux / Steam Deck): unpack it and start the AppImage with a double click. The bare `.AppImage` is attached too.

Each download has a matching `.sha256` file. Compare the SHA-256 of the file you downloaded with it.

### Known limitations

- Linux and Steam Deck remain experimental and are tested on one Steam Deck.
- Scores cannot be verified, so cheating cannot be ruled out. The operator can hide a player or delete a score.
- Everything in [upstream's known limitations](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.6.0) applies as well.

### Reporting problems

Please open an issue and attach the newest `geometry_wars_retro_evolved_2_*.log` file from the `logs` folder next to the program (the launcher's About > Open logs folder opens it).
