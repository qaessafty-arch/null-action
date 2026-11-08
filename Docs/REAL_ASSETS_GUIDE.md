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

## 4. What stays to do after models are in
- **Sounds:** footsteps, creature calls, weapons. The code already has sound slots on weapons and creatures (`FireSound`, `GrowlSound`, ...).
- **Particle effects** (Niagara): muzzle flash, blood, embers, lightning.
- **Riding animations** for the player on horses and gliders.
- **A sculpted logo and font:** drop a font asset in and the menus can use it.

Tell me which pieces you've added and I'll tune the fit (offsets, scales, animation hookups) from your screenshots.
