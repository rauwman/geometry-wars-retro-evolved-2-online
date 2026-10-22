# Friends leaderboard (online names)

The port can send each player's best scores to a small private leaderboard server, so friends see each other on a
shared board. It is optional: with no server address the game stays fully offline, as before.

## For players

1. Open the launcher's **Online** page.
2. Enter the server address and invite code you got from the person who runs the server. (A ZIP made for friends, see below, already has them filled in.)
3. Type the name you want. The launcher asks the server while you type and tells you if the name is taken.
4. Press **Create online name**. The name and a private token are stored in `user_data\launcher\settings.toml`.

From then on, every new best score is uploaded in the background when a game ends. If the server cannot be reached,
the upload is retried with growing pauses; nothing slows the game down. **Change name** renames your account;
**Sign out** stops uploading on this PC (the name stays reserved on the server).

Keep `settings.toml` private: the token in it is the key to your account. Reset to defaults leaves the account alone.

### Getting your name back after a reinstall

Your name belongs to a private account key that lives in the game's settings. Before you delete or reinstall the game,
open the Online page and press **Copy account key**; keep the key somewhere private (a password manager). In the new
installation, enter the same server address, paste the key under **Restore** and press **Restore account**. You keep
your name, and your best scores upload again the next time you finish a game.

Lost the key? Ask the person who runs the server: `python -m gw2lb.admin rotate-token <name>` prints a new key (the old
one stops working), which you paste the same way.

### Scores earned with cheats

A game in which a cheat was in effect at any point is not uploaded; the score still counts on this PC. Scores that
existed before the account was made are uploaded once, as they are.

## For the person who runs the server

The server is a separate project, [gw2-leaderboard](https://github.com/rauwman/gw2-leaderboard) (FastAPI + SQLite, one Docker container behind HTTPS). Its README explains what it assumes you already know: running Docker and publishing it safely behind a reverse proxy and your own domain.

A public release carries no server address and no invite code. There are two ways to get them to your players:

1. Tell them the address and the invite code privately. They enter both on the launcher's Online page once.
2. Build a ZIP for friends that has both filled in:

   ```powershell
   .\tools\package-release.ps1 -ServerUrl https://gw2.example.org -InviteCode the-shared-invite-code
   ```

   The result is `dist\GeometryWarsRetroEvolved2-friends.zip`. Share it privately only: the
   invite code can be read from the file. The normal `package-release.ps1` run (no parameters) refuses to package a
   build whose configuration holds an address or a code, so a public ZIP cannot carry them by accident.

Do not commit the invite code to a public repository. Plain `http://` is accepted only for `localhost` and private
network addresses, for testing.

## On Linux and Steam Deck

The Linux build talks to the server through the system `libcurl.so.4` (loaded when needed, so the game starts without it). A Steam Deck and most distributions have it; otherwise install your distribution's libcurl package. The Online page works as on Windows, and the Paste and Copy buttons use the system clipboard. In Gaming Mode the Steam Deck has no keyboard shortcut for pasting into the launcher, so use the buttons. This is untested on real hardware so far.

## What the game shows

- **Leaderboards:** the game's own leaderboard screens list your friends, ranked per mode, with you under your name.
  All three filters (Overall, My score and Friends) show everyone on the server: the game's friend list is the other
  players on the server (`XFriendsCreateEnumerator` is answered by the port).
  The boards come from the server in the background (at start, when a leaderboard is opened, after each upload and
  every 30 seconds); the screen never waits for the network. Without a connection the game shows your own scores, as
  offline.
- **Your name:** the name shown under the menus and in the leaderboards is your online name. This replaces the
  runtime's fixed profile name "User" in memory only; saves and settings are untouched.
- **Your own row** shows the better of the server's score and your clean best on this PC, so a new score appears at once,
  before the upload has finished.
