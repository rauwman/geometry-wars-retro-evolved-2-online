# Global leaderboard and friends (online names)

The port can send each player's best scores to a global leaderboard, and shows you where you stand among all players
and among your friends. It is optional: without an online name the game stays fully offline, as before.

## For players

1. Open the launcher's **Online** page.
2. Type the name you want. The launcher asks the server while you type and tells you if the name is taken.
3. Press **Create online name**. The name and a private account key are stored in `user_data\launcher\settings.toml`.

There is no invite and no server address to enter: the server comes with the game. From then on, every new best score
is uploaded in the background when a game ends. If the server cannot be reached, the upload is retried with growing
pauses; nothing slows the game down. **Change name** renames your account; **Sign out** stops uploading on this PC (the
name stays reserved on the server).

Keep `settings.toml` private: the account key in it is the key to your account. Reset to defaults leaves the account alone.

### Friends

Friends need everyone's consent. Anybody can ask you, but nobody becomes your friend without your answer, and nobody sees your friends list.

- Enter a friend's **name** under **Add a friend** and press **Send request**. Your friend sees it under **Requests** on
  their Online page and presses **Accept** or **Decline**. Only after they accept do you see each other on the Friends board.
- If you both ask each other, you are friends at once.
- A declined request is not announced to the sender, and the same player cannot send it again and again.
- **Take back** withdraws a request you sent; **Remove** ends a friendship (it asks once more).
- New friends appear in the game the next time the game starts.

### Delete my account

**Delete account** removes your name, your scores and your friendships from the server for good. Scores stored on this PC
stay. If you only want to stop uploading, use **Sign out**.

### Getting your name back after a reinstall

Your name belongs to a private account key that lives in the game's settings. Before you delete or reinstall the game,
open the Online page and press **Copy account key**; keep the key somewhere private (a password manager). In the new
installation, paste the key under **Restore** and press **Restore account**. You keep
your name, and your best scores upload again the next time you finish a game.

Lost the key? Ask the person who runs the server: the admin API's `rotate-token` call gives you a new key (the old one
stops working), which you paste the same way.

### Scores earned with cheats

A game in which a cheat was in effect at any point is not uploaded; the score still counts on this PC.

### Only games played online count

Only games you finish while signed in are uploaded, and they are uploaded right away. Scores from offline play, from
before the account existed and from an edited save never reach the server, and nothing is uploaded from your profile
when the game starts. A game finished without a connection is retried while the game keeps running; if you quit before
the server answers, that score stays on this PC only. A new account, or one made again after deleting the old one,
starts empty.

## For the person who runs the server

The server is a separate project, [gw2-leaderboard](https://github.com/rauwman/gw2-leaderboard) (a Cloudflare Worker with
a D1 database; the free plan is enough). Its address, `https://gw2.rauwman.org`, is built into the launcher
(`kDefaultOnlineUrl` in `src/launcher_settings.h`). To use another server, for example a test copy on `localhost`, put
`gw2_online_url = "https://host"` into `user_data\launcher\settings.toml`. Plain `http://` is accepted only for `localhost`
and private network addresses, for testing.

Version 1.0.0 talked to a private server and needed an invite code. Its accounts were copied to the new server, so those
players keep their name, their scores and their account key.

## On Linux and Steam Deck

The Linux build talks to the server through the system `libcurl.so.4` (loaded when needed, so the game starts without it). A Steam Deck and most distributions have it; otherwise install your distribution's libcurl package. The Online page works as on Windows, and the Paste and Copy buttons use the system clipboard. In Gaming Mode the Steam Deck has no keyboard shortcut for pasting into the launcher, so use the buttons. This is untested on real hardware so far.

## What the game shows

- **Leaderboards:** the game's own leaderboard screens.
  - **Overall** is the global board, ranked per mode.
  - **My score** shows you with the players just above and below you, wherever you rank.
  - **Friends** shows you and your accepted friends, ranked among themselves.
  The boards come from the server in the background (at start, when a leaderboard is opened, after each upload and
  every 30 seconds); the screen never waits for the network. Without a connection the game shows your own scores, as
  offline. The game asks for its friends once, when it starts, so a new friend appears after the next start.
- **Your name:** the name shown under the menus and in the leaderboards is your online name. This replaces the
  runtime's fixed profile name "User" in memory only; saves and settings are untouched. The runtime keeps the game's
  progress in a folder named after the profile, so at start the folder of your online name receives the newest progress
  found under any name: a new or renamed account keeps playing where this installation left off.
- **Your own row** shows the better of the server's score and your best from games finished online in this run, so a new
  score appears at once, before the upload has finished. Scores from offline play are not shown there.
