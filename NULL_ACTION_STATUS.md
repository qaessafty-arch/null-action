# NULL-ACTION — Development Status

_Last updated: 2026-11-08 — v0.6 (Stages 1-2 complete + 3A) compiles and runs on owner's PC after adding the CoreOnline module. Next: 4A day/night + weather (3B waits on the owner's Epic/Google developer accounts)._

## Milestone 3A: multiplayer foundation and owner permissions (Stage 3, part 1)

Status: ✅ **Compiles and runs on owner's PC (v0.6)**; owner reports it working. First build failed to link (`FUniqueNetIdWrapper::ToString`): fixed by adding the `CoreOnline` module. Per-item checklist and two-PC test results not given yet. Details: [`Docs/MULTIPLAYER.md`](Docs/MULTIPLAYER.md).

| Feature | Status |
|---|---|
| Player state: name, kills, deaths, role (replicated); kill feed for everyone | Written |
| Scoreboard (hold **Tab**): names, role tags, K/D, ping | Written |
| Text chat via console: all, team, proximity (25 m), private; server-side length limit and anti-spam; per-player mute; reports to a server log | Written. Chat UI with a text box comes with the UI stage (Stage 6). |
| **Owner/admin permissions:** roles Player < Moderator < Admin < Owner, kept only on the server (`Saved/Server/Roles.ini`); listen-server host is owner; every command re-checked on the server; **all attempts audited** (including denied ones) | Written |
| Owner/admin commands: God mode, fly, heal, teleport, go to / bring player, give weapon, spawn soldier/creature/dummy, difficulty, kick (mod), ban and set role (owner) | Written |
| Bans checked before a player can join (`PreLogin`) | Written |
| Server-side saved player records (kills, deaths, play time, first/last seen), versioned | Written |
| Reconnect keeps score and role (engine inactive-player support; switched game mode to `AGameMode`) | Written |
| Player limit (16 per server) | Config |
| Dedicated server + client launch scripts (`Scripts/`), LAN join by IP | Written |
| Network stress-test commands (latency, packet loss, stats) | Documented |

**Honest limits:** without an online identity service, player IDs on LAN/offline aren't verified, so owner rights on a *dedicated* server should only be granted once EOS/Google sign-in exists (3B). Lobbies, matchmaking, friends/invites, voice chat, Google sign-in and a *packaged* dedicated server need your Epic/Google developer accounts and a source-built engine (see the doc).

