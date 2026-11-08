# NULL-ACTION — Development Status

_Last updated: 2026-11-08 — Milestone 1 (project foundation + first playable character)_

## First test on owner's PC (2026-11-08)

| Check | Result |
|---|---|
| Project compiles in UE 5.8 (VS 2026) | ✅ **Verified**: editor opened with the NullAction module built |
| Game mode + dev HUD load in Play | ✅ **Verified** |
| Enhanced Input reaches the character (Shift → HUD shows Sprint) | ✅ **Verified** |
| Test arena appears in an empty level | ❌ **Bug found**: the editor's temporary PIE spawn point made the game mode think the level was real, so no arena was built and the player fell into the void. **Fixed** in `Core/NAGameMode.cpp` (ignores `APlayerStartPIE`, always uses arena spawns in arena mode). Fix not yet re-tested. |

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
