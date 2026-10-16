# Friends leaderboard (online names)

The port can send each player's best scores to a small private leaderboard server, so friends see each other on a
shared board. It is optional: with no server address the game stays fully offline, as before.

## For players

1. Open the launcher's **Online** page.
2. Enter the server address and invite code (a release made for friends already has them filled in).
3. Type the name you want. The launcher asks the server while you type and tells you if the name is taken.
4. Press **Create online name**. The name and a private token are stored in `user_data\launcher\settings.toml`.

From then on, every new best score is uploaded in the background when a game ends. If the server cannot be reached,
the upload is retried with growing pauses; nothing slows the game down. **Change name** renames your account;
**Sign out** stops uploading on this PC (the name stays reserved on the server).

Keep `settings.toml` private: the token in it is the key to your account. Reset to defaults leaves the account alone.

### Scores earned with cheats

A game in which a cheat was in effect at any point is not uploaded; the score still counts on this PC. Scores that
existed before the account was made are uploaded once, as they are.

## For the person who runs the server

The server is a separate project (`gw2-leaderboard`: FastAPI + SQLite, one Docker container behind HTTPS). Put its
address and the invite code in the release's `geometry_wars_retro_evolved_2.toml`:

```toml
gw2_online_url = "https://gw2.example.org"
gw2_online_invite = "the-shared-invite-code"
```

Do not commit the invite code to a public repository. Plain `http://` is accepted only for `localhost` and private
network addresses, for testing.

## What the game shows

- **Leaderboards:** the game's own leaderboard screens list your friends, ranked per mode, with you under your name.
  The boards come from the server in the background (at start, when a leaderboard is opened, after each upload and
  every 30 seconds); the screen never waits for the network. Without a connection the game shows your own scores, as
  offline.
- **Your name:** the name shown under the menus and in the leaderboards is your online name. This replaces the
  runtime's fixed profile name "User" in memory only; saves and settings are untouched.
- **Your own row** shows the better of the server's score and your clean best on this PC, so a new score appears at once,
  before the upload has finished.
