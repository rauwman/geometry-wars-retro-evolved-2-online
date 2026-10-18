# Making a release

A release is a ZIP built on your PC and attached to a GitHub release by hand. Nothing is published automatically.

1. Set the version in `CMakeLists.txt`. The upstream version stays in `project(... VERSION x.y.z ...)`; this fork's own
   (mod) version is `set(GW2_MOD_VERSION "M.N.P")`. A release is `vx.y.z-modded-vM.N.P` (for example
   `v0.3.1-modded-v0.1.0`) and its ZIP `GeometryWarsRetroEvolved2-vx.y.z-windows-x64-modded-vM.N.P.zip`. Raise the mod
   version for each fork release; when upstream moves to a new version and you merge it, take that version and keep
   counting the mod version. Write `.github/release-notes-vx.y.z-modded-vM.N.P.md`. Commit both.
2. Make sure the build output holds the default configuration. A public release carries no server address and no invite
   code; `package-release.ps1` refuses to package a build that does:

   ```powershell
   Copy-Item config\geometry_wars_retro_evolved_2.toml build-release\ -Force
   .\tools\test.ps1
   .\tools\package-release.ps1
   ```

   This builds `build-release\` and writes `dist\GeometryWarsRetroEvolved2-vx.y.z-windows-x64-modded-vM.N.P.zip` and its `.sha256` file.
   Open the ZIP once and check that `geometry_wars_retro_evolved_2.toml` has empty `gw2_online_url` and
   `gw2_online_invite` values.
3. Tag and publish:

   ```powershell
   git tag -a vx.y.z-modded-vM.N.P -m "vx.y.z-modded-vM.N.P"
   git push origin vx.y.z-modded-vM.N.P
   ```

   On GitHub: **Releases** > **Draft a new release**, choose the tag, paste the release notes, attach the ZIP and the
   `.sha256` file, publish.
   With the GitHub CLI: `gh release create vx.y.z-modded-vM.N.P dist\GeometryWarsRetroEvolved2-vx.y.z-windows-x64-modded-vM.N.P.zip dist\GeometryWarsRetroEvolved2-vx.y.z-windows-x64-modded-vM.N.P.sha256 --notes-file .github\release-notes-vx.y.z-modded-vM.N.P.md --title "vx.y.z-modded-vM.N.P"`.
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
