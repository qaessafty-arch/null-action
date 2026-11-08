# NULL-ACTION — Development Status

_Last updated: 2026-11-08 — v2.2: realism hookup (weapon/building/crate models, sounds, ambience, music, Niagara effects, renderer tuned for photo-real assets). **v2.1.1 builds and runs on the owner's PC (confirmed)**, so v2.0 and v2.1 compile too. v2.2 not yet built._

## v2.2: realism hookup (models, sound, effects, renderer)

Status: **written, not yet compiled or tested (v2.2).**

New slots in **Project Settings → Game → NULL-ACTION Assets** (all optional; empty = stand-in):
- **Weapons:** a real model in the hand for each of the 5 weapons (the muzzle point moves to the tip of the model), plus fire and reload sounds.
- **Building:** foundation, wall, doorway (keeps its working door), campfire (keeps its light, plus real flames if the Campfire Fire effect is set), loot crate.
- **Sound:**
  - Footsteps for every player: quieter crouched, silent prone or swimming.
  - Bullet impacts.
  - Creature growl/bite/death for every creature without its own.
  - The boss roars when it wakes; a horse neighs when tamed.
  - Looping day and night ambience that crossfades with the clock, and a rain bed that follows the weather.
  - Menu music (fades in and out); a UI click.
- **Effects (Niagara):** muzzle flash, bullet impact, blood on every hit, campfire flames, lightning (Storm power and Ember Storm), Ash Rebirth burst at the shrine.
- **Renderer:** TSR anti-aliasing, Nanite, virtual textures, a wider auto-exposure range and Lumen mesh-distance-field tracing. Lumen GI/reflections and virtual shadows were already on. The Niagara plugin is enabled.
- **Docs/REAL_ASSETS_GUIDE.md** has a new section on these slots and a prioritised free "shopping list".

Honest limits:
- No assets are included: licensed assets can't be downloaded from here, and buying anything needs approval.
- The creature growl is heard by the host only (bite and death sounds play for everyone).
- Footsteps are one sound for every surface.

## v2.1: Stage 6 presentation, part 1 + real-asset slots

Status: **builds and runs on the owner's PC (v2.1.1, 2026-11-08).** Checklist items not yet reported individually.

- **Intro (first launch):** a camera journey through the live world (the lake, the Ash Crater, the Old Cabin, an Ash Shrine) with story captions, ending in the NULL-ACTION title reveal.
  - Letterbox bars. **Space or click skips it.**
  - It plays only on first launch (saved on this PC). **REPLAY INTRO** is in the main menu.
  - It's built from cameras moving through the real world, not a Sequencer asset (no music yet).
- **Main menu** over the live world: a slow aerial orbit of the arena while time, weather and creatures carry on. Your body is hidden, ignored by AI and can't be hurt while you're in it. Buttons, all working:
  - **Play.**
  - **Multiplayer:** Host = listen server; Join this PC; instructions for LAN/internet/dedicated play.
  - **Character Studio.**
  - **Hunting Guide:** bestiary and gear.
  - **Missions & Events:** shows the live world event.
  - **Inventory / Progression:** open the I menu or skills tab in game.
  - **Settings, News, Replay Intro, Exit.**
  - **Friends** is greyed out ("needs online accounts") until Stage 3B: honest rather than a fake button.
- **Character Studio:**
  - The camera orbits your real character; rotate and zoom with buttons.
  - Options: outfit (6), skin (5), goggle colour (6), helmet on/off, backpack on/off. Changes preview live.
  - **Save Look** stores it on this PC and sends it to the server. It's replicated, so every player sees your look on any server; it's re-sent each time you join. **Reset** and **Back (discard)** also work.
- **Settings** (apply immediately and are saved):
  - graphics quality Low/Medium/High/Epic, resolution scale, window/fullscreen, V-Sync, frame limit (Unlimited/60/120/144);
  - mouse sensitivity, field of view (70–110), invert Y.
  - Audio settings wait for the sound pass.
- **Pause menu:** **Esc or P** (in the editor, Esc stops Play, so use **P**). Resume, Settings, Controls, Main Menu, Quit. Single player really pauses; online the world keeps going and you're **not** protected (no dodging fights by pausing).
- **Real-asset slots:** **Project Settings → Game → NULL-ACTION Assets**.
  - Characters: Player, Soldier.
  - Creatures: Ash Stalker, Wolf, Horse, Deer, Ash Glider, Boss.
  - Environment: terrain material, tree meshes, rock meshes, grass, bushes, plus scales.
  - Empty slot = stand-in shapes. Filled = the real model (creature hit zones keep working).
  - Full walkthrough: **Docs/REAL_ASSETS_GUIDE.md**.

