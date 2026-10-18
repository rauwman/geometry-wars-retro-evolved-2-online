## Geometry Wars: Retro Evolved 2 v0.4.0-modded-v0.1.1 - Friends Leaderboard on upstream v0.4.0

This is a fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled). The version number is upstream's, v0.4.0, with `-modded-vX.Y.Z` appended for this fork's own version. It adds an **optional friends leaderboard**. Without a server address the game behaves exactly as before. This is an unofficial fan-made port, not affiliated with or endorsed by the game's makers or publishers.

### What is new

- **Online name.** The launcher has a new **Online** page: enter the address and invite code you got from the person who runs the server, pick a name (checked against the server while you type) and press **Create online name**.
- **Scores for friends.** Your best scores are uploaded in the background when a game ends. The game's own leaderboard screens (**Overall**, **My score** and **Friends**) list everyone on the server, ranked per mode, with you under your name. The name under the menus is your online name.
- **No cheated scores.** A game in which a cheat was in effect at any point is not uploaded. It still counts on your PC.
- **Account key.** The Online page can copy your private account key. After a reinstall, paste it under **Restore** to get your name back.
- **Nothing blocks the game.** Uploads and board downloads run in the background. Without a connection the game shows your own scores, as before.

Everything from upstream v0.4.0 is included (controller vibration, aspect ratios, render resolution, graphics card choice and more, see the [upstream release notes](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.4.0)). Compared with `v0.3.1-modded-v0.1.0`, this release only brings the fork up to upstream v0.4.0, and the friends leaderboard now also works in the Linux build (untested on real hardware).

### You need a leaderboard server

A server is a small separate program (FastAPI + SQLite in one Docker container). It is **not** included and not hosted for you: someone you know has to run it and give you its address and an invite code. See [docs/online.md](docs/online.md).

### Download

**GeometryWarsRetroEvolved2-v0.4.0-windows-x64-modded-v0.1.1.zip** needs Windows 10 or 11 (64-bit), a DirectX 12 graphics card and the latest Microsoft Visual C++ 2015-2022 Redistributable (x64). **No game files are included**: you need your own Geometry Wars: Retro Evolved 2 Xbox Live Arcade package (see the [full manual](docs/manual.md) for the exact version). The SHA-256 of the ZIP is in the attached `.sha256` file.

**This release carries no server address and no invite code.** Enter yours on the launcher's Online page.

### Updating

Extract the new ZIP over your old folder. `user_data` (settings, scores, your online account) and `game` stay as they are. Or move both folders into the new one. Deleting the old folder deletes `user_data` with it.

### Known limits

- **Windows only download.** This fork ships only the Windows ZIP. The friends leaderboard is built into the Linux code (it needs the system `libcurl.so.4`), but no Linux package is published from this fork yet.
- **Windows SmartScreen** may warn the first time, because the program is not digitally signed. If you downloaded it from this repository's releases page, choose **More info**, then **Run anyway**.
- **The server address and invite code are shared secrets of your group.** Do not post them publicly.
