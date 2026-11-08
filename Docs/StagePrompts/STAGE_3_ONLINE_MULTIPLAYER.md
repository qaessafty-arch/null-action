NULL-ACTION — STAGE 3
REAL-TIME ONLINE MULTIPLAYER AND SERVER INFRASTRUCTURE
Continue developing the existing NULL-ACTION Unreal Engine project.
Your mission is to transform the existing playable game into a genuine online multiplayer experience.
This is a major infrastructure milestone.
Do not restart the project. Inspect the existing architecture, preserve working features, and extend the systems already implemented.
IMPORTANT: This must be REAL online multiplayer, not simulated multiplayer or multiple local characters pretending to be connected.

1. MULTIPLAYER ARCHITECTURE
Design and implement a professional multiplayer architecture.
Use:

* Unreal Engine networking.
* Dedicated authoritative servers.
* Epic Online Services where appropriate.
* Server-authoritative gameplay.
* Replicated player states.
* Secure server-side validation.

Research official documentation before selecting the final implementation.
The architecture must support future expansion to large multiplayer environments.
Do not assume that one server can handle unlimited players.
Create configurable player limits, server regions, and scalable deployment options.

2. PLAYER AUTHENTICATION
Implement secure player authentication.
Support:

* Account registration.
* Google sign-in.
* Player profiles.
* Unique player IDs.
* Session management.
* Account linking.
* Secure logout.
* Reconnection.

Use Google OAuth 2.0 / OpenID Connect or a compatible supported identity integration.
Never request Gmail inbox access when basic account authentication is sufficient.
Never store authentication secrets inside the game client.

3. MULTIPLAYER LOBBIES
Create a fully functional lobby system.
Players must be able to:

* Create a lobby.
* Join a lobby.
* Leave a lobby.
* Invite friends.
* Accept invitations.
* Set lobby privacy.
* Set maximum player count.
* View connected players.
* Select characters.
* Choose game modes.
* Start matches.
* Return to the lobby.

Create public and private lobbies.
Implement matchmaking and session discovery.

4. REAL-TIME PLAYER SYNCHRONIZATION
Synchronize all important gameplay states:

* Player movement.
* Rotation.
* Animations.
* Weapons.
* Shooting.
* Reloading.
* Health.
* Damage.
* Death.
* Revival.
* Character abilities.
* Inventory changes.
* Interactions.
* Vehicle and animal riding.

Players must see other players moving and interacting naturally.
Implement network prediction and server reconciliation where appropriate.
Use interpolation to reduce visual jitter.
Test under different network conditions.

5. MULTIPLAYER COMBAT
Make combat fully multiplayer compatible.
Requirements:

* Server-authoritative weapon firing.
* Server-side hit validation.
* Damage replication.
* Recoil and reload synchronization.
* Hit reactions.
* Headshot validation.
* Death synchronization.
* Weapon pickup.
* Ammunition synchronization.

Prevent clients from arbitrarily granting themselves damage, health, weapons, or ammunition.

6. COOPERATIVE GAMEPLAY
Create cooperative multiplayer activities.
Players must be able to:

* Explore together.
* Hunt creatures together.
* Fight enemy bots together.
* Collect resources.
* Craft items.
* Build shelters.
* Share selected resources.
* Revive teammates.
* Ride together where supported.
* Complete missions.
* Fight boss creatures.

Create a party system that keeps teammates connected during activities.

7. PVP AND PVE MODES
Implement configurable game modes:
PVE:

* Cooperative survival.
* Creature hunting.
* Boss battles.
* Exploration missions.
* Resource gathering.

PVP:

* Team deathmatch.
* Free-for-all.
* Custom private matches.
* Objective-based combat.

Create separate configuration rules for different game modes.
Ensure that character abilities and weapons remain balanced.

8. VOICE AND TEXT COMMUNICATION
Create multiplayer communication systems.
Implement:

