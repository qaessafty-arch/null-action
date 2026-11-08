# NULL-ACTION: bringing in a realistic environment and real 3D models

The game runs today on **stand-in shapes** (cubes, spheres, cylinders) so that every system works without any art.
From v2.1, real assets plug in **without code changes**:

**Unreal Editor → Edit → Project Settings → Game → NULL-ACTION Assets**

Every slot is optional. Fill one and that thing turns real; leave it empty and the stand-in stays.
The settings are saved to `Config/DefaultGame.ini`.

---

## 1. Where to get assets (free and properly licensed for Unreal projects)

| Source | What you get | Cost / licence |
|---|---|---|
| **Fab** (fab.com, built into the editor: *Window → Fab*) | Everything: Quixel Megascans (photo-scanned rocks, ground, plants, trees), characters, creatures, animals | Megascans are **free for use in Unreal Engine**. Many other packs are free; Fab gives away free packs every month. Check each item's licence ("Standard licence" is fine for games). |
| **Quixel Megascans** (now inside Fab) | Photo-real terrain materials, rocks, cliffs, grass, bushes, forest floor, trees | Free with Unreal Engine |
| **MetaHuman Creator** (metahuman.unrealengine.com) | Photo-real human characters | Free for Unreal Engine projects |
| **Epic sample projects** (Epic Launcher → *Samples*) | High-quality environments, the UE5 mannequins and animations, Electric Dreams PCG forest | Free; may be used in UE projects |
| **Mixamo** (mixamo.com) | Rigged characters and thousands of animations (run, swim, ride, death) | Free with an Adobe account; usable in games |

**Rules from the project brief:** use only assets with a clear licence and don't buy anything without approval.
Keep a note of each asset and its licence in `Docs/ASSET_LICENSES.md`.

---

## 2. Realistic environment (biggest visual jump, ~30 minutes)

1. Open **Fab** in the editor and search **"Megascans"**.
2. Add to the project:
   - one **ground surface** (e.g. *Forest Floor* or *Grassy Soil*);
   - 2–4 **rocks or boulders**;
   - 1–3 **trees** (search "Megascans trees" or the free *European Hornbeam* / *Black Alder* packs);
   - one **grass clump**;
   - one **bush**.
3. Go to **Project Settings → NULL-ACTION Assets → Environment** and set:
   - **Terrain Material**: the ground material's instance (`MI_...`). The terrain also writes a vertex colour per surface type (grass, forest, rock, snow, sand) if your material wants to tint by it.
   - **Tree Meshes**: add each tree static mesh. **Tree Scale**: 1.0 (try 0.8–1.5).
   - **Rock Meshes**: add the rocks. **Rock Scale** to taste.
   - **Grass Mesh**, **Bush Mesh**, **Grass Scale**.
4. Press Play. The streamed world rebuilds with the real meshes. Trees and rocks keep working for gathering (E).

Tips:
- In the mesh editor, turn on **Nanite** for Megascans rocks and trees for lots of detail at low cost.
- Lighting, fog, clouds and the colour grade are already set up by the world environment.
- For a fully hand-built map (Landscape + World Partition + PCG forest), ask for the "real level" step. The game currently spawns its creatures, shrines and events from the procedural world, so a custom map needs those spawn points hooked up too.

---

## 3. Real characters

### Player and soldiers
**Fastest:** *Add → Add Feature or Content Pack → Third Person*. The game finds Epic's mannequin automatically, already animated.

**Your own character:**
1. Import or add a rigged skeletal mesh (Fab, Mixamo FBX or MetaHuman body). For Mixamo, retarget its animations to the UE5 mannequin with the **IK Retargeter**.
2. Use an Animation Blueprint that reads the pawn's speed and falling state. The Third Person template's `ABP_Unarmed` (or ABP_Manny) works with any character.
3. **Project Settings → NULL-ACTION Assets → Characters → Player**: set **Mesh** and **Anim Class**.
   - **Offset**: 0. **Rotation**: Yaw -90 (most UE characters face +Y). **Scale**: 1.
4. Set **Soldier** the same way with a different mesh, so enemies look different.

With a real skeleton, headshots use the real head bone. **MetaHumans** are built from several meshes (body, face, hair), so they need a small Blueprint step. Make one and I'll wire it in.

