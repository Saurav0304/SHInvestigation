# SH Investigation

My submission for the Silver Haze Studios Unreal Engine Developer assessment. It's a small 2-player co-op investigation prototype made in Unreal Engine 5.7 with C++ and Blueprints.

## The game

One player hosts, the other joins. Room 1 is open and has a torn letter and a power switch. Room 2 is locked and dark. Turning the power on lights up Room 2 and unlocks its doors (this is the gameplay event). Inside is the second clue, a bloodstained knife. Once both clues are found, both players see "case solved".

Controls: WASD and mouse to move, Space to jump, E to interact, Tab for the evidence journal.

## Running it

In the editor: open the project (it starts in `Lvl_Investigation`), set Number of Players to 2 and Net Mode to Play As Listen Server, then press Play.

As a real session: right click `SHInvestigation.uproject` and Launch game twice. Click Host Game in one window and Join Game (LAN) in the other. You can also skip the menu with `-autohost` and `-autojoin`:

UnrealEditor.exe "<path>\SHInvestigation.uproject" -game -windowed -ResX=1280 -ResY=720 -autohost
UnrealEditor.exe "<path>\SHInvestigation.uproject" -game -windowed -ResX=1280 -ResY=720 -autojoin

## Code layout

Source/SHInvestigation/
Interaction/    IInteractable, UInteractionComponent, AInteractableBase
Interactables/  ADoorInteractable, ALightSwitchInteractable
Investigation/  UEvidenceData, AEvidenceActor
Core/           AInvestigationGameState, UInvestigationGameInstance
Menu/           AMainMenuGameMode, AMainMenuPlayerController
UI/             UInteractionPromptWidget, UInvestigationHUDWidget, UMainMenuWidget

The character is the Third Person template character, as the brief allowed. The interaction, doors, switch, evidence, journal and sessions are my own systems.

## Interaction

`IInteractable` is an interface (Interact, CanInteract, GetInteractionPrompt). `UInteractionComponent` lives on the character, and `AInteractableBase` is the replicated base actor for everything you can use.

The local player traces from the camera every 0.1s on a custom "Interact" channel and shows a prompt like "[E] Open Door". Pressing E sends `Server_Interact` from the component. The server checks the target again (is it interactable, CanInteract, in range) and calls Interact. The object changes its replicated state and the clients update in OnRep.

Why this way:
- The RPC is on the player's component, not on the door. A client can only send server RPCs through actors it owns (its controller and pawn), so an RPC on the door would be dropped.
- The server never trusts the client, so you can't open things from far away or open a locked door.
- The trace runs only for the local player and on a timer, not on Tick.
- Walls ignore the Interact channel, so a second Visibility trace stops you interacting through walls.

## Doors, switch and the gameplay event

The door is a hinged leaf. Open and locked are replicated values, two leaves can be paired into a double door, and it opens away from whoever used it. The power switch turns its lights on and off, and unlocks its doors when the power is on.

The server only changes state. Each machine plays the door swing itself in OnRep, so movement isn't replicated and late joiners just snap to the right position. I used RepNotify instead of Multicast for all state, because a player joining later misses old multicasts but always gets current values. Since C++ OnReps don't run on the server, the server calls them by hand so the host sees the change too. The lights aren't replicated actors; each machine switches its own based on the switch state.

## Evidence and journal

Each clue is a data asset (`UEvidenceData`: name, description, icon). `AEvidenceActor` is the clue in the world. When you pick it up, the server adds it to `AInvestigationGameState` and hides the actor for everyone.

The journal lives in the GameState because it's shared team progress and the GameState exists on every machine. Only the server writes to it and duplicates are refused, so a clue can't be collected twice. The total is counted from the clues placed in the level, so adding a clue doesn't need code. The HUD only listens to GameState events. Inspecting is local only, since it doesn't change game state. The finder sees the inspect panel and the other player gets "Your partner found: ...".

## Sessions

`UInvestigationGameInstance` uses the Online Subsystem with the Null (LAN) platform. Host creates a 2-player session and opens the level as a listen server. Join searches, joins the first session of this game with a free slot, and travels to the host. It lives in the GameInstance because that survives level changes. The GameMode turns away a third player, and a dropped connection sends you back to the menu with the reason.

## UI and Blueprints

The journal (Tab) shows one card per clue with its picture, name, who found it and the description, plus "Undiscovered" cards for clues still missing. The widgets build their layout in C++, so nothing extra is needed to run. Their parts are optional bindings, so a Widget Blueprint with the same names can restyle them. All logic and networking is in C++. Blueprints and assets hold the content: meshes, materials, prompt text, clue data, and the level setup for linked doors, locks and lights.

## Limitations

- LAN only for now. Online play would need Steam or EOS.
- No client-side prediction, so actions show after one round trip (not noticeable on LAN).
- No pause or leave menu yet.
- No sounds or animations on interactions, but the Blueprint events for them are ready.
- The lever is a single mesh, so its handle doesn't move. A small red/green light on the switch shows the power state instead.

## Art

The level uses free Fab assets (wooden door, lever, tactical knife, paper note, castle wall and grass) with evening lighting. The evidence pictures in the journal are simple images I added in `Content/Inventory/SampleImages`.