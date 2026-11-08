NULL-ACTION — STAGE 2
REALISTIC GAMEPLAY, COMBAT, WEAPONS AND AI
Continue developing the existing NULL-ACTION Unreal Engine project.
You must inspect the current project and its development status before making changes.
Do not restart the project, replace working systems, or create duplicate implementations.
Your mission is to transform the initial playable character into a complete action-game experience.

1. ADVANCED CHARACTER MOVEMENT
Implement realistic:

* Walking and running.
* Sprinting.
* Crouching.
* Prone movement.
* Jumping.
* Climbing.
* Vaulting over obstacles.
* Sliding.
* Dodging.
* Falling.
* Landing.
* Swimming.
* Sprint stamina.
* Movement transitions.

Character movement must feel natural.
Use appropriate animations, physics, camera movement, and sound effects.

2. REALISTIC CHARACTER ANIMATIONS
Implement:

* Idle animations.
* Walking and running animations.
* Weapon-holding animations.
* Aiming animations.
* Reloading.
* Shooting.
* Taking damage.
* Falling.
* Getting up.
* Jumping.
* Climbing.
* Swimming.
* Death.
* Revival.

Use animation blending and appropriate movement transitions.
Avoid robotic or disconnected animations.

3. WEAPON SYSTEM
Create a modular weapon architecture.
Initially implement:

* Pistol.
* Assault rifle.
* Shotgun.
* Sniper rifle.

Each weapon must have:

* Shooting.
* Reloading.
* Recoil.
* Ammunition.
* Magazine capacity.
* Damage.
* Bullet travel.
* Hit detection.
* Weapon sounds.
* Muzzle flashes.
* Animations.
* Weapon switching.
* Aiming.
* Attachments.

Make weapons visually realistic.
Allow future weapons to be added without rewriting the system.

4. ADVANCED COMBAT
Create:

* Headshot detection.
* Body damage.
* Limb damage where practical.
* Armor protection.
* Damage feedback.
* Hit reactions.
* Death animations.
* Weapon pickup.
* Weapon inventory.
* Ammunition collection.

Implement server-authoritative damage calculations in preparation for multiplayer.

5. CHARACTER HEALTH AND SURVIVAL
Implement:

* Health.
* Stamina.
* Hunger.
* Thirst.
* Temperature.
* Oxygen.
* Fatigue.

Create a functional HUD showing the necessary survival information.
Make each system interact with gameplay.
For example:

* Low stamina reduces sprinting ability.
* Low oxygen creates underwater danger.
* Cold weather affects survival.
* Injuries affect movement where appropriate.

6. INTELLIGENT ENEMY AI
Create an actual intelligent enemy system.
Start with one human enemy capable of:

* Patrolling.
* Detecting players.
* Hearing gunshots.
* Investigating sounds.
* Chasing.
* Attacking.
* Using cover.
* Shooting.
* Reloading.
* Retreating.
* Searching for missing targets.
* Returning to patrol.

Use Unreal Engine AI Perception, Behavior Trees, or a suitable modern architecture.
The enemy must behave differently depending on difficulty.
Create:

* Easy.
* Normal.
* Hard.
* Expert.

Difficulty must improve decision-making rather than simply increase enemy health.

7. FIRST CREATURE
Create one original, high-quality creature.
Requirements:

* Distinctive 3D model.
* Realistic textures.
* Locomotion animations.
* Attack animations.
* Detection system.
* Hunting behavior.
* Environmental awareness.
* Health and damage.
* Death animation.
* Sound design.

The creature should be dangerous and intelligent.
Do not use a static model with basic movement.

8. INTERACTIVE ENVIRONMENT
Create a small but detailed testing environment containing:

* Forest.
* Cabin.
* Multiple interior rooms.
* Rocks.
* Trees.
* Water.
* Different surfaces.
* Interactive objects.
* Weapon pickup locations.
* Resource collection points.

Allow the player to interact with objects.
Use a consistent interaction system that supports future crafting, doors, storage, vehicles, and equipment.

9. GAMEPLAY INTERFACE
Improve the HUD.
Include:

* Health.
* Stamina.
* Ammunition.
* Equipped weapon.
* Crosshair.
* Interaction prompts.
* Survival indicators.
* Enemy warnings.

Create professional inventory and weapon-selection interfaces.
Maintain a consistent NULL-ACTION visual identity.

10. MULTIPLAYER PREPARATION
All important gameplay systems must be designed with multiplayer compatibility.
Prepare:

* Replicated movement.
* Replicated weapon states.
* Server-authoritative damage.
* Replicated health.
* Inventory validation.
* AI ownership.
* Multiplayer-safe interactions.

Do not create client-only implementations for systems that must work online.

11. QUALITY CONTROL
After implementation:

1. Compile the project.
2. Launch the game.
3. Test every movement action.
4. Test every weapon.
5. Test AI detection and combat.
6. Test creature behavior.
7. Test environmental interactions.
8. Check animation transitions.
9. Check performance.
10. Fix discovered bugs.

Do not report features as complete without testing them.

12. DELIVERABLE
At the end, provide:

* Updated Unreal Engine project.
* All source code.
* Updated Blueprints.
* Functional gameplay systems.
* Updated documentation.
* Screenshots or gameplay recordings where possible.
* Test results.
* Known issues.
* Updated NULL_ACTION_STATUS.md.

FINAL INSTRUCTION
Do not merely describe these systems.
Implement them inside the existing NULL-ACTION project.
Prioritize quality, functionality, realistic behavior, and clean architecture.
When this stage is complete, NULL-ACTION should have a playable human character capable of exploring, fighting, shooting, surviving, and encountering intelligent enemies.
START DEVELOPMENT.