Honest limits:
- Menus are drawn with Canvas (code), not UMG widgets.
- No music or sound yet; no Sequencer cinematic.
- Customisation colours the stand-in body; with a real mesh it would need material slots (next step).
- Joining over the internet needs typing `open <IP>` in the console (no IP text box yet).

### v2.1 test checklist
- [ ] First launch: the intro plays; Space skips it; the next launch goes straight to the main menu
- [ ] Main menu: the world orbits behind it; every button does something; Friends is greyed out
- [ ] Character Studio: change outfit/skin/goggles/helmet, Save; walk around; a second player sees your look
- [ ] Settings: change graphics quality and FOV, close and restart, settings are kept
- [ ] P in game: pause menu; Resume; Main Menu returns to the orbit
- [ ] Project Settings → NULL-ACTION Assets: set Tree Meshes to any static mesh → trees change in game

## v2.0: the big Stage 5 update (owner asked for one massive update)

Status: **compiles and runs (confirmed with the v2.1.1 build on the owner's PC).** Gameplay checklist not yet reported.

### Survival
- **Fatigue (REST bar):** drains over ~50 minutes awake, faster when sprinting. Below 25% stamina recovers slowly (worse at 0). Restore it by standing still beside a campfire; an Ember Tonic restores it fully.
- **Bleeding:** heavy hits (20+ damage, 30% chance) and animal bites (50%) open a wound (0.6 HP/s). A **Bandage** stops it. Untreated for 90 s it becomes an **Infection**.
- **Poison:** eating Raw Meat has a 30% chance to poison you for 40 s (0.5 HP/s, double thirst, weak stamina). **Antidote** (2 Herbs + 1 Berries) cures it. Cook your meat.
- **Infection:** 0.15 HP/s and half stamina recovery until **Medicine** (4 Herbs + 2 Berries at a campfire; also heals 20).
- **Weapon durability:** every shot wears your gun (shotgun and sniper faster; AI guns don't wear). Below 25% it's inaccurate; at 0 it won't fire. **Repair Kit** (3 Scrap + 1 Wood) restores the gun in your hands. Durability bar under the ammo counter.
- **Protective gear:** **Fur Coat** (3 Wolf Pelts + 1 Deer Hide) halves cold; **Rebreather** (6 Scrap + 2 Deer Hide + 1 Ember Cell, or shipwreck loot) makes your breath last 4x longer underwater.
- **Survival difficulty:** `SurvivalDifficulty=Easy|Normal|Hard` in DefaultGame.ini, or owner command `NASurvival Easy|Normal|Hard` (needs drain x0.5 / x1 / x1.6).

### Progression
- **XP** from: soldiers 25, Ash Stalker 60, wolf 30, deer 20, boss 500 (everyone within 80 m), taming 50, reviving 40, skinning 10, crafting 5, world events 150 (Ember Storm survivors 75). "+XP" pops up on screen; an XP bar runs along the bottom edge.
- **Levels 1-30**, one **skill point** per level. **K** opens the Skills tab (also the third tab of the I menu). Up/Down, Enter to learn:
  - Vitality: +10 max health per rank
  - Endurance: +12% stamina recovery and less sprint drain per rank
  - Hunter's Eye: +8% damage to creatures and bosses per rank
  - Survivalist: -12% hunger and thirst drain per rank
  - Power Mastery: -10% power cooldown and +10% energy recovery per rank
  - 3 ranks each, so 15 points max. Gains are capped on purpose so high levels aren't unbeatable.
- Saved in the server-side player record (version 2), on level-up and on logout. Console: `NASkill Vitality`. Owner testing: `NAGiveXP 1000`.

### Wildlife
- **Grey Wolf pack** (3, den in the Whispering Pines, south-west). Smaller, faster, weaker than the Ash Stalker. When one spots you, packmates within 30 m join the hunt and flank. Drops Wolf Pelt and meat.
- **Deer** (4, near the pines). Very skittish: anything faster than a crouch within 16 m makes them bolt. Can't be tamed. Drops meat and Deer Hide.
- Skinning works on wolves too (pelt instead of hide). Tracks show the right species name. Kill feed and XP use species names.

### World events (every 8-12 minutes, first one ~4 min in)
Objective panel top-left with a timer; a marker and distance in the world.
- **Creature Sighting:** an alpha Ash Stalker (bigger, 450 HP). Kill it for XP and a Rare or Epic drop.
- **Supply Drop:** a high-quality crate somewhere out there. The first player to open it wins.
- **Distress Signal:** switch on the beacon (E). It transmits for 45 s and draws 3 soldiers; hold the area and supplies drop.
- **Ember Storm:** a thunderstorm with falling embers. Red circles appear near players and burst 1.6 s later (30 damage). Survivors get XP.
- Owner: `NAEvent sighting|supply|beacon|storm` starts one now.

### Loot rarity
Items have tiers: Common, **Uncommon (green)**, **Rare (blue)**, **Epic (purple)** and **Legendary (orange)**, shown in the inventory and crafting lists.
- Crates roll a bonus item: Common 55%, Uncommon 30%, Rare 12%, Epic 3%.
- High-quality crates (supply drops and the shipwreck) give Rare or better plus an **Ember Cell** (new rare resource; Ash Stalkers also drop one 15% of the time).
- All rolls are made on the server.

### Exploration and gear
- **Shipwreck** at the bottom of Mirror Lake (~6 m down, south-east part of the lake), with a high-quality crate inside.
- **Binoculars** (4 Scrap + 2 Stone): carry them and press **B** to zoom (moving lowers them). The scan shows the creature, boss, mount or soldier you're looking at: its state, health %, distance, weak spot and counter-tips.

### Engineer (5th class)
`NAClass Engineer`. **Q: Energy Barrier**, a 3 x 2.2 m shield wall in front of you that stops bullets, creatures and players for 10 s (300 health, so focused fire breaks it). 50 energy, 22 s cooldown.

### Ash Glider (flying mount)
- Two roost on the Grey Peaks foothills (north-east, around X 14500, Y -4000). They perch, then take off and circle high above the roost, then land again. Running near one makes it take off.
- **Tame it while it's perched:** walk up and feed it Raw or Cooked Meat 3 times. Only the owner can ride it.
- **Riding:**
  - On the ground, **Space** takes off.
  - In the air: **W** flies where you look, **A/D** strafe, **Space** climbs, **C** dives, **Shift** boosts.
  - With no input it glides and slowly sinks.
  - Below 1.6 m it lands by itself; **E** lands when near the ground and dismounts once landed.
- **Stamina:** climbing and boosting cost stamina; gliding recovers it. At 0 it can only glide down.
- **Weather:** thunderstorms, blizzards, dust storms and wind buffet and slow it, and the HUD warns you.
- **Whistle:** H calls your mount (horse or glider).
- **Riding HUD:** altitude, mode, stamina and controls.
- Owner: `NASpawn wolf|deer|glider`.

### Honest limits
- All new creatures and gear are built from engine shapes with code animation; there are no sculpted models, sounds or particle effects yet.
- No aquatic monsters or underwater caves yet; the shipwreck is the first underwater content.
- No crafting stations beyond the campfire, no base storage, no horse saddlebags.
- Companions only follow when whistled; there are no stay or attack commands.
- Engineer has one power. Powers have no upgrade tree of their own; Power Mastery scales them.
- No automated tests yet. All of this needs the in-game checklist below.

### v2.0 test checklist
- [ ] Builds; F1 panel shows `dev build 2.0`
- [ ] REST bar drains; standing still by a campfire refills it
- [ ] Take a soldier's burst → sometimes BLEEDING; a Bandage stops it. Eat raw meat a few times → POISONED; an Antidote cures it
- [ ] Fire a gun a lot → the durability bar drops; a Repair Kit restores it
- [ ] Kill things → "+XP"; level up → message and skill point; K → learn Vitality → max health goes up
- [ ] Leave and rejoin → level and skills are still there
- [ ] Wolves south-west hunt as a pack; deer bolt unless you crouch
- [ ] `NAEvent supply` → marker → open the crate → event complete + rare item. Try `NAEvent storm` and `NAEvent beacon`
- [ ] Craft Binoculars, press B on a creature → scan panel
- [ ] Dive to the shipwreck in the lake (bring a Rebreather or come up for air) and open its crate
- [ ] `NAClass Engineer`, Q → shield wall stops bullets
- [ ] Find an Ash Glider, feed it meat 3x, ride it, take off, climb, dive, land. Fly in a storm (`NAWeather Thunderstorm`) and feel the gusts

## Milestone 5E: wild horses: taming and riding (Stage 5, part 5)

Status: **written, not yet compiled or tested (v1.4).**

- **Wild herd**: 4 horses (bay, black, grey or chestnut) graze on the grassland west of the arena (around X 2500, Y -9500). They wander, lower their heads to graze, and **bolt** if anyone moves faster than a walk within 14 m (or shoots them). A cloaked Phantom doesn't spook them. Dead horses drop meat and are replaced after 5 minutes.
- **Taming**: walk (Alt) or crouch up to one, press **E** with **Berries or Ashroot Herbs** to feed it. Each feed adds 34% trust; spooking it costs trust. Three feeds tame it. It's yours (tied to your account id), and a saddle and bridle appear on it.
- **Riding**: **E** on your horse mounts it (only the owner can). Your controller steers the horse.
  - **WASD:** trot (~20 km/h).
  - **Shift:** gallop (~39 km/h); drains the horse's stamina, and it's winded at 0.
  - **Space:** jump. **E:** dismount on the left side.
  - Steep slopes slow it (down to 55%); wading through water slows it to 45%.
  - If you're knocked down or the horse dies, you're thrown off.
- **Whistle**: **H** calls your horse to you from up to 150 m.
- **Riding HUD**: compass, your health, food and water, the horse's health and stamina, speed in km/h, and controls.
- **Owner/admin**: `NASpawn horse` spawns a wild horse where you're looking.

Honest limits: you can't shoot from the saddle yet. The stand-in body sits astride, but the real mannequin, if installed, stands in the saddle until a riding animation is added. Horse ownership isn't saved across server restarts yet. Customisation is coat colour only (random). Flying mounts come next.

### 5E test checklist
- [ ] Builds; F1 panel shows `dev build 1.4` and a Horses line
- [ ] Find the herd west of the arena. Sprint at them → they bolt. Walk up (Alt) → they stay
- [ ] `NAGive Berries 5`, E three times on one horse → "You tamed a ... horse!", saddle appears
- [ ] E to ride; the riding HUD appears. Shift gallops and stamina drains; Space jumps; E dismounts and your normal controls come back
- [ ] Ride up a steep hill or into the lake → it slows
- [ ] Walk 50 m away, press H → your horse comes to you
- [ ] Two players: the second player can't ride your horse

## Visual pass (owner request): new HUD, better character, better world (v1.3)

Status: **builds and runs on owner's PC (v1.3, 2026-11-08).** Individual checklist items not yet reported.

**Character.** The stand-in body was a cylinder with a block for a face. It is now a **jointed human figure** made from engine shapes: helmet, goggles, olive jacket and vest, backpack, arms, legs, knee pads and boots. It animates in code:
- **Movement:** walk and run strides with arm swing, a forward lean when sprinting, and folded legs when crouched.
- **Weapons:** both arms come up to hold the weapon, higher when aiming.
- **Lying down:** face down when prone or downed, on its back when dead.

Soldiers wear dark red with red goggles, and training dummies are orange.

**Real human model (free).** For a properly modelled and animated character, add Epic's free UE mannequin, which you may use in Unreal Engine projects:
1. In the editor, click **Add** (or **+ Add** in the Content Browser).
2. Choose **Add Feature or Content Pack → Third Person**, then **Add to Project**.
3. Restart Play.

The game now looks for the mannequin in all known folder layouts (`SKM_Manny`/`SKM_Manny_Simple`, `ABP_Manny`/`ABP_Unarmed`) and uses it automatically, with per-bone headshots. The jointed figure is only the fallback.

**World.**
- Engine volumetric clouds.
- Volumetric fog, so light shafts show through the trees.
- A world-wide colour grade: slightly desaturated ash look, warm highlights, vignette, light bloom, film grain and ambient occlusion.
- **Grass tufts** in patches and **bushes** across the open terrain, kept off the arena floor, the cabin, the crater and water.
- The arena floor is **concrete** instead of the engine's debug grid, and the block colours are toned down to rust, steel and sandstone.

**HUD.**
- A **compass** strip at the top, with the region name under it.
- Clock and weather in a panel at the top right; the kill feed moved below it.
- Vitals in a dark panel with a thicker health bar and labels inside the bars.
- Weapon slots are small boxes (they no longer run off-screen with 5 weapons).
- The boss bar moved below the enemy warnings.
- The controls list is hidden: **F1** shows or hides it.

Honest limits: the jointed body is still a stand-in, and the real fix is the free mannequin above. Grass and bushes are simple shapes, not real foliage meshes. The HUD is still drawn with Canvas; the UMG redesign is Stage 6.

### Visual pass checklist
- [ ] Builds; top-left says `F1 controls ... dev build 1.3`; F1 opens and closes the controls panel
- [ ] Your character is a human figure; walking, running, crouching, aiming, prone and dying all look right
- [ ] Soldiers are red figures, dummies orange
- [ ] Clouds in the sky; grass and bushes outside the arena; the arena floor is concrete
- [ ] Compass turns with the camera (N is red); the clock is top right
- [ ] Optional: add the Third Person pack → the real mannequin replaces the figure

## Milestone 5D: first boss, the ASHEN BEHEMOTH (Stage 5, part 4)

Status: **compiles (confirmed with the v1.3 build on owner's PC); gameplay checklist not yet reported.**

- **Ash Crater** (new region, east of the arena and north of the Old Cabin, around X 12500, Y 6500): a flat ash bowl with a raised rim, no trees.
- **Ashen Behemoth**: a ~5 m armored four-legged beast built from engine shapes (an original design). It sleeps until a player comes within 22 m or shoots it, then everyone gets the message "The ASHEN BEHEMOTH has awoken".
  - Health 1800 + 900 per extra living player (it scales when it wakes).
  - **Armor:** body hits do 50%, legs 70%. **Weak points:** the glowing **ember core on its back** and the **head** (count as headshots). The bow's creature bonus applies.
  - **Phase 1:** *Ground Slam*, a red circle (6.5 m) under it, 1.2 s warning, then 45 damage and a knock-back. *Charge* from 9–30 m away, 40 damage and a knock-back on contact. It turns slowly, so get behind it.
  - **Phase 2 (below 60%):** adds *Ember Rain*, three red circles on and around the target (one where you're running), 35 damage each.
  - **Phase 3 (below 25%):** **ENRAGED**, faster, shorter warnings, harder slams.
  - **Leash:** if it goes 60 m from its lair or finds nobody for 10 s, it walks home, heals fully and sleeps again.
  - **Rewards:** Behemoth Heart (unique), 3 Ember Fangs, 4 Ash Hide, 2 Ammo Packs, meat and scrap, spread around the body. Kill feed and a server-wide message. It returns 10 minutes later.
  - **Ashen Aegis** (new recipe at a campfire: Behemoth Heart + 3 Ash Hide + 4 Scrap): use for full health, full armor and +1 Ash charge.
- HUD: boss health bar with phase name, a weak-point hint, and "!! GET OUT OF THE RED CIRCLE" when you stand in a telegraph. The Hunter's Tracking Pulse marks the boss.
- Max health now replicates to clients (needed for correct boss and health bars).

Honest limits: the boss walks straight at its target (no pathfinding around big obstacles; the crater is open ground on purpose); no boss music or sounds yet; shape-built body until a real model exists.

### 5D test checklist
- [ ] Builds; HUD `dev build 1.2`; region name "ASH CRATER" when you walk there (head east-north-east from the arena)
- [ ] Get close → wake message, boss bar appears
- [ ] Red circle under it → step out before it lands; standing in it hurts and throws you
- [ ] Shooting the glowing back core shows HEADSHOT damage; body shots do much less
- [ ] Below 60% → Ember Rain circles; below 25% → ENRAGED
- [ ] Run far away → it walks home and heals
- [ ] Kill it (owner: `NAGod` helps) → loot ring, kill feed, server message; craft Ashen Aegis at a campfire

## Milestone 5C: hunting equipment and tracking (Stage 5, part 3)

Status: **compiles (confirmed with the v1.3 build on owner's PC); gameplay checklist not yet reported.**

New craftable gear (I → Crafting tab):
| Item | Recipe | What it does |
|---|---|---|
| Hunting Bow | 6 Wood + 2 Herb | Use it to add the bow to **slot 5 (key 5)**. 50 damage, one arrow at a time, **almost silent** (AI hears it at ~6% of a gunshot's range), **+60% damage to creatures**. Starts with 12 arrows. |
| Arrow Bundle | 2 Wood + 1 Stone | Use: +6 arrows (max 40). Ammo Packs, supply pickups and crates don't refill the bow. |
| Hunting Knife | 2 Scrap + 1 Wood | Keep it in your inventory and press E on a dead Ash Stalker to **skin** it: +1 Ash Hide, +2 Raw Meat, 25% chance of an Ember Fang. One skinning per carcass. |
| Snare Trap | 3 Scrap + 2 Wood | Use: sets a dark jaw trap on the ground in front of you (needs fairly flat ground, max 5 armed per player). The first creature or enemy that steps in takes 30 damage; **creatures and soldiers are held for 5 s**. Other players take only the damage (their movement is predicted on their own machine, so the server doesn't freeze it). It never triggers on the player who set it. |
| Meat Bait | 1 Raw Meat + 2 Berries | Use: drops bait (lasts 2 min). A roaming creature within 35 m smells it, walks over and **eats for 10 s**, distracted: it ignores you unless you come within 7 m. |

Tracking: Ash Stalkers leave **paw prints** every ~4.5 m (they fade after 90 s). Look at one for "Ash Stalker tracks - fresh (23 s), heading north-east" (+X is north). A Hunter's Tracking Pulse also highlights tracks within 30 m.

Also: creature loot now drops beside the body instead of under it. HUD `dev build 1.1`.

Honest limits: the bow uses the same instant-hit trace as the guns (no arrow drop or flight time yet); trap and bait models are simple shapes; there's no separate "Hunt Mode" screen yet, so tracking goes through the paw prints and the Hunter power.

### 5C test checklist
- [ ] Builds; HUD says `dev build 1.1`
- [ ] Owner: `NAGive Wood 30`, `NAGive Herb 10`, `NAGive ScrapMetal 20`, `NAGive Stone 10`, `NAGive RawMeat 3`, `NAGive Berries 6`, then craft each new item
- [ ] Use the Hunting Bow → key 5 equips it; shots are quiet (a soldier nearby doesn't turn round as it would for a pistol); it hits the Ash Stalker hard
- [ ] Arrow Bundle adds 6 arrows; an Ammo Pack doesn't add any
- [ ] Follow paw prints from the den; the prompt shows age and heading
- [ ] Drop bait near the den → the Stalker walks over and eats; it ignores you at range
- [ ] Set a snare in its path → it snaps shut, the Stalker is held for 5 s and takes 30 damage
- [ ] Kill it, E on the carcass with the knife → extra hide/meat; E again does nothing

## Milestone 5B: downed & revive, carrying, ASH REBIRTH and class powers (Stage 5, part 2)

Status: **compiles (confirmed with the v1.3 build on owner's PC); gameplay checklist not yet reported.**

What was added (all server-authoritative; clients only ask):
- **Downed state (players only).** At 0 health you go down instead of dying: you crawl slowly, can't shoot, aim, sprint, jump, build or use your power, and you bleed out over ~30 s. Damage while down finishes you. Soldiers stop targeting downed players so teammates get a chance; creatures do not.
- **Revive.** Look at a downed teammate and **tap E steadily** (about once every 0.25–1.4 s, like CPR). Mashing faster does nothing; slow taps count half. About 13 good taps; progress fades if you stop. Revived players come back at 30% health and move slower for 20 s.
- **Carry.** **Crouch + E** on a downed teammate puts them over your shoulder (you move at 60%, can't shoot). **E** puts them down. They are dropped automatically if either of you dies, you go down, or you start swimming.
- **ASH REBIRTH.** Three Ash Shrines: *Arena Shrine* (inside the arena, south side), *Cabin Shrine* (south of the Old Cabin), *Lakeside Shrine* (Mirror Lake shore facing the arena). Use one to **attune**. Everyone starts with **1 Ash charge** (max 3). Offer an **Ember Fang** (Ash Stalker drop) at your attuned shrine for +1 charge.
  - Die with a charge, attuned, and no rebirth in the last 5 minutes → you rise at your shrine after 3 s **with all your items**, at 60% health, Ash-touched (slower) for 120 s. The shrine flares.
  - Otherwise you respawn normally and **everything in your inventory drops where you died** as pickups. The death screen and a system message tell you which happened and why.
- **Classes and powers (Q).** Pick with the console command `NAClass Phantom | Hunter | Storm | Guardian` (default Hunter). Energy bar (100, +6/s) and cooldowns are on the server.
  - **Phantom – Cloak** (50 energy, 20 s cooldown): invisible to other players for 8 s; AI only notices you within 3 m. Shooting or taking damage breaks it.
  - **Hunter – Tracking Pulse** (35, 15 s): 10 s of on-screen markers for enemies, creatures and (uncloaked) players within 80 m, through walls.
  - **Storm – Lightning Strike** (60, 18 s): marks where you aim (max 40 m, line of sight checked by the server); everyone sees a red **!! LIGHTNING** warning; 0.6 s later 70 damage in a 3.5 m radius (falls off with distance, never hits you).
  - **Guardian – Restoration Field** (60, 25 s): 8 s field, heals you and players within 6 m by 8/s; revives inside it go 60% faster.
- **HUD:** downed screen with bleed-out and revive bars, "+ DOWN 12m" markers over downed teammates, carry/recovering/Ash-touched status lines, bottom-centre power panel (`[Q] name READY / cooldown`, energy bar, `ASH 1/3 - shrine name`), cloak tint, field indicator, rebirth info on the death screen. Version string `dev build 1.0`.

Honest limits: no team system yet (any player can revive or carry any other); the cloak hides the body and weapons but footsteps still make AI-audible noise; power visuals are simple (tracer bolt, light flare, HUD markers) until real effects/assets exist; carrying is a simple attach (no carry animation).

### 5B test checklist
- [ ] Builds; HUD says `dev build 1.0`; bottom centre shows `[Q] Tracking Pulse READY` and `ASH 1/3 - not attuned`
- [ ] Walk to the **Arena Shrine** (tall dark pillar with a glowing ember), press E → "attuned" message; HUD shows the shrine name
- [ ] `NAGive EmberFang 1` (owner) then E at the shrine → `ASH 2/3`, shrine flares
- [ ] Get killed by a soldier with a charge → go **down** first (crawl, red screen, bleed-out bar). Let it run out → death screen says **ASH REBIRTH**, you rise at the shrine with your items, slower for 120 s, `ASH 1/3`
- [ ] Die again within 5 minutes → items drop at your body, message says the shrine is recovering; walk back and pick them up
- [ ] Two players (Listen Server): down one, the other taps E steadily → revive bar fills, revived at 30% health. Mashing E fast does not fill it
- [ ] Crouch + E on a downed teammate → carried over the shoulder; E puts them down
- [ ] `NAClass Storm`, Q at a spot → both players see the red warning, then the bolt and damage
- [ ] `NAClass Phantom`, Q → other player can't see you; shooting reveals you; soldiers ignore you unless very close
- [ ] `NAClass Hunter`, Q → markers on soldiers and the Ash Stalker through walls
- [ ] `NAClass Guardian`, Q while a teammate is hurt → their health rises; reviving inside the field is faster

## Milestone 5A: inventory, gathering, crafting and base building (Stage 5, part 1)

Status: **compiles (confirmed with the v1.3 build on owner's PC); gameplay checklist not yet reported.**

| Feature | How it works |
|---|---|
| Inventory | 24 slots, stacking; the server owns it and replicates it only to you. **I** opens the panel (Left/Right tabs, Up/Down select, Enter use/craft/build). |
| Items | Wood, Stone, Ashroot Herb, Berries, Scrap Metal, Raw Meat, Ash Hide, Ember Fang; Bandage, Cooked Meat, Ammo Pack, Hide Armor, Ember Tonic; Wood Foundation, Wall, Doorway, Campfire. All defined in one table (`Inventory/NAItems.cpp`). |
| Gathering | **E** on world trees = wood, boulders = stone (+ chance of scrap); herb bushes, berry bushes and scrap piles near the cabin, arena and combat yard (regrow after 90 s). Trees/rocks give 5 harvests then rest 3 min. The server checks a real tree or rock is there, plus range and swing rate. |
| Hunting rewards | The Ash Stalker drops Ash Hide ×2, Raw Meat ×3 and (60%) an Ember Fang; soldiers drop scrap. |
| Crafting | 9 recipes. Cooked Meat and Ember Tonic need a campfire nearby. The server checks the ingredients, so items can't be duplicated. |
| Base building | Use a placeable from the inventory → green/red preview. **LMB** place, **R** rotate, **RMB** cancel. Foundations snap to a 3 m grid; walls and doorways snap to foundation edges; doorways come with a working door. The server re-validates the spot (range, slope, no one inside, no duplicates). |
| Ownership | Pieces remember their builder; only the builder can demolish (refunds half). Pieces have 400 health and can be shot down. |
| Campfire | Flickering light; warms you fast (beats the cold); counts as a cooking station. |
| Owner | `NAGive Wood 50`, `NAGive Campfire 3`, etc. for testing. |

Honest limits: inventory, buildings and world changes aren't saved to disk between server restarts yet (world persistence is a later milestone). No storage boxes yet. UI is the dev HUD; the polished UMG inventory with drag & drop comes in Stage 6.

### 5A test checklist
- [ ] HUD says `dev build 0.9`
- [ ] Outside the arena, **E** on a tree → "+2 Wood"; on a boulder → "+2 Stone"
- [ ] Herb bush, berry bush, scrap pile give items and then look picked-over; they regrow
- [ ] **I**: inventory lists items; Right arrow → crafting list shows what you can make (green)
- [ ] Craft a Campfire (4 wood, 3 stone), use it from Items → place it; you warm up beside it
- [ ] Near the campfire, kill the Ash Stalker, take its meat, craft Cooked Meat, eat it (food goes up)
- [ ] Craft Wood Foundations and Walls; build a small hut; add a Doorway (door opens with E)
- [ ] A second player can't demolish your pieces; you can (refund)
- [ ] Shoot a wall enough and it breaks

## Milestone 4B: streaming open world and the Old Cabin (Stage 4, part 2)

Status: ✅ **Compiles and runs on owner's PC (v0.8)**; owner reports it working. Per-item checklist results not given.

| Feature | How it works |
|---|---|
| Open world | Endless generated terrain around the arena, built in 64 m chunks that **stream in around each player** (and, on the server, every AI) and are removed when nobody is near. Same maths on every machine → identical ground in multiplayer, nothing sent over the network. Uses the engine's ProceduralMeshComponent plugin. |
| Regions | **Ashen Grassland** (rolling hills), **Whispering Pines** (dense forest, south-west), **Grey Peaks** (ridged mountains with rock and snow caps, north / +X), **Mirror Lake** (swimmable lake, north-west), **Old Cabin**. Region name shows under the clock. |
| Surfaces | Grass, dark forest floor, rock on steep slopes, snow on peaks, sand/mud at the lake. |
| Trees and rocks | Pines (trunk + layered canopy) and boulders placed from a deterministic hash; trunks and rocks have collision (also on the server). |
| Mirror Lake | Uses the same swim/dive/oxygen/cold systems as the pool. |
| **The Old Cabin** (east, just outside the arena wall) | 10 × 7 m log cabin: **living room** (table, chairs, bench, shelf, fireplace), **bedroom** (bed, wardrobe, nightstand), **storage** (racks, barrels); windows, roof, chimney, porch, warm interior lights. |
| Doors | Hinged, swing away from you, replicated (everyone sees them open), block bullets and players. Front door and storage door. |
| Loot crate | Server rolls loot (ammo + a random supply); lid stays open; refills after 5 min. |
| Story note | A journal on the table starts the NULL-ACTION story (the Array, the ash sky, the first "Rebirth", the creature at the lake, the facility in the north). |
| Leaving the arena | Vault/climb over any arena wall with **Space**. |

Honest limits: this is a runtime-generated world, not a hand-sculpted Unreal Landscape with World Partition. Those need editor work in your copy of the project (I can guide you), and they're the route to AAA-quality terrain, foliage and Nanite assets. Trees and the cabin are built from simple shapes until licensed or original art is imported. No HLOD/impostors yet (view distance ~320 m). Rain still falls indoors.

### 4B test checklist
- [ ] HUD says `dev build 0.8`; region name under the clock ("TEST ARENA")
- [ ] Vault over the east wall (toward the cabin side) → "OLD CABIN"; open the door with E; walk through all 3 rooms
- [ ] Read the note on the table (text appears in the chat area); search the crate in storage; take the medkit in the bedroom
- [ ] Doors: a second player sees them open/close
- [ ] Walk/fly (`NAFly`) out over hills: terrain and forest keep appearing ahead and disappearing behind
- [ ] Find Grey Peaks (snow on top), Whispering Pines (dense forest), Mirror Lake (swim in it)
- [ ] Trees and boulders block you
- [ ] Frame rate stays acceptable while flying around (`stat fps` in the console)

## Milestone 4A: day/night cycle and dynamic weather (Stage 4, part 1)

Status: ✅ **Compiles and runs on owner's PC (as part of v0.8)**; owner reports it working. (Stage 3B, online services, is on hold until the owner creates Epic/Google developer accounts.)

| Feature | How it works |
|---|---|
| Day/night | Sun moves east → overhead → west; a bluish moon at night; sky light dims at night. A full day = 40 real minutes (owner: `NADayLength <minutes>`, `NATime <hour>`). |
| Weather | Clear, Cloudy, Rain, HeavyRain, Thunderstorm (lightning flashes), Fog, Snow, Blizzard, Windy, DustStorm. Changes by itself every 4–9 minutes along natural sequences (clouds → rain → storm…), blending over ~45 s. Owner: `NAWeather Snow`. |
| Visuals | Fog density, dimmer sun and sky, rain streaks, falling snow, blowing dust with brown tint, lightning flashes. Rain/snow/dust are drawn with instanced shapes (no imported particle assets). |
| Multiplayer | Server owns time and weather; every client sees the same conditions (replicated, smoothly extrapolated). |
| Gameplay: temperature | Ambient 12–24 °C through the day, minus weather (snow −18, blizzard −28, rain −5…). Below 15 °C your body cools (faster when wet from rain); hypothermia slows stamina and, severe, hurts. |
| Gameplay: visibility | Soldiers' sight range shrinks in fog, rain, blizzards, dust and darkness (e.g. fog ×0.35, night ×0.55). |
| Gameplay: movement | Snow slows everyone (×0.88), blizzards more (×0.75). |
| Gameplay: sound | Rain and wind mask footsteps (enemies hear you from shorter range). |
| Gameplay: night predator | The Ash Stalker charges 20% faster and stalks for half as long before attacking at night. |
| HUD | Clock, weather and ambient temperature at the top; "NIGHT" after dark. |

Honest limits: no volumetric clouds or star field yet; rain and snow don't stop under roofs yet (comes with real buildings); thunder needs a sound file; regional weather tables come with the open world (Stage 4 continues).

### 4A test checklist
- [ ] HUD says `dev build 0.7`; clock and weather shown at the top
- [ ] `NATime 20` → dusk then night (moonlight, darker sky, "NIGHT"); `NATime 7` → sunrise
- [ ] `NAWeather Rain` → rain streaks, darker, foggier; `NAWeather Thunderstorm` → lightning flashes
- [ ] `NAWeather Snow` / `Blizzard` → snow, slower movement, body temperature falls (HUD)
- [ ] `NAWeather Fog` → soldiers only notice you much closer
- [ ] `NAWeather DustStorm` → brown tint and blowing dust
- [ ] Night: the Ash Stalker attacks sooner and faster
- [ ] Two players see the same time and weather
- [ ] Leave the game running ~10 minutes: weather changes on its own

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