### Creatures and mounts
Slots: **Ash Stalker, Wolf, Horse, Deer, Ash Glider, Boss**.

1. Get a rigged, animated creature from Fab, e.g.:
   - a wolf pack for **Wolf**;
   - any big quadruped monster for **Ash Stalker**;
   - a horse with riding animations for **Horse**;
   - a deer;
   - a dragon or wyvern for **Ash Glider**;
   - a giant beast for **Boss**.
2. Set the slot's **Mesh** and **Anim Class**. Then tune:
   - **Offset**: the slot puts the model's root at the bottom of the capsule; adjust Z if it floats or sinks.
   - **Rotation**: usually Yaw -90.
   - **Scale**: 1 to start.
3. The shape body hides automatically. The invisible shape parts still take the bullet hits, so weak spots (head, the boss's core) keep working.

**About animation Blueprints from Fab packs:** many expect their own demo character class. If yours stands still in game, open the Anim BP's Event Graph:
- change "Cast to <TheirCharacter>" to **Try Get Pawn Owner → Get Velocity → Vector Length** for speed;
- use **Is Falling** from the Character Movement component.

That is a two-minute edit; send a screenshot if you get stuck.

---

## 4. Weapons, buildings, sounds and effects (v2.2)

Same settings page, more sections:

| Section | Slots | What to look for on Fab (filter: Free) |
|---|---|---|
| **Weapons** | Pistol, Assault Rifle, Shotgun, Sniper Rifle, Hunting Bow: each has **Model** (static mesh), **Fire Sound** and **Reload Sound** | "pistol", "AK", "rifle", "shotgun", "sniper", "bow" static meshes. Barrel along +X; if it points the wrong way set **Rotation** Yaw 90 / -90 / 180. Start with **Scale** 1. |
| **Building** | Foundation (3x3 m floor), Wall (3 m wide, 2.8 m high), Doorway (wall with an opening; the door is added automatically), Campfire, Loot Crate | "wooden wall", "log cabin modular", "campfire", "wooden crate". Modular survival or medieval building packs fit best. Tune Offset/Scale until it fills the build grid. |
| **Sound** | Footstep, Bullet Impact, Creature Growl/Bite/Death, Boss Roar, Horse Neigh, Ambient Day, Ambient Night, Rain Loop, Menu Music, UI Click | Fab and Epic samples have free sound packs: search "footsteps", "gunshot", "creature roar", "forest ambience", "night ambience", "rain loop", "UI click". The engine's Starter Content also includes some sounds. |
| **Effects** (Niagara) | Muzzle Flash, Bullet Impact FX, Blood, Campfire Fire, Lightning Strike, Ash Rebirth | Fab has free Niagara packs: search "Niagara muzzle flash", "impact", "blood", "fire", "lightning", "magic burst". The free **Niagara examples** content from Epic also works. |

Everything stays optional: fill what you have, and the rest keeps the stand-ins.

## 5. A realistic "shopping list" (all free), in the order that matters most
1. **Third Person pack** (human player, already animated).
2. **Megascans**: 1 ground, 3 rocks, 2–3 trees, grass, bush, filled into **Environment**.
3. **Forest and night ambience + rain loop + footsteps**, into **Sound**. Sound makes the biggest jump in "realism" after the visuals.
4. **One rifle and one pistol model + gunshot sounds**, into **Weapons**.
5. **Muzzle flash, impact and blood Niagara**, into **Effects**.
6. **Wolf, deer and horse** animated animals, into **Creatures**.
7. **Modular wooden building pieces + campfire + crate**, into **Building**.
8. **A dragon/wyvern** (Ash Glider) and **a large monster** (Boss).

The project's renderer is also tuned for photo-real assets: Lumen lighting, virtual shadow maps, Nanite, virtual textures (for Megascans), TSR anti-aliasing and a wider auto-exposure range. The first launch after this update may compile shaders for a few minutes; that's normal.

## 6. What stays to do after models are in
- **Riding animations** for the player on horses and gliders.
- **A sculpted logo and font:** drop a font asset in and the menus can use it.

Tell me which pieces you've added and I'll tune the fit (offsets, scales, animation hookups) from your screenshots.
