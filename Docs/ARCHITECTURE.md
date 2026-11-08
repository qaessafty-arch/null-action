# NULL-ACTION — Architecture

Engine: **Unreal Engine 5.8**, gameplay in **C++**, content/tuning in **Blueprint subclasses**.
Single runtime module `NullAction` today, organised by system folder so each folder can be
split into its own module (or Game Feature plugin) when it grows.

## Layout

```
NullAction.uproject
Config/                 Engine, game and input defaults (text, version-controlled)
Content/NullAction/     All project assets (Git LFS). Maps/ holds L_TestArena.
Source/
  NullAction.Target.cs        Game client target
  NullActionEditor.Target.cs  Editor target
  NullActionServer.Target.cs  Dedicated server target (packaging needs a source-built engine)
  NullAction/
    Core/       Game mode, HUD — server rules and session flow
    Character/  ANACharacterBase (shared humanoid), ANAPlayerCharacter (camera + input)
    Animation/  UNAAnimInstance — native parent for every character AnimBP
    World/      ANATestArena — code-built movement test course
    Combat/     Health/armor component, hit zones, training dummy
    Weapons/    ANAWeapon base, four weapon types, tracer effect
    Survival/   Stamina, hunger, thirst component
    Interaction/ INAInteractable interface, weapon and supply pickups
Docs/                   Architecture and decisions
NULL_ACTION_STATUS.md   Live progress report
```

## Class responsibilities

| Class | Runs on | Responsibility |
|---|---|---|
| `ANAGameMode` | Server only | Spawn rules, default classes. Builds the test arena + player starts in levels that have no PlayerStart. Future home of match rules and owner/admin authorization. |
| `ANACharacterBase` | All | Locomotion speeds, replicated `Gait` (Walk/Run/Sprint), crouch, body mesh resolution. Shared by players **and** future enemy bots/NPCs. |
| `ANAPlayerCharacter` | All (input on owning client) | Over-shoulder spring-arm camera, Enhanced Input bindings. |
| `UNAAnimInstance` | All | Computes speed, direction, aim pitch, falling/crouch/sprint flags for AnimBPs. |
| `ANAHUD` | Owning client | Dev overlay (controls, gait, speed, net mode). Replaced by UMG later. |
| `ANATestArena` | Server-spawned, replicated | Stairs, ramp, jump boxes, crouch tunnel, camera-collision pillars, sun/sky/fog. |

## Multiplayer model

* **Server-authoritative.** The game mode exists only on the server. Clients request, the server decides.
* **Movement** uses `UCharacterMovementComponent` client prediction. Jump and crouch are fully
  predicted by the engine (they are part of saved moves).
* **Gait** is applied locally at once and sent with a validated `Server` RPC; the server value
  replicates to everyone. Because max speed is not yet part of the saved move, a sprint start
  can cause a tiny correction under high latency. Planned fix: custom movement component with a
  sprint flag in `FSavedMove` (standard UE pattern).
* **Admin / owner powers (planned):** authorization will be checked in server code against
  server-side configuration or a backend; clients will only ever send *requests*. No credentials
  or permission lists will ship in the client.

## Combat flow (server-authoritative)

1. Owning client presses fire → `ANAWeapon::FireOnce` applies recoil locally and calls `ServerFire(viewOrigin, viewDir)`.
2. Server rejects if dead, reloading, out of ammo, firing too fast, or the view origin is >6 m from the body.
3. Server traces from the camera ray (moved level with the body) to find the aim point, then from the body's eyes to that point (no shooting around corners), on the `Weapon` trace channel.
4. Damage goes through `ApplyPointDamage` → `ANACharacterBase::TakeDamage` → hit zone → `UNAHealthComponent::ApplyDamage` (armor, death).
5. Server multicasts tracers/muzzle flash and sends the shooter a hit confirmation for the HUD.

Clients never change health, ammo, inventory or pickups; they only send requests.

## Input

Enhanced Input. If a Blueprint subclass assigns a `MappingContext` and actions, those are used.
Otherwise `ANAPlayerCharacter` builds a complete keyboard/mouse + gamepad scheme in code, so the
game is playable with zero editor assets.

## Assets and licensing

No third-party assets are committed. The character body defaults to the Epic Mannequin from
the free **Third Person** content pack (licensed for Unreal projects), loaded only if present.
Without it, a stand-in body is shown and everything still works. Binary assets go through Git LFS
(`.gitattributes`).

## Extension plan

New systems get their own folder (later module): `Combat/`, `Weapons/`, `Inventory/`,
`Interaction/`, `AI/`, `Creatures/`, `Powers/` (likely Gameplay Ability System),
`Survival/`, `Environment/` (weather, day/night), `UI/`, `Admin/`. Bots derive from
`ANACharacterBase` so they share locomotion and animation with players.
