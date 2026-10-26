## Geometry Wars: Retro Evolved 2 v1.2.1 - Fewer antivirus false positives on upstream v0.6.1

This is an unofficial fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled). It is based on **upstream v0.6.1** and keeps the global leaderboard of v1.2.0. **There are no gameplay changes.**

### New in this release

- **The Windows update helper is built differently.** Some virus scanners flagged `trg-updater.exe` of v1.2.0 with generic heuristic detections (a false positive). Upstream v0.6.1 builds it again from the same source with MSYS2 CLANG64, as a self-contained program that uses only Windows system libraries, and writes a more detailed log of every update step. This release takes that over.
- **Linux / Steam Deck:** rebuilt with the new version number, otherwise the same as v1.2.0.
- The leaderboard, your account, your scores and your friends are unchanged.

### Updating

The launcher offers the update itself (**UPDATE NOW**). Or extract the ZIP over your old folder; your `game` and `user_data` folders are kept. If your virus scanner still complains about `trg-updater.exe`, please report it as a false positive to the scanner's vendor and tell us which scanner it was.

### Downloads

- **GeometryWarsRetroEvolved2-v1.2.1-windows-x64.zip**: Windows 10 or 11 (64-bit), DirectX 12 graphics card, latest Microsoft Visual C++ 2015-2022 Redistributable (x64). Extract anywhere you can write to and run `geometry_wars_retro_evolved_2.exe`. The build is not digitally signed, so SmartScreen may warn the first time: choose **More info**, then **Run anyway**.
- **GeometryWarsRetroEvolved2Recompiled-v1.2.1-x86_64.tar.gz** (Linux / Steam Deck): unpack it and start the AppImage with a double click. The bare `.AppImage` is attached too.

Each download has a matching `.sha256` file. Compare the SHA-256 of the file you downloaded with it.

### Known limitations

- Linux and Steam Deck remain experimental and are tested on one Steam Deck.
- The game simulates at 60 steps per second. With the frame rate limit at 60 or 120 FPS it runs smoothly; at 90 or 144 FPS busy scenes such as explosions can stutter (also on a Steam Deck). Use a multiple of 60.
- One player with a PlayStation controller saw the main menu ignore some presses while Steam Input added a second, virtual copy of the controller. It went away after turning off Steam Input for PlayStation controllers in Steam (Settings > Controller). The cause is not confirmed.
- Scores cannot be verified, so cheating cannot be ruled out. The operator can hide a player or delete a score.
- Everything in [upstream's known limitations](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.6.1) applies as well.

### Reporting problems

Attach the newest `geometry_wars_retro_evolved_2_*.log` file from the `logs` folder, and for update problems also the log of the update helper, to an [issue](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/issues).
