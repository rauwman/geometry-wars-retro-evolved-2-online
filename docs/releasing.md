# Making a release

A release is a ZIP built on your PC and attached to a GitHub release by hand. Nothing is published automatically.

1. Set the version in `CMakeLists.txt`. This fork's own version is `set(GW2_MOD_VERSION "x.y.z")`: it is the version of the
   release (tag `vx.y.z`, ZIP `GeometryWarsRetroEvolved2-windows-x64.zip`, with no version in its name so the folder people unpack it into does not go stale) and the one the launcher's self-update
   compares, so it must go up with every release and the release must not be a prerelease. The upstream version this fork
   is based on stays in `project(... VERSION a.b.c ...)`; take it over when you merge upstream, and say it in the release
   notes ("based on upstream va.b.c"). Write `.github/release-notes-vx.y.z.md`. Commit both.
2. Make sure the build output holds the default configuration. A public release carries no server address and no invite
   code; `package-release.ps1` refuses to package a build that does:

   ```powershell
   Copy-Item config\geometry_wars_retro_evolved_2.toml build-release\ -Force
   .\tools\test.ps1
   .\tools\package-release.ps1
   ```

   This builds `build-release\` and writes `dist\GeometryWarsRetroEvolved2-windows-x64.zip` and its `.sha256` file.
   Open the ZIP once and check that `geometry_wars_retro_evolved_2.toml` has empty `gw2_online_url` and
   `gw2_online_invite` values.
3. Tag and publish:

   ```powershell
   git tag -a vx.y.z -m "vx.y.z"
   git push origin vx.y.z
   ```

   On GitHub: **Releases** > **Draft a new release**, choose the tag, paste the release notes, attach the ZIP and the
   `.sha256` file, publish.
   With the GitHub CLI: `gh release create vx.y.z dist\GeometryWarsRetroEvolved2-windows-x64.zip dist\GeometryWarsRetroEvolved2-windows-x64.sha256 --notes-file .github\release-notes-vx.y.z.md --title "vx.y.z"`.
4. For friends, who should not have to type the server address and invite code, build a second ZIP and share it
   privately (never attach it to the release):

   ```powershell
   .\tools\package-release.ps1 -SkipBuild -ServerUrl https://gw2.example.org -InviteCode the-code
   ```

   It is named `...-friends.zip`.

## Getting new versions

Players download the new ZIP from the **Releases** page and extract it over their old folder (`user_data` and `game`
stay). **Watch** > **Custom** > **Releases** on the repository page, or the feed
`https://github.com/<owner>/<repo>/releases.atom`, tells them when one is out.
