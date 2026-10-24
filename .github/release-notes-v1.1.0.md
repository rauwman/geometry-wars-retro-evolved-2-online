## Geometry Wars: Retro Evolved 2 v1.1.0 - Global Leaderboard on upstream v0.5.0

This is an unofficial fork of [saulob/geometry-wars-retro-evolved-2-recompiled](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled). It is based on **upstream v0.5.0** and adds an online leaderboard and a Linux / Steam Deck performance fix. It is not affiliated with or endorsed by the game's makers or publishers. **No game files are included.** You need your own Geometry Wars: Retro Evolved 2 Xbox Live Arcade package.

### New in this release

- **A global leaderboard that anyone can join.** The Online page of the launcher now has the server built in (`gw2.rauwman.org`): pick a free name and you are on the board. No server address and no invite code are needed any more. The game's Overall screen shows everyone's best scores, with your own place and the rows around it.
- **Friends by name.** On the Online page, type a player's name to ask them to be friends. They see the request on their own Online page and accept or decline it. Only accepted friends appear on the game's Friends screen. A friend who accepts while the game is running appears after the next start.
- **Only scores earned online count.** A score is uploaded when you finish a game while signed in, and a game in which a cheat was in effect is not uploaded. Scores from offline play, from before you had an account and from an edited save are never uploaded. If the server cannot be reached, the upload is retried while the game keeps running. Your own scores on this PC are always kept, and without a connection the game shows them as before.
- **Your account is yours to manage.** Create a name, rename it, restore it with your account key on another PC, or delete it (this removes the name, the scores and the friendships from the server; the saves on your PC stay). Keep the account key private: it is the only way back into the account.
- **Your progress follows your name.** The game keeps part of your profile in a folder named after the profile name. When you switch to another online name, the newest copy is carried over, so a new or renamed account keeps your unlocked game modes.
- **Everything from upstream v0.5.0** and the Linux / Steam Deck performance fix of v1.0.0 (see the [v1.0.0 notes](https://github.com/rauwman/geometry-wars-retro-evolved-2-recompiled/releases/tag/v1.0.0)).

### If you used the friends leaderboard of v1.0.0

Your account, scores and friendships were moved to the new server, and the game of v1.0.0 keeps working against it. Update to v1.1.0 to get friends by name and the global board. After updating, nothing needs to be set up again.

### About the leaderboard

Scores cannot be verified, so cheating cannot be ruled out. The operator can hide a player or delete a score. The server is a small free Cloudflare Worker with a D1 database, [gw2-leaderboard](https://github.com/rauwman/gw2-leaderboard); it stores a name, a hash of your account key, your best score per mode and your friendships, and no e-mail address or password. The leaderboard is vibe-coded and has had no independent review, see the README.

### Downloads

- **GeometryWarsRetroEvolved2-v1.1.0-windows-x64.zip**: Windows 10 or 11 (64-bit), DirectX 12 graphics card, latest Microsoft Visual C++ 2015-2022 Redistributable (x64). Extract anywhere you can write to and run `geometry_wars_retro_evolved_2.exe`. To update from an older version, extract over the old folder; your `game` and `user_data` folders are kept. The launcher also offers the update itself (**UPDATE NOW**). The build is not digitally signed, so SmartScreen may warn: choose More info, then Run anyway.
- **Linux / Steam Deck:** the native Linux build of v1.1.0 (AppImage and `.tar.gz`) follows and will be attached to this release. Until then, Linux players stay on v1.0.0; the global leaderboard server also works with it (see "If you used the friends leaderboard of v1.0.0").

Each download has a matching `.sha256` file. Compare the SHA-256 of the file you downloaded with it.

### Known limitations

- Linux and Steam Deck remain experimental and are tested on one Steam Deck.
- Everything in [upstream's known limitations](https://github.com/saulob/geometry-wars-retro-evolved-2-recompiled/releases/tag/v0.5.0) applies as well.

### Reporting problems

Please open an issue and attach the newest `geometry_wars_retro_evolved_2_*.log` file from the `logs` folder next to the program (the launcher's About > Open logs folder opens it). Problems that are not about the leaderboard or the Linux fix may be upstream's; this is a hobby fork without any support promise.