* Text chat.
* Team chat.
* Private messages.
* Proximity chat.
* Voice channels where supported.
* Mute controls.
* Player reporting.
* Blocking.

Voice communication must require appropriate permissions and work with a supported voice solution.
Do not assume that a voice system is implemented merely because a microphone button exists.

9. AI IN MULTIPLAYER
Adapt existing AI to multiplayer.
Bots and creatures must:

* Be controlled by the server.
* Detect real players.
* Attack appropriate targets.
* Synchronize movement.
* Synchronize attacks.
* Maintain consistent health.
* Share combat states.
* Work correctly in cooperative missions.

Prevent multiple clients from independently controlling the same creature.
Create configurable bot populations for empty sessions.

10. SERVER INFRASTRUCTURE
Create a dedicated server configuration.
Implement:

* Server startup.
* Server shutdown.
* Session creation.
* Player connection.
* Player disconnection.
* Reconnection handling.
* Server health monitoring.
* Match management.
* Server logs.
* Error recovery.

Design support for different geographical regions.
Research realistic hosting options, costs, and deployment requirements.
Do not purchase hosting or create paid services without authorization.
Provide local dedicated-server testing first.

11. PERSISTENT PLAYER DATA
Create secure persistent storage for:

* Account identity.
* Character selection.
* Character progression.
* Inventory.
* Unlocked abilities.
* Achievements.
* Statistics.
* Friends.
* Settings.

Validate important changes on the server.
Protect player data from unauthorized modification.
Create backup and recovery procedures.

12. MULTIPLAYER SECURITY
Implement:

* Server-side permission validation.
* Session validation.
* Secure authentication.
* Input validation.
* Inventory validation.
* Movement sanity checks.
* Damage validation.
* Administrative permission checks.
* Basic exploit detection.
* Logging.

Use server-authoritative architecture.
Protect the owner account and administrative commands.

13. OWNER MULTIPLAYER CONTROL
Integrate the future NULL-ACTION Owner Control Center.
The owner must eventually be able to:

* Inspect active servers.
* View connected players.
* Manage sessions.
* Monitor player activity.
* Control game events.
* Manage bots.
* Spawn creatures.
* Manage player permissions.
* Kick or ban abusive players.
* Configure server settings.

All owner commands must be authenticated and verified by the server.
Maintain audit logs.

14. NETWORK TESTING
Create automated and manual multiplayer tests.
Test using:

* Two separate game clients.
* Multiple local clients where supported.
* A dedicated server.
* Different network conditions.
* High latency.
* Packet loss.
* Player disconnections.
* Reconnection.
* Simultaneous combat.
* Multiple AI enemies.

Verify that all players see consistent game states.
Measure:

* Ping.
* Packet loss.
* Server tick rate.
* Bandwidth.
* Replication load.
* CPU usage.
* Memory usage.

Fix synchronization problems before expanding the player count.

15. DELIVERABLES
Provide:

* Working multiplayer implementation.
* Dedicated server build.
* Client build.
* Authentication integration.
* Functional lobby.
* Multiplayer combat.
* Cooperative gameplay.
* Player synchronization.
* Persistent player data.
* Updated source code.
* Updated documentation.
* Network testing results.
* Server setup instructions.
* Known limitations.

Update NULL_ACTION_STATUS.md.
For every feature, clearly distinguish between implemented, tested, and verified.

FINAL ACCEPTANCE TEST
This stage is not complete until:

1. Two separate computers can run the game.
2. Both players can connect to the same dedicated server.
3. Both can see each other.
4. Both can move independently.
5. Both can shoot and deal synchronized damage.
6. Both can fight the same AI enemies.
7. Both can interact with the same environment.
8. Both can disconnect and reconnect appropriately.
9. Player information is synchronized correctly.
10. Unauthorized clients cannot grant themselves owner permissions.

Do not claim success based on a local simulation.
START DEVELOPMENT NOW.
