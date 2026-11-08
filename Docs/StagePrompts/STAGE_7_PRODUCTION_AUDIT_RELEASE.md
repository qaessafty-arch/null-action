NULL-ACTION — STAGE 7
AAA PRODUCTION, COMPLETE GAME AUDIT, OPTIMIZATION, SECURITY AND RELEASE
You are now acting as the executive production and quality assurance team of a professional AAA game studio.
Your responsibility is to prepare NULL-ACTION for real-world distribution.
Continue working inside the existing Unreal Engine project.
Do not restart the project.
Do not assume that previous stages were completed successfully. Inspect the actual implementation and verify everything.
Your mission is to transform the existing development project into a stable, optimized, secure, tested, and distributable game.

1. COMPLETE PROJECT AUDIT
Perform a complete technical and gameplay audit.
Inspect:

* C++ source code.
* Blueprints.
* Game architecture.
* Character systems.
* Animation systems.
* Weapon systems.
* AI systems.
* Creature systems.
* Survival systems.
* Crafting.
* Inventory.
* Multiplayer.
* Server architecture.
* Authentication.
* Google integration.
* Database.
* World streaming.
* Environment.
* Weather.
* Audio.
* UI/UX.
* Owner Control Center.
* PC configuration.
* Android configuration.

Identify:

* Missing functionality.
* Broken functionality.
* Duplicate systems.
* Performance problems.
* Security vulnerabilities.
* Incorrect implementations.
* Unfinished features.
* Crashes.
* Memory problems.
* Multiplayer synchronization issues.

Create a complete audit report.
Do not mark a feature as completed based on its appearance in the interface.
Verify its actual functionality.

2. CREATE A MASTER BUG TRACKING SYSTEM
Create a professional bug tracking document.
Classify issues:

* CRITICAL: prevents gameplay or causes serious security problems.
* HIGH: major functionality is broken.
* MEDIUM: noticeable gameplay or performance problems.
* LOW: minor visual or usability issues.

For every bug, record:

* Description.
* Reproduction steps.
* Affected system.
* Severity.
* Root cause.
* Proposed solution.
* Fix status.
* Verification result.

Fix critical and high-priority issues first.
Do not introduce new features while serious existing functionality is broken.

3. COMPLETE GAMEPLAY TESTING
Test every major gameplay system.
Verify:

* Character movement.
* Animations.
* Weapons.
* Shooting.
* Damage.
* Character abilities.
* Enemy AI.
* Creature AI.
* Monster hunting.
* Boss fights.
* Survival.
* Gathering.
* Crafting.
* Inventory.
* Base building.
* Horse riding.
* Flying mounts.
* Swimming.
* Underwater exploration.
* CPR.
* Ash Rebirth.
* Missions.
* World events.
* Character progression.

Test complete gameplay sequences rather than testing only individual buttons.
Example:
A player enters a forest, discovers a creature, tracks it, fights it, receives damage, retreats, heals, collects resources, crafts equipment, and returns to a shelter.
Verify that the entire sequence works correctly.

4. MULTIPLAYER STRESS TESTING
Perform comprehensive online multiplayer testing.
Test:

* Multiple simultaneous players.
* Dedicated servers.
* Player synchronization.
* Character movement.
* Weapon firing.
* Damage.
* AI enemies.
* Creature encounters.
* Boss battles.
* Item transfers.
* Crafting.
* Base construction.
* Revival.
* Character abilities.
* Player disconnections.
* Reconnection.
* Server restarts.
* Persistent player information.

Simulate:

* High latency.
* Packet loss.
* Temporary network interruptions.
* Sudden player disconnections.
* Multiple simultaneous combat events.

Measure:

* Ping.
* Server tick rate.
* Packet loss.
* Bandwidth.
* Server CPU.
* Server memory.
* Replication performance.

Fix synchronization problems.
Do not claim multiplayer readiness based only on local Play-In-Editor testing.

5. SECURITY AUDIT
Perform a complete security review.
Inspect:

* Google authentication.
* Account permissions.
* Session management.
* Player data.
* Inventory transactions.
* Item generation.
* Character powers.
* Weapon damage.
* Server authority.
* Anti-cheat.
* Database access.
* Owner privileges.
* Administrative commands.
* API keys.
* Authentication secrets.

Protect the game against:

* Unauthorized administrator access.
* Fake damage reports.
* Inventory manipulation.
* Unauthorized item generation.
* Privilege escalation.
* Session abuse.
* Common cheating techniques.
* Account impersonation.
* Unauthorized data access.

Verify that all owner powers are validated on the server.
Never expose credentials or secrets in packaged client files.
Implement audit logs and secure recovery procedures.

6. OWNER CONTROL CENTER FINALIZATION
Complete the original NULL-ACTION owner account.
The owner should have access to a professional administrative dashboard.
Include:

