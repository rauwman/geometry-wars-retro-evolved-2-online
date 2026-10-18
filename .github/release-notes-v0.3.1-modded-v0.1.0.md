## Geometry Wars: Retro Evolved 2 v0.3.1-modded-v0.1.0 - Friends Leaderboard

This is the first release of a fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled). The version number is upstream's, v0.3.1, with `-modded-vX.Y.Z` appended for this fork's own version. It adds an **optional friends leaderboard**. Without a server address the game behaves exactly as before. This is an unofficial fan-made port, not affiliated with or endorsed by the game's makers or publishers.

### What is new

- **Online name.** The launcher has a new **Online** page: enter the address and invite code you got from the person who runs the server, pick a name (checked against the server while you type) and press **Create online name**.
- **Scores for friends.** Your best scores are uploaded in the background when a game ends. The game's own leaderboard screens (**Overall**, **My score** and **Friends**) list everyone on the server, ranked per mode, with you under your name. The name under the menus is your online name.
- **No cheated scores.** A game in which a cheat was in effect at any point is not uploaded. It still counts on your PC.
- **Account key.** The Online page can copy your private account key. After a reinstall, paste it under **Restore** to get your name back.
- **Nothing blocks the game.** Uploads and board downloads run in the background. Without a connection the game shows your own scores, as before.

Everything from v0.3.1 and earlier is included.

### You need a leaderboard server

A server is a small separate program (FastAPI + SQLite in one Docker container). It is **not** included and not hosted for you: someone you know has to run it and give you its address and an invite code. See [docs/online.md](docs/online.md).

### Download

**GeometryWarsRetroEvolved2-v0.3.1-windows-x64-modded-v0.1.0.zip** needs Windows 10 or 11 (64-bit), a DirectX 12 graphics card and the latest Microsoft Visual C++ 2015-2022 Redistributable (x64). **No game files are included**: you need your own Geometry Wars: Retro Evolved 2 Xbox Live Arcade package (see the [README](README.md) for the exact version). The SHA-256 of the ZIP is in the attached `.sha256` file.

**This release carries no server address and no invite code.** Enter yours on the launcher's Online page.

### Updating

Extract the new ZIP over your old folder. `user_data` (settings, scores, your online account) and `game` stay as they are. Or move both folders into the new one. Deleting the old folder deletes `user_data` with it.

### Known limits

- **Windows only.** The Linux and Steam Deck build of upstream is not part of this release, and the friends leaderboard does not work on Linux yet.
- **Windows SmartScreen** may warn the first time, because the program is not digitally signed. If you downloaded it from this repository's releases page, choose **More info**, then **Run anyway**.
- **The server address and invite code are shared secrets of your group.** Do not post them publicly.
