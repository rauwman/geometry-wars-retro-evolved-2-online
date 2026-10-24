<div align="center">

# Geometry Wars: Retro Evolved 2 - PC Port

### Modded fork with a global leaderboard

[![Latest release](https://img.shields.io/github/v/release/rauwman/geometry-wars-retro-evolved-2-recompiled?style=for-the-badge&label=release&color=943cff&labelColor=07101f&cacheSeconds=300)](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/rauwman/geometry-wars-retro-evolved-2-recompiled/total?style=for-the-badge&color=943cff&labelColor=07101f&cacheSeconds=300)](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/releases)

</div>

<img width="1905" height="1071" alt="2026-10-11_18-47" src="https://github.com/user-attachments/assets/08b1ed2c-3244-41c1-94f1-e2ae4c1b76d1" />

This is an unofficial fan-made fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled), the Xbox 360 game Geometry Wars: Retro Evolved 2 statically recompiled to a native PC program with the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). Everything except the friends leaderboard is the original port, with all credit to its author. It is not affiliated with or endorsed by the game's makers or publishers. **No game files are included.**

## What this fork adds

- **Online name.** The launcher has an **Online** page: pick a name (checked while you type) and create it. No invite and no server address: the global leaderboard comes with the game.
- **Global leaderboard and friends.** Your best scores are uploaded in the background when a game ends. The game's own **Overall** screen is the global board, **My score** shows you among the players around you, and **Friends** shows you and the friends you added, ranked per mode. Friends are asked by name and need the other player's consent.
- **No cheated scores.** A game in which a cheat was in effect is not uploaded.
- **Account key.** Copy your private key on the Online page; after a reinstall, paste it under **Restore** to get your name back.
- **Nothing blocks the game.** Without a server or a connection the game shows your own scores, as before.
- **Linux / Steam Deck build that runs well.** The native Linux build ran at about 30 FPS on a Steam Deck, with the music slowing down along with it. The cause was in the ReXGlue runtime (it searched `/proc/self/maps` thousands of times a second under a global lock), and this fork fixes it: 60 to 90 FPS on the maintainer's Steam Deck. Details: [docs/linux.md](docs/linux.md) and [docs/rexglue-runtime-patch.md](docs/rexglue-runtime-patch.md#why-0048-and-0049).

A server is **not** included and not hosted for you: someone you know has to run one. The server is a separate project, **[gw2-leaderboard](https://github.com/rauwman/gw2-leaderboard)** (a Docker image; running it and putting it safely on the internet behind HTTPS is up to you). How the game side works: [docs/online.md](docs/online.md).

## Vibe-coded

**The friends leaderboard in this fork is vibe-coded**: written with heavy AI assistance (Claude) and directed, built and tested by hand by the fork's maintainer (rauwman), not hand-written line by line. It works for the maintainer's group of friends, but it has had no independent review. Expect rough edges, and do not treat the server as hardened for the open internet. The original port (by saulob) also used AI tools, see the [full manual](docs/manual.md#ai-disclosure).

## Download

**GeometryWarsRetroEvolved2-v1.2.0-windows-x64.zip** from the [latest release](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/releases/latest). The version is the fork's own; this release is based on upstream v0.6.0. The launcher checks this repository's releases for updates.

You need Windows 10 or 11 (64-bit), a DirectX 12 graphics card and **your own Geometry Wars: Retro Evolved 2 Xbox Live Arcade package**. Extract the ZIP, run `geometry_wars_retro_evolved_2.exe` and install the game from your package on the launcher's **Play** page. The exact game version, launcher pages, saves and troubleshooting are in the [full manual](docs/manual.md).

**Linux / Steam Deck (native, tested on one Steam Deck):** download **GeometryWarsRetroEvolved2Recompiled-v1.2.0-x86_64.tar.gz** from the same release and unpack it (the unpacked AppImage starts with a double click; a bare AppImage download first needs "Is executable" under Properties > Permissions). The game files go into a `game` folder next to the AppImage, installed from your package on the launcher's **Play** page. Steps for the Steam Deck, requirements and known limits are in [docs/linux.md](docs/linux.md). The friends leaderboard is built into it too (it needs the system `libcurl.so.4`).

**Frame rate (Steam Deck and every other PC):** the game simulates at 60 steps per second, so set the launcher's frame rate limit to **60 or 120 FPS**. At 90 or 144 FPS busy scenes such as explosions can stutter.

## More

- [Full manual](docs/manual.md): requirements, installing, launcher, cheats, building it yourself
- [Leaderboard server](https://github.com/rauwman/gw2-leaderboard) (Docker)
- [Friends leaderboard](docs/online.md) and [making a release](docs/releasing.md)
- [Upstream project](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled)

## License

GPL-2.0, see [LICENSE](LICENSE). The ReXGlue license and third-party notices are in [packaging/licenses](packaging/licenses).