### 3A test checklist
- [ ] HUD says `dev build 0.6` and shows **ROLE: OWNER** (you're the host)
- [ ] Console (~): `NAGod` → GOD shown, damage does nothing; `NAFly` → fly with Space/C; `NAGive sniper`; `NASpawn creature`; `NASpawn soldier`; `NATeleport 0 0 300`
- [ ] Kill a soldier or creature: the kill feed (top right) shows it; Tab shows your kill count
- [ ] 2 players (Play As Client): the **client** typing `NAGod` gets "Permission denied"; `Saved/Server/AdminAudit.log` records the DENIED attempt
- [ ] Host `NASetRole <client name> Admin` → the client can now use `NAGod`
- [ ] `NAChat "hello"` appears for both; `NALocal` only reaches players within 25 m; `NAWhisper Name "hi"` only reaches that player
- [ ] Separate processes: `Scripts\StartServer.bat`, then `Scripts\JoinLocal.bat` twice → two clients in the same world, both see each other, shoot, fight the same AI
- [ ] Leave and rejoin: "Welcome back" shows your lifetime stats
- [ ] With `NetEmulation.PktLag 150` the game stays playable

## Milestone 2D: advanced movement and environment survival (Stage 2, part 4)

Status: ✅ **Compiles and runs on owner's PC (as part of v0.6)**; owner reports it working.

| Feature | Controls | How it works |
|---|---|---|
| Prone | **Z** (toggle) | Lie flat: 70 cm/s, quietest, aim spread x0.45, enemies notice you 3x slower. Space or C gets you back to crouch. |
| Slide | **C while sprinting** | Speed boost and low friction for 0.9 s, ending crouched. |
| Dodge roll | **V** / right-stick click | Quick hop in your steering direction (backwards if still). 20 stamina, 0.9 s cooldown, **0.35 s where damage can't hurt you** (server-checked). |
| Vault / climb | **Space** facing an obstacle | 45 cm–1.3 m: vault over; 1.3–2.6 m: climb on top. The server checks the obstacle itself. 10 stamina. Try the 120 cm jump box. |
| Swimming / diving | In water: **WASD** swims where you look, **Space** up, **C** down, **Shift** fast | New **diving pool** in the back-right corner (stairs on the side facing the arena). Climb out with Space at the wall. |
| Oxygen | automatic | 30 s of breath with your head underwater, then drowning damage. Oxygen bar appears when needed. |
| Body temperature | automatic | Water chills you toward 30 °C. Under 35 °C stamina recovers at half speed; under 33 °C you take damage. Warms back up on land (faster when sprinting). |
| Hard landings | automatic | Falls faster than 11 m/s hurt. |
| Injuries | automatic | Under 35% health you limp (25% slower). |

Honest limits: no character animations for these yet (they need an animated character, e.g. the Third Person pack's Manny, plus animation work). Prone keeps the crouched-height hitbox. Fatigue needs a sleep system (later). Weather-based temperature comes with the weather system (Stage 4).

### 2D test checklist
- [ ] HUD says `dev build 0.5`; new control help line visible
- [ ] Z: lie down (stand-in body lies flat, HUD "prone"), very slow; Z or C back to crouch
- [ ] Sprint + C: slide forward, ending crouched
- [ ] V: dodge roll; stamina drops; can't spam it
- [ ] Space at the 120 cm red box: you vault on; at the 2 m stair landing / ramp platform: you climb up
- [ ] Pool: climb the stairs, jump in; swim, dive with C, rise with Space; screen turns blue underwater
- [ ] Stay under: oxygen bar drains, then "DROWNING!" and health drops; surface to recover
- [ ] Stay in the water: BODY temperature falls; below 35 shows "(cold!)"
- [ ] Climb out with Space at the pool wall
- [ ] Jump off something tall: hard landing damage
- [ ] Health under 35: you move slower
- [ ] Multiplayer: the other player sees you prone, sliding, rolling, swimming

## Milestone 2C: first creature, the ASH STALKER (Stage 2, part 3)

Status: ✅ **Compiles and runs on owner's PC (v0.4)**; owner reports it working. Per-item checklist results not given.

An original predator: low, long body; a row of ember-glowing spines down the back; glowing eyes; a long swaying tail. **Weak point: the head and eyes** (head-shot multiplier). 220 health.

| Requirement | What exists now |
|---|---|
| Distinctive 3D model | Built from engine shapes in code (torso, chest, haunch, neck, head, jaw, fangs, eyes, 6 spines, 4 two-part legs with claws, 5-part tail). **Not** a sculpted, textured model: that needs Blender/ZBrush work or a licensed asset (see below). |
| Locomotion animation | Procedural, in code: trotting legs (diagonal pairs), knee folding, body bounce, breathing at rest, tail sway, crouched posture while stalking. |
| Attack animation | Bite: neck lunges forward and down, jaw opens. Replicated to all players. |
| Detection | AI Perception: 30 m sight (wide 160° view), 40 m hearing (gunfire carries far). |
| Hunting behaviour | Roam → Stalk (circles at ~9 m, low and slow) → Ambush/Hunt when you look away or after ~7 s → bite → spring back → charge again. |
| Intelligence | Dodges sideways when you aim at it; ambushes unaware players instantly; drawn to gunshots; turns on an unseen shooter; flees to its den at 30% health and heals there to 70%; abandons prey that leaves its 45 m territory. |
| Health and damage | Replicated health; body / head / leg hit zones; bite does 25 damage. |
| Death animation | Collapses onto its side; eyes go dark. Respawns at the den after 45 s. |
| Sound design | Code plays growl / bite / death sounds when they're assigned. **No sound files yet** (need licensed or original audio). |

HUD: **"!! SOMETHING IS HUNTING YOU"** when it charges you; a faint **"...you feel watched"** while it stalks you.

**Where:** its den is in the **far back-left corner**: turn around from spawn and go left, past the pillars. Look for dark rocks.

**To get a real high-quality creature model later:** sculpt/rig/animate it in Blender (free) or buy a licensed creature on Fab.com, import it, and point a Blueprint subclass at it. The AI and gameplay code stay the same.

### 2C test checklist
- [ ] The Ash Stalker roams near its den, legs and tail animating
- [ ] Getting seen: it circles you low and slow ("...you feel watched")
- [ ] Turn your back: it charges ("!! SOMETHING IS HUNTING YOU"), bites (health drops 25), springs back, charges again
- [ ] Aim at it while it circles: it darts sideways
- [ ] Fire a gun far away: it comes toward the sound
- [ ] Head shots do much more damage than body shots
- [ ] Hurt it badly: it runs back to the den; later it comes back healed
- [ ] Kill it: it collapses; a new one appears at the den ~45 s later
- [ ] Multiplayer: both players see the same creature, moves and bites

## Milestone 2B: enemy soldier AI (Stage 2, part 2)

Status: ✅ **Compiles and runs on owner's PC (v0.4)**; owner reports it working. Per-item checklist results not given.

Architecture: Behavior Tree / State Tree assets are editor-made `.uasset` files that can't be authored from the cloud workspace, so the soldier's brain is a C++ state machine (`AI/NAEnemyAIController`) on Unreal's **AI Perception** (sight + hearing). It runs only on the server; clients see the result through normal character replication.

| Behaviour | How it works |
|---|---|
| Patrol | Walks a route, pausing and looking around at each point. |
| Detect players | Sight cone (70° each side). An awareness meter fills faster up close and slower if you crouch; reaction time depends on difficulty. |
| Hear gunshots / footsteps | Every shot is a loud noise; sprinting is loud, running audible, walking/crouching nearly silent. |
| Investigate | Goes to a noise or half-seen movement, looks around, then returns to patrol. |
| Chase + attack | Strafes and keeps a preferred range, aims with difficulty-based error, fires in bursts, sometimes at your head. |
| Cover | Finds spots where it can't be seen crouched but can peek standing; crouches there to reload, then pops back up. |
| Reload | When the magazine empties; in cover if it uses cover. |
| Retreat | Hard/Expert: falls back to far cover once when badly hurt, regroups, returns. |
| Search | Lost sight for 3 s: checks around your last known position, then gives up. |
| Return to patrol | After searching or investigating. |
| Reacts to being shot | Immediately fights back against the player who shot it. |
| Calls allies | Hard/Expert: nearby soldiers are told where you are. |
| Respawn | A replacement soldier arrives at the same post 25 s after a death; the dead soldier drops an ammo box. |

**Difficulty changes decisions, not health:**

| | Easy | Normal | Hard | Expert |
|---|---|---|---|---|
| Reaction | 1.6 s | 1.0 s | 0.6 s | 0.35 s |
| Sight / hearing | 25 / 20 m | 35 / 30 m | 45 / 38 m | 55 / 45 m |
| Aim error | 6° | 3.5° | 2° | 1° |
| Head-shot bursts | 0% | 10% | 25% | 45% |
| Uses cover | rarely | often | mostly | almost always |
| Investigates suspicion | no | yes | yes | yes |
| Retreats when hurt | no | no | <35% | <40% |
| Calls allies | no | no | yes | yes |
| Flanks | no | no | no | yes |

Set it in `Config/DefaultGame.ini` (`EnemyDifficulty=`) or, as the host, open the console (`~`) and type `NADifficulty Hard`. Connected clients are refused.

HUD warnings: **"! ENEMY ENGAGING"** (red) when a soldier is fighting you, **"? YOU ARE BEING NOTICED"** (yellow) when one is suspicious of you.

### One-time setup for best AI movement: Nav Mesh Bounds Volume
Soldiers walk straight lines without a navigation mesh, and can get stuck on cover. To give them real pathfinding:
1. Open `L_TestArena`. Press **Place Actors** (the **+** cube icon in the toolbar) and search **Nav Mesh Bounds Volume**. Drag it into the level.
2. In **Details**, set its **Location** to `0, 0, 0` and **Scale** to `45, 45, 5`.
3. **Ctrl+S** to save. Press **P** in the viewport to preview: green = walkable. It appears once the arena exists, which is in Play, because navigation rebuilds at runtime.

### 2B test checklist

The combat yard is to your **right** from spawn, about 30 m away, with crates, low walls, a container and a tall red wall.
- [ ] Three dark-red soldiers with red visors patrol the yard
- [ ] Walking in crouched, you can get closer before they notice than when sprinting
- [ ] Firing a shot nearby makes them come and investigate
- [ ] Once they see you: HUD shows ENEMY ENGAGING, they shoot in bursts and strafe
- [ ] They duck behind cover to reload, then pop up again
- [ ] Break line of sight: they search your last position, then return to patrol
- [ ] Shooting an unaware soldier makes it fight back immediately
- [ ] Killed soldiers drop ammo; a replacement appears ~25 s later
- [ ] `NADifficulty Easy` vs `NADifficulty Expert`: Expert reacts much faster, aims better, uses cover, retreats, flanks
- [ ] Multiplayer: soldiers react to both players; both see their movement and shots


## Milestone 2A: combat foundation (Stage 2, part 1)

Status: ✅ **Compiles on the owner's PC** (UE 5.8.4, VS 2026 / MSVC 14.51: `Result: Succeeded`, no errors, only engine deprecation warnings). ✅ **Owner reports everything working in game** (HUD, weapons, firing, pickups, dummies). Detailed per-item checklist results not given. 2B (enemy AI): owner needs to confirm it is installed and tested.

| System | What it does |
|---|---|
| Health + armor (`Combat/NAHealthComponent`) | Replicated, server-only changes. Armor absorbs 60% of body/limb damage (not headshots). Death and revive events. |
| Hit zones (`Combat/NACombatTypes`) | Head / body / limb from bone names (real skeleton) or hit height on the capsule (stand-in body). |
| Weapons (`Weapons/NAWeapon`, `NAWeaponTypes`) | Pistol, Assault Rifle (full-auto), Shotgun (9 pellets), Sniper Rifle (deep zoom). Each has damage, headshot/limb multipliers, fire rate, spread, recoil, magazine, reserve ammo and reload time. New guns are a subclass with new numbers. |
| Server-authoritative shooting | Client sends view origin + direction; server checks fire rate, ammo and origin distance, traces from camera then from the body's eyes (no shooting through walls), applies damage, confirms hits to the shooter, and multicasts tracers and muzzle flash. |
| Inventory | One weapon per slot (1-4), replicated; switch with 1-4, mouse wheel or bumpers. Picking up a gun you own gives its ammo. |
| Aiming | Hold RMB: camera zooms to the weapon's aim FOV, spread tightens, movement slows, body faces the aim direction. Sprinting stops aiming and firing. |
| Survival (`Survival/NASurvivalComponent`) | Stamina (sprint and jump cost it; exhaustion blocks sprint until 30%), hunger and thirst drain over 45/30 min; low needs slow stamina regen; empty needs drain health. Server-simulated, replicated to the owner only. |
| Interaction (`Interaction/NAInteractable`, `NAPickup`) | Look at an item, press E. Server re-checks range. Weapon pickups plus Ammo, Medkit, Armor, Food and Water supplies; they respawn after 20 s. Interface ready for doors, containers, crafting stations. |
| Death + respawn | Characters fall over (ragdoll with a real skeleton), bodies stop blocking; players respawn after 5 s at an arena spawn point. |
| Training dummies (`Combat/NATrainingDummy`) | Orange targets with the same hit zones and health; stand back up 3 s after dying. |
| Combat HUD | Crosshair that widens with spread, hit marker with damage number / HEADSHOT / KILL, health/armor/stamina/food/water bars, weapon name, ammo, reload state, weapon slots, interaction prompt, damage flash, death screen. |

### 2A test checklist

Layout: weapon rack 5 m in front of spawn; supplies 4.5 m behind spawn; **shooting range to your left** (dummies at 22 m and 34 m).

- [ ] Project compiles
- [ ] HUD shows health, armor, stamina, food, water; pistol and ammo `12 / 48`
- [ ] LMB fires: tracers, muzzle flash, ammo counts down, recoil kicks the view up
- [ ] R reloads (HUD shows RELOADING); empty magazine auto-reloads
- [ ] Look at a weapon on the rack: prompt `[E] Pick up Assault Rifle`; E picks it up and equips it
- [ ] Rifle fires full-auto while LMB is held; shotgun fires a spread; sniper zooms deep with RMB
- [ ] 1-4 and the mouse wheel switch weapons
- [ ] Shooting a dummy shows a hit marker and damage; head shots show HEADSHOT; dummy falls over and stands up after 3 s
- [ ] Sprinting drains stamina; at zero you can't sprint until it refills
- [ ] Supplies: ammo refills reserve; medkit, armor, food and water raise their bars (only when not full)
- [ ] 2 players (Play As Client): shooting the other player damages them, they die, respawn after 5 s
- [ ] Each player only sees their own ammo/needs; both see tracers and deaths

### Not in 2A (coming in 2B-2D, with honest limits)

- **2B:** enemy soldier AI (patrol, perception, hearing gunshots, cover, difficulty levels).
- **2C:** the creature. AI behaviour can be built in code; an original high-quality 3D model, textures and animations need to be made in Blender/ZBrush etc. or bought/licensed. I can't generate production art from here.
- **2D:** prone, vault, slide, dodge, swim, climb, plus temperature and oxygen.
- Weapon sounds, real gun models and character animations need imported assets. The code has slots for sounds; meshes/anims come with art.
- Bullets are instant (hitscan); projectile travel and server lag compensation are later improvements.


## First test on owner's PC (2026-11-08)

| Check | Result |
|---|---|
| Project compiles in UE 5.8 (VS 2026) | ✅ **Verified**: editor opened with the NullAction module built |
| Game mode + dev HUD load in Play | ✅ **Verified** |
| Enhanced Input reaches the character (Shift → HUD shows Sprint) | ✅ **Verified** |
| Test arena appears in an empty level | ✅ **Verified** after fix: arena, lighting, sky, shadows, pillars and wall render; player spawns on the floor with the stand-in body and third-person camera. (First run showed a bug: the editor's temporary PIE spawn point stopped the arena from building. Fixed in `Core/NAGameMode.cpp`; the first fix attempt used a header missing in 5.8, so it now matches the PIE start by class name.) |
| 2-player networked Play-In-Editor (listen server + client) | ✅ **Verified**: both players spawn in the same arena and see each other's body and facing direction |
| Full movement checklist (walk/run/sprint speeds, jump boxes, crouch tunnel, stairs, ramp, camera collision) and remote sprint/crouch replication | ⏳ Waiting for owner's results |
| Separate-process / two-PC dedicated server test | ⏳ Planned for Stage 3 |

## ⚠️ Read first: build state

All code in this repository was written in a **Linux cloud container** that has no Unreal Engine,
no GPU and no Windows toolchain. It is tested on the owner's PC (results above). It is written against
the UE 5.8 API, but until it builds in Unreal Editor on your PC, every feature below counts as
**"written, not verified"**. Your first compile is the first real test. Send me the errors if it fails.

## Environment inspection (Step 1)

| Item | Found | Notes |
|---|---|---|
| Machine | Cloud VM, Ubuntu 24.04, 4 vCPU, 15 GB RAM, ~30 GB free disk, **no GPU** | This is **not your PC**. It is the sandbox Claude Code runs in. |
| Unreal Engine | **Not installed** | Can't be installed here: Epic's source repo needs a linked Epic account (returned 403), a source build needs ~150–250 GB, and the editor needs a GPU. |
| Visual Studio / Windows SDK | Not available | Linux container. |
| Compilers | clang 18.1, g++ 13.3, CMake 3.28 | Not useful without engine headers. |
| Git / Git LFS | git 2.43, git-lfs 3.4.1 | Used for this repo. |
| Current UE release | **5.8** (Epic says it's the last major UE5 release before UE6) | Project targets 5.8. |

### What you need on your Windows PC

1. **Epic Games Launcher** → install **Unreal Engine 5.8** (needs about 60+ GB).
2. **Visual Studio** with the *Game development with C++* workload, plus the Windows SDK and .NET
   components it suggests. Check Epic's "Setting Up Visual Studio for Unreal Engine" page for 5.8 to
   get the exact VS/MSVC versions it supports.
3. **Git + Git LFS** (`git lfs install` once).
4. Recommended hardware: 8+ core CPU, 32 GB RAM, DX12 GPU with 8 GB+ VRAM, SSD.

## Milestone 1: what was implemented

| Feature | Status |
|---|---|
| UE 5.8 C++ project (`NullAction.uproject`, Game/Editor/Server targets, module, config) | Written, not compiled |
| Professional folder structure + architecture doc | Done |
| Source control setup (`.gitignore`, Git LFS rules for binary assets) | Done |
| Enhanced Input. A full KB/mouse + gamepad scheme is built in code, with optional editor-asset override | Written, not compiled |
| Third-person over-the-shoulder camera with collision, lag, and boom that pulls back when sprinting | Written, not compiled |
| Walk (toggle), run (default), sprint (hold), jump, crouch (toggle) | Written, not compiled |
| Replicated gait (server-validated RPC); engine-predicted jump and crouch | Written, not compiled |
| Character body: Epic Mannequin + its AnimBP if the Third Person pack is installed; otherwise a stand-in body | Written, not compiled |
| `UNAAnimInstance` C++ parent for AnimBPs (speed, direction, aim pitch, crouch/sprint/fall flags) | Written, not compiled |
| Test arena built from engine shapes: stairs, ramp, jump boxes, crouch tunnel, pillars, sun/sky/fog | Written, not compiled |
| Game mode: builds arena + 8 player starts in any level that has no PlayerStart (works on a dedicated server too) | Written, not compiled |
| Dev HUD: controls, gait, speed, net mode | Written, not compiled |

## Files created

```
NullAction.uproject, README.md, NULL_ACTION_STATUS.md, .gitignore, .gitattributes
Config/DefaultEngine.ini, DefaultGame.ini, DefaultInput.ini
Docs/ARCHITECTURE.md
Source/NullAction.Target.cs, NullActionEditor.Target.cs, NullActionServer.Target.cs
Source/NullAction/NullAction.Build.cs, NullAction.h, NullAction.cpp
Source/NullAction/Core/NAGameMode.{h,cpp}, NAHUD.{h,cpp}
Source/NullAction/Character/NACharacterBase.{h,cpp}, NAPlayerCharacter.{h,cpp}
Source/NullAction/Animation/NAAnimInstance.{h,cpp}
Source/NullAction/World/NATestArena.{h,cpp}
```

## How to build and test on your PC

1. Clone the repo, then right-click `NullAction.uproject` → **Generate Visual Studio project files**.
   (If it asks for an engine version, pick 5.8.)
2. Open `NullAction.sln` and build **Development Editor | Win64**. Or just double-click the
   `.uproject` and let it compile.
3. In the editor: **File → New Level → Empty Level**, then save it as
   `Content/NullAction/Maps/L_TestArena`. (Maps are binary assets, so I can't author one from here.)
4. Optional, for a real human body and animations: **Content Browser → Add → Add Feature or Content Pack →
   Third Person**. The character picks up the Mannequin automatically.
5. Press **Play**.

### Test checklist (please report results)

- [ ] Project compiles with no errors
- [ ] Arena appears and is lit; you spawn facing the course
- [ ] WASD + mouse move and look; camera doesn't clip through pillars or the long red wall
- [ ] Shift sprints (HUD shows ~680), Alt toggles walk (~180), default run is ~420
- [ ] Space jumps onto the 40 cm and 80 cm boxes but **not** the 120 cm box
- [ ] C/Ctrl crouches; you can pass the tunnel (lane 4) only while crouched
- [ ] Stairs (lane 1) and ramp (lane 2) climb smoothly
- [ ] Gamepad works (left stick, right stick, A jump, B crouch, L3 sprint)
- [ ] **Multiplayer smoke test:** in the Play dropdown, set *Number of Players* = 2 and *Net Mode* =
      *Play As Client*. Both players should see each other move, sprint and crouch.
- [ ] **Separate-process test** (closer to real multiplayer; run from the UE 5.8 `Engine\Binaries\Win64` folder):

```bat
UnrealEditor.exe "C:\path\to\NullAction.uproject" /Game/NullAction/Maps/L_TestArena -server -log -port=7777
```

```bat
UnrealEditor.exe "C:\path\to\NullAction.uproject" 127.0.0.1:7777 -game -windowed -resx=1280 -resy=720 -log
```

(Run the second command twice for two clients. To test from a second PC, use the server's LAN IP and allow UDP 7777 through the firewall.)

## Known limitations / remaining problems

1. **Not compiled.** This is the biggest risk. Small API mismatches with 5.8 are possible.
2. No crouch animation in the stock Mannequin AnimBP; the capsule crouches but the body won't play a crouch pose until crouch anims are added (Milestone 2).
3. Sprint speed isn't yet part of the predicted saved move, so a small correction can happen under high latency (planned: custom movement component).
4. A **packaged** dedicated server (`NullActionServer` target) needs a source-built engine. Running the editor binary with `-server` works with the launcher engine for testing.
5. The "realistic human" is currently the Epic Mannequin. Realistic characters (e.g. MetaHuman) come later.

## Saved roadmap

The owner's Stage 2–7 prompts are saved in [`Docs/StagePrompts/`](Docs/StagePrompts/README.md). Stage 2 starts once Milestone 1 compiles and passes the checklist above.

## Next (Milestone 2)

1. Fix anything from your first compile.
2. Health component (replicated, server-authoritative damage, death and respawn).
3. First firearm: server-validated hitscan, ammo, reload, recoil, aim-down-sights camera.
4. Crouch/aim animation layer on `UNAAnimInstance` + aim offset.
5. Custom movement component with predicted sprint.
6. Interaction system (interface + server-validated "use"), groundwork for inventory.
