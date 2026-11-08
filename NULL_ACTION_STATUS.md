# NULL-ACTION — Development Status

_Last updated: 2026-11-08 — Milestone 2B (enemy AI) written on top of 2A; neither compiled yet. Milestone 1 verified on owner's PC._

## Milestone 2B: enemy soldier AI (Stage 2, part 2)

Status: **written, not yet compiled or tested.** Build and test together with 2A.

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
