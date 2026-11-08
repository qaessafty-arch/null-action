# NULL-ACTION — Multiplayer, server and security

## Model
- **Server-authoritative.** The server (dedicated, or the host on a listen server) owns health, damage, ammo,
  inventory, pickups, AI, creatures, roles and saved records. Clients only send *requests*
  (fire, reload, interact, equip, chat, admin command) and the server validates each one.
- Movement uses Unreal's predicted `CharacterMovementComponent` with server reconciliation and
  smoothing for other players.
- AI soldiers and creatures run only on the server; clients see them through replication, so no client
  can control a creature.

## What runs where (Milestone 3A)
| System | Where | Notes |
|---|---|---|
| Player identity | `ANAPlayerState::GetStableId()` | The online subsystem's unique ID. **Offline/LAN it's not a verified identity**; EOS/Google sign-in (3B) makes it secure. |
| Roles | Server: `Saved/Server/Roles.ini` | `Player < Moderator < Admin < Owner`. Listen-server host = Owner (`bHostIsOwner`). Never stored in the client. |
| Bans | Server: `Saved/Server/Bans.ini` | Checked in `PreLogin`, before the player gets in. |
| Audit log | Server: `Saved/Server/AdminAudit.log` | Every admin command, **including denied attempts**. |
| Reports | Server: `Saved/Server/Reports.log` | `NAReport <player> "reason"`. |
| Player records | Server: `Saved/SaveGames/Player_<id>.sav` | Kills, deaths, play time, first/last seen. Versioned. Saved on leave. |
| Reconnect | Engine inactive-player support (`AGameMode`) | Score and role come back if you rejoin soon; lifetime stats come from the record. |
| Chat | Server routes it | All, Team (= all until teams exist), Proximity (25 m), Private. Max 200 chars, 5 messages per 5 s. Mute is per-listener. |

## Console commands (press `~`)
Chat: `NAChat "hello"`, `NATeam "..."`, `NALocal "..."`, `NAWhisper Name "..."`, `NAMute Name`, `NAUnmute Name`, `NAReport Name "reason"`, `NAPlayers`.

Admin (server checks role): `NAGod`, `NAFly`, `NAHeal`, `NATeleport X Y Z`, `NAGoto Name`, `NABring Name`,
`NAGive pistol|rifle|shotgun|sniper`, `NASpawn soldier|creature|dummy`, `NADifficulty Easy|Normal|Hard|Expert`,
`NAWeather <type>`, `NATime <hour>`, `NADayLength <minutes>`.
Moderator: `NAKick Name`. Owner only: `NABan Name`, `NASetRole Name Player|Moderator|Admin|Owner`.

## Making yourself owner on a dedicated server
1. Start the server and join once. The server log prints `Player joined: <name> id=<ID>`.
2. Stop the server. Create `Saved/Server/Roles.ini` next to the project's Saved folder with the line `<ID>=Owner`.
3. Restart. You're the owner; you can then promote others with `NASetRole`.

## Local and LAN testing (Windows)
- `Scripts\StartServer.bat`: dedicated server on port 7777 (needs the saved level `L_TestArena`).
- `Scripts\JoinLocal.bat`: a client on the same PC (run it twice for two players).
- `Scripts\JoinServer.bat`: a client on another PC (type the server PC's IP; allow UDP 7777 in its firewall).

## Network stress tests (in a client's console)
- `NetEmulation.PktLag 150`: add 150 ms latency. `NetEmulation.PktLoss 5`: 5% packet loss. `NetEmulation.PktLag 0` to reset.
- `stat net`: ping, bandwidth, packet loss. `stat unit`: frame/game/render times. `p.NetShowCorrections 1`: draw movement corrections.
- On the server: `stat net`, and watch tick rate with `stat unit`.

## Not done yet (Milestone 3B, needs the owner's accounts)
- **Epic Online Services** (lobbies, matchmaking, invites, friends, voice): create a product in the Epic Developer Portal, then I wire the EOS plugin with your Product/Sandbox/Deployment IDs. The client credentials ID is public by design; secrets stay off the client.
- **Google sign-in** (OpenID Connect, basic profile scope only, never Gmail): create an OAuth client in Google Cloud Console; sign-in is linked through EOS Connect or a small backend. No secrets in the client.
- **Packaged dedicated server build** needs a source-built engine (GitHub access to Epic's repo).
- **Hosting**: none purchased. Options get compared before you decide.