* Server monitoring.
* Player management.
* Player permissions.
* God Mode.
* Teleportation.
* Flight.
* Invincibility.
* Item spawning.
* Weapon spawning.
* Creature spawning.
* Weather control.
* Time control.
* World event management.
* Difficulty control.
* AI population control.
* Player moderation.
* Server configuration.
* Game statistics.
* Diagnostic tools.

Add secure authentication and permission verification.
Ensure ordinary administrators cannot silently acquire full owner privileges.
Every privileged action must be logged.

7. PERFORMANCE OPTIMIZATION — PC
Optimize the game for Windows PC.
Create performance configurations:
ULTRA:
Maximum supported visual quality.
HIGH:
Excellent graphics with balanced performance.
MEDIUM:
Balanced visual quality and performance.
LOW:
Reduced graphical complexity for weaker computers.
Optimize:

* Rendering.
* Lighting.
* Shadows.
* Nanite.
* Lumen.
* Textures.
* Materials.
* Vegetation.
* Characters.
* Animation.
* AI.
* Physics.
* Water.
* Particles.
* Audio.
* World streaming.
* Memory allocation.

Use Unreal Insights and appropriate profiling tools.
Measure CPU, GPU, RAM, VRAM, loading time, and frame time.
Do not optimize based on assumptions.
Use actual profiling results.

8. ANDROID OPTIMIZATION
Prepare a genuinely optimized Android version.
Do not simply package the PC game with reduced resolution.
Create a dedicated mobile configuration.
Optimize:

* Rendering.
* Materials.
* Textures.
* Lighting.
* Shadows.
* Vegetation.
* Animations.
* Particle effects.
* Water.
* Memory.
* Battery consumption.
* Thermal behavior.
* Touch controls.
* UI layouts.
* Network usage.

Create mobile graphical profiles.
Test on actual supported Android hardware.
Identify minimum supported hardware rather than claiming universal device compatibility.
Use current official Unreal Engine Android requirements and Google Play publishing requirements.
Verify Android SDK, NDK, JDK, target API, and package configuration before release.

9. PERFORMANCE TARGETS
Establish realistic performance budgets.
Suggested targets:
PC:

* 60 FPS target on the agreed reference hardware.
* Stable frame pacing.
* Reasonable memory usage.
* Acceptable loading times.

Android:

* 30 FPS baseline on agreed supported devices.
* 60 FPS where hardware permits.
* Stable thermal behavior.
* No excessive memory usage.

Multiplayer:

* Reliable synchronization.
* Consistent server performance.
* Stable network behavior.
* Reasonable bandwidth consumption.

Report actual measurements.
Never invent performance results.

10. SAVE SYSTEM AND DATA RECOVERY
Verify player persistence.
Players should retain:

* Accounts.
* Character appearance.
* Equipment.
* Inventory.
* Progression.
* Abilities.
* Achievements.
* Statistics.
* Preferences.
* Appropriate world progress.

Implement:

* Automated backups.
* Database recovery procedures.
* Versioned save data.
* Data migration.
* Corrupted-save handling.
* Server-side validation.

Test saving, loading, reconnecting, and recovering after server restarts.

11. CINEMATIC AND VISUAL FINAL POLISH
Perform a visual quality review.
Inspect:

* Main menu.
* Cinematic introduction.
* Character customization.
* Game HUD.
* Inventory.
* Map.
* Mission interface.
* Character animations.
* Creature animations.
* Weapons.
* Lighting.
* Environments.
* Weather.
* Underwater visuals.
* Effects.
* Audio.

Fix:

* Visual inconsistencies.
* Broken animations.
* Clipping.
* Incorrect shadows.
* Texture problems.
* Lighting problems.
* UI overlap.
* Incorrect scaling.
* Broken transitions.
* Audio issues.

Preserve the NULL-ACTION identity.

12. ACCESSIBILITY AND USABILITY
Perform a usability audit.
Test:

* First-time player onboarding.
* Tutorial.
* Navigation.
* Interface readability.
* Subtitle accessibility.
* Controls.
* Keyboard remapping.
* Controller support.
* Touch controls.
* Colorblind options.
* UI scaling.
* Audio controls.
* Camera settings.

Ensure the game is understandable without requiring players to read external documentation.

13. LEGAL AND ASSET LICENSE AUDIT
Review all external resources.
Create a complete asset and dependency register.
Check:

* 3D models.
* Characters.
* Animations.
* Textures.
* Music.
* Sound effects.
* Plugins.
* Libraries.
* Fonts.
* Third-party code.
* APIs.
* SDKs.

Verify licenses and commercial distribution permissions.
Remove unauthorized assets.
Document required attribution.
Review applicable privacy requirements for account creation, Google sign-in, player information, analytics, and communication features.
Create:

* Privacy policy draft.
* Terms of service draft.
* Community guidelines.
* Player reporting policy.
* Account deletion and data management procedures.

Flag anything requiring legal review.

14. BUILD AND PACKAGING
Create release-ready build configurations.
Windows PC
Prepare:

* Development build.
* Test build.
* Shipping build.
* Standalone executable.
* Required game assets.
* Configuration files.
* Server build where supported.

Verify installation and launch on a clean Windows environment.
Android
Prepare:

* Development APK for testing.
* Release APK where appropriate.
* Android App Bundle for Google Play distribution.
* Signing configuration.
* Package identifier.
* Versioning.
* Permissions.
* Mobile graphics configuration.

Keep signing keys and credentials secure.
Never commit private signing keys to source control.

15. DEDICATED SERVER DEPLOYMENT
Prepare production server infrastructure.
Implement and document:

* Dedicated server build.
* Server configuration.
* Startup procedure.
* Shutdown procedure.
* Authentication.
* Session management.
* Monitoring.
* Logging.
* Backups.
* Restart recovery.
* Deployment updates.
* Security configuration.

Evaluate suitable hosting providers.
Compare:

* Pricing.
* Server locations.
* CPU.
* RAM.
* Bandwidth.
* Scalability.
* Security.
* Operating system compatibility.

Do not purchase hosting or launch paid infrastructure without authorization.
Create a local production-like deployment first.

16. STORE AND RELEASE PREPARATION
Prepare NULL-ACTION for distribution.
Research current official requirements for:

* Steam.
* Google Play.
* Other appropriate distribution platforms.

Prepare:

* Game title.
* Game description.
* Logo.
* Cover artwork.
* Screenshots.
* Trailers.
* Feature list.
* System requirements.
* Supported platforms.
* Privacy documentation.
* Age-rating preparation.
* Installation instructions.

Ensure screenshots and promotional materials represent actual gameplay.
Do not advertise unimplemented features.
Do not publish the game without explicit owner authorization.

17. AUTOMATED TESTING
Create a professional testing system.
Implement:

* Unit tests.
* Gameplay functional tests.
* Multiplayer tests.
* Save/load tests.
* Performance tests.
* Content validation.
* Packaging tests.
* Regression tests.

Use Unreal Engine's Automation Testing Framework and suitable external tools.
Create repeatable testing procedures.
Every major update should be tested against previously working features.

18. FINAL RELEASE CANDIDATES
Prepare three distinct release candidates.
RC1 — Windows PC
A complete playable Windows build with functioning gameplay and verified systems.
RC2 — Android
An optimized Android build tested on actual supported hardware.
RC3 — Online Infrastructure
A functioning dedicated server configuration with verified real multiplayer connections.
Clearly document anything that prevents a release candidate from passing acceptance.

19. FINAL ACCEPTANCE TEST
NULL-ACTION must not be declared release-ready until:

1. The game launches correctly.
2. The introduction works.
3. Players can create or access accounts.
4. Character selection works.
5. Core gameplay works.
6. Combat works.
7. AI works.
8. Survival works.
9. Hunting works.
10. Multiplayer works across separate computers.
11. Player progression saves correctly.
12. Owner permissions are secure.
13. The game passes security checks.
14. PC packaging succeeds.
15. Android packaging succeeds where the target configuration supports it.
16. Performance is measured on reference hardware.
17. No unresolved critical bugs remain.
18. External assets have verified licensing.
19. Installation instructions are complete.
20. Backups and recovery procedures are tested.

20. FINAL DELIVERABLES
Create a professional release package containing:

* Complete Unreal Engine source project.
* Windows PC build.
* Android test build.
* Dedicated server build.
* Source code.
* Configuration files.
* Installation guide.
* Player guide.
* Server administration guide.
* Owner administration guide.
* Technical documentation.
* Asset license register.
* Security audit.
* Performance report.
* Multiplayer testing report.
* Bug tracking report.
* Release notes.
* Known limitations.
* Backup and recovery instructions.

Maintain:
NULL_ACTION_STATUS.md
Create additional documents:
NULL_ACTION_FINAL_AUDIT.md
NULL_ACTION_SECURITY_REPORT.md
NULL_ACTION_PERFORMANCE_REPORT.md
NULL_ACTION_MULTIPLAYER_REPORT.md
NULL_ACTION_RELEASE_CHECKLIST.md

EXECUTION RULES

1. Do not fake successful tests.
2. Do not claim an APK works without installing and testing it.
3. Do not claim multiplayer works without real clients connecting.
4. Do not hide unresolved problems.
5. Do not remove working features to make testing easier.
6. Do not publish without owner approval.
7. Do not spend money without owner approval.
8. Back up the project before major changes.
9. Maintain version control.
10. Fix critical errors before release.
11. Document anything that cannot be completed in the current environment.

FINAL DIRECTIVE
NULL-ACTION must be prepared as a real distributable game, not merely an Unreal Engine development project.
Prioritize stability, security, performance, functionality, and player experience.
Perform the work systematically.
Do not declare the game finished simply because the planned stages have been attempted.
The game is finished only when the actual acceptance tests pass.
BEGIN THE FINAL PRODUCTION AUDIT AND RELEASE PREPARATION.
