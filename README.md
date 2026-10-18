<div align="center">

# Geometry Wars: Retro Evolved 2 - PC Port

### Modded fork with an optional friends leaderboard

[![Latest release](https://img.shields.io/github/v/release/rauwman/geometry-wars-retro-evolved-2-recompiled?style=for-the-badge&label=release&color=943cff&labelColor=07101f&cacheSeconds=300)](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/rauwman/geometry-wars-retro-evolved-2-recompiled/total?style=for-the-badge&color=943cff&labelColor=07101f&cacheSeconds=300)](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/releases)

</div>

<img width="1905" height="1071" alt="2026-10-11_18-47" src="https://github.com/user-attachments/assets/08b1ed2c-3244-41c1-94f1-e2ae4c1b76d1" />

This is an unofficial fan-made fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled), the Xbox 360 game Geometry Wars: Retro Evolved 2 statically recompiled to a native PC program with the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). Everything except the friends leaderboard is the original port, with all credit to its author. It is not affiliated with or endorsed by the game's makers or publishers. **No game files are included.**

## What this fork adds

- **Online name.** The launcher has an **Online** page: enter the server address and invite code you got from the person who runs the server, pick a name (checked while you type) and create it.
- **Scores for friends.** Your best scores are uploaded in the background when a game ends. The game's own **Overall**, **My score** and **Friends** screens list everyone on the server, ranked per mode.
- **No cheated scores.** A game in which a cheat was in effect is not uploaded.
- **Account key.** Copy your private key on the Online page; after a reinstall, paste it under **Restore** to get your name back.
- **Nothing blocks the game.** Without a server or a connection the game shows your own scores, as before.

A server is **not** included and not hosted for you: someone you know has to run one. See [docs/online.md](docs/online.md).

## Vibe-coded

**The friends leaderboard in this fork is vibe-coded**: written with heavy AI assistance (Claude) and directed, built and tested by hand by the fork's maintainer (rauwman), not hand-written line by line. It works for the maintainer's group of friends, but it has had no independent review. Expect rough edges, and do not treat the server as hardened for the open internet. The original port (by saulob) also used AI tools, see the [full manual](docs/manual.md#ai-disclosure).

## Download

**GeometryWarsRetroEvolved2-v0.3.1-windows-x64-modded-v0.1.0.zip** from the [latest release](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/releases/latest). The version is upstream's (v0.3.1) with the fork's own version (`modded-v0.1.0`) appended.

You need Windows 10 or 11 (64-bit), a DirectX 12 graphics card and **your own Geometry Wars: Retro Evolved 2 Xbox Live Arcade package**. Extract the ZIP, run `geometry_wars_retro_evolved_2.exe` and install the game from your package on the launcher's **Play** page. The exact game version, launcher pages, saves and troubleshooting are in the [full manual](docs/manual.md).

**Windows only for now.** The Linux and Steam Deck build of upstream is described in the manual; the friends leaderboard does not work there yet.

## More

- [Full manual](docs/manual.md): requirements, installing, launcher, cheats, building it yourself
- [Friends leaderboard](docs/online.md) and [making a release](docs/releasing.md)
- [Upstream project](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled)

## License

GPL-2.0, see [LICENSE](LICENSE). The ReXGlue license and third-party notices are in [packaging/licenses](packaging/licenses).
