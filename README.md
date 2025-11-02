# MO56

Developed with Unreal Engine 5.6.

## Editor Implementation Guides

The following sections summarize how to stand up each gameplay system, component, and widget inside the Unreal Editor. Every entry mirrors the inline C++ documentation and provides quick links to authoritative Unreal Engine resources and related external guides.

### Gameplay Core

#### `AMO56GameMode`
1. Create a Blueprint subclass and assign it under **Project Settings → Maps & Modes → Default Modes**.
2. Set `DefaultPawnClass` to your `AMO56Character` Blueprint and `PlayerControllerClass` to the MO56 controller Blueprint.
3. Configure HUD/GameState overrides if you provide Blueprint implementations of those classes.
4. Use Blueprint events such as `BeginPlay` or `OnPostLogin` to initialize crafting, saving, or UI managers.
5. Pair the Blueprint with **World Settings** overrides so server travel loads consistent defaults.
   * Resources: [GameMode and GameState](https://docs.unrealengine.com/5.3/en-US/gamemode-and-game-state-in-unreal-engine/)

#### `AMO56PlayerController`
1. Derive a Blueprint and assign it as the project default player controller.
2. Populate `DefaultMappingContexts` with Enhanced Input assets and configure `MobileExcludedMappingContexts` for touch builds.
3. Provide a `MobileControlsWidgetClass` to spawn touch controls when running on mobile platforms.
4. Drive Blueprint menus and save prompts through the `Request*` functions so server RPCs execute correctly.
5. Surface the replicated `PlayerSaveId` through your UI to identify save slots per controller.
   * Resources: [Player Controller Overview](https://docs.unrealengine.com/5.3/en-US/player-controller-in-unreal-engine/), [Enhanced Input](https://docs.unrealengine.com/5.3/en-US/enhanced-input-in-unreal-engine/)

#### `AMO56Character`
1. Create a Blueprint subclass (e.g., `BP_PlayerCharacter`) and set it as the default pawn in the GameMode.
2. Adjust the `CameraBoom` length, socket offsets, and `FollowCamera` FOV to achieve the desired third-person framing.
3. Assign Enhanced Input assets to the `Input|Actions` fields, then add the mapping context during `BeginPlay`.
4. Provide widget classes (`HUDWidgetClass`, `InventoryWidgetClass`, `CharacterStatusWidgetClass`, `CharacterSkillMenuClass`) to wire UI.
5. Implement Blueprint logic for menu toggles (`OnToggleInventory`, `OnToggleSkillMenu`, etc.) and inventory replication events.
   * Resources: [Third-Person Character Template](https://docs.unrealengine.com/5.3/en-US/third-person-blueprint-game-template-in-unreal-engine/)

#### `AMyActor`
1. Derive a Blueprint whenever you need a lightweight scripted prop or trigger volume.
2. Add Static Mesh, Particle System, or collision components in the Blueprint to visualize the actor.
3. Override `BeginPlay`/`Tick` in Blueprint for timeline-driven behavior without touching C++.
4. Expose BlueprintCallable helpers if the actor must communicate with inventories, skills, or crafting systems.
5. Place instances in a level and tune exposed variables per-instance in the Details panel.
   * Resources: [Creating Actors](https://docs.unrealengine.com/5.3/en-US/actors-in-unreal-engine/)

### Building & Crafting

#### `ABuildSiteActor`
1. Create a Blueprint subclass (e.g., `BP_BuildSiteActor`) to author per-recipe visuals.
2. Assign a hologram mesh to `BlueprintMesh` and tune the material parameter defined by `BlueprintFillParameter`.
3. Adjust `InteractionSphere` radius and collision channels to match the desired interaction distance.
4. Set `BuildableActorClass` and `BuildMaterialRequirements` on your `UCraftingRecipe` assets so initialization succeeds.
5. Bind `OnBuildProgress` and `OnBuildCompleted` in Blueprint for VFX, SFX, and UI broadcasts.
   * Resources: [Actor Replication](https://docs.unrealengine.com/5.3/en-US/actors-and-networking-in-unreal-engine/)

#### `ABuildGhostActor`
1. Author a Blueprint derivative per build recipe to supply unique hologram materials.
2. Apply a translucent material instance that exposes a scalar named by `PlacementValidParameter`.
3. Call `SetGhostMesh`/`SetGhostMaterial` from build mode Blueprint logic to update visuals per recipe selection.
4. Use `TestPlacementAtTransform` before confirming placement to gate accept/deny messaging.
5. Extend `UpdateGhostMaterialState` in Blueprint for custom glow, audio, or Niagara indicators.
   * Resources: [Collision Queries](https://docs.unrealengine.com/5.3/en-US/collision-in-unreal-engine/)

#### `UCraftingRecipe`
1. Create a Blueprint Data Asset of type `CraftingRecipe` in the Content Browser.
2. Set `DisplayName`, `Icon`, and IO maps (`Inputs`, `Outputs`, `FailByproducts`) to reference your item IDs.
3. Configure gating fields (`RequiredKnowledge`, `PrimarySkillTag`) and tune `BaseDuration`/`BaseDifficulty`.
4. Enable `bIsBuildable` and assign `BuildableActorClass` plus `BuildMaterialRequirements` for placeable structures.
5. Reference the asset from crafting menus or `ABuildSiteActor::InitializeFromRecipe` to drive runtime behavior.
   * Resources: [Data Assets](https://docs.unrealengine.com/5.3/en-US/data-assets-in-unreal-engine/)

#### `UCraftingSystemComponent`
1. Add the component to your character Blueprint alongside inventory and skill components.
2. Populate `InitialRecipes` and `InitialBuildRecipes` with `UCraftingRecipe` assets granted at spawn.
3. Bind the crafting delegates to UI widgets for timers, progress bars, and success/failure messaging.
4. Drive crafting attempts via Blueprint calls to `TryCraftRecipe` and `CancelCraft` from menus or hotkeys.
5. Persist unlocked recipes by calling `WriteToSaveData`/`ReadFromSaveData` in the save subsystem.
   * Resources: [Actor Components](https://docs.unrealengine.com/5.3/en-US/creating-actor-components-in-unreal-engine/)

### Survival Systems

#### `UCharacterStatusComponent`
1. Attach the component to your player Blueprint to replicate vitals on both server and client.
2. Tune the `Status|Energy`, `Status|Hydration`, and `Status|Config` defaults to match your survival balance.
3. Bind `OnVitalsUpdated` to your status widget so UI refreshes when values change.
4. Feed gameplay inputs (`Speed`, `AmbientTemp`, `LoadKg`) from animation or environment managers in Blueprint.
5. Call `ConsumeFood`, `DrinkWater`, and `ApplyInjury` from interactions, then serialize values through your save system.
   * Resources: [Gameplay Attribute Systems](https://docs.unrealengine.com/5.3/en-US/gameplay-ability-system-component-in-unreal-engine/) *(conceptual reference for stat management)*

#### `USkillSystemComponent`
1. Add the component to characters that require skill or knowledge tracking and enable replication.
2. Populate your skill/knowledge catalogs via data tables referenced by UI widgets and the component's getters.
3. Bind `OnSkillStateChanged` and `OnInspectionStateChanged` to menus or HUD overlays in Blueprint.
4. Start inspections or grant XP through Blueprint-exposed methods when players interact with items or world actors.
5. Persist progress using `WriteToSaveData`/`ReadFromSaveData` via `UMO56SaveSubsystem`.
   * Resources: [Creating Actor Components](https://docs.unrealengine.com/5.3/en-US/creating-actor-components-in-unreal-engine/)

#### `UInspectableComponent`
1. Attach the component to Blueprint actors that players can study.
2. Fill the `Discoveries` array with knowledge and skill tags plus descriptive text for UI prompts.
3. Use `bOncePerActor` for unique discoveries to prevent repeat rewards.
4. Build inspection parameters in Blueprint and feed them into `UWorldActorContextMenuWidget`.
5. Call `BuildInspectionParams` when assembling context menus or countdown widgets for inspections.
   * Resources: [Blueprint Components](https://docs.unrealengine.com/5.3/en-US/components-overview-in-unreal-engine/)

### Save & Persistence

#### `UMO56SaveSubsystem`
1. Register the subsystem on your GameInstance so it initializes automatically.
2. Route menu commands to `SaveGame`, `LoadGame`, `SetActiveSaveSlot`, and `CreateNewSaveSlot` from Blueprint UI.
3. Register inventory and skill components in `BeginPlay` to capture their state for persistence.
4. Pair controller/character Blueprints with `NotifyPlayerControllerReady` and `RegisterPlayerCharacter` hooks after possession.
5. Extend the subsystem when introducing additional persistent actors (buildings, quests, etc.).
   * Resources: [Subsystems](https://docs.unrealengine.com/5.3/en-US/subsystems-in-unreal-engine/), [Saving and Loading Your Game](https://docs.unrealengine.com/5.3/en-US/saving-and-loading-your-game-in-unreal-engine/)

#### `UMO56SaveGame`
1. Extend the SaveGame Blueprint when adding new serialized fields to MO56.
2. Update `UMO56SaveSubsystem` read/write helpers whenever you introduce new properties.
3. Expose Blueprint functions to inspect `InventoryStates`, `PlayerStates`, or metadata for menu displays.
4. Create sample SaveGame assets in the Content Browser to debug serialized data in PIE.
5. Maintain backward compatibility by providing defaults for newly added fields.
   * Resources: [Save Game System](https://docs.unrealengine.com/5.3/en-US/saving-and-loading-your-game-in-unreal-engine/)

### User Interface

#### `UHUDWidget`
1. Build a Blueprint subclass and bind the panel containers (status, skills, inventories, mini-map, menu).
2. Add overlay widgets in the designer to match the container layout described by the C++ properties.
3. Spawn the HUD in your PlayerController Blueprint and add it to the viewport.
4. Configure skill menus and countdown overlays via `ConfigureSkillMenu` and `ConfigureInspectionCountdown` before use.
5. Use helper methods (`AddLeftInventoryWidget`, `ToggleSkillMenu`, etc.) to manage runtime widgets from Blueprint.
   * Resources: [UMG Designer](https://docs.unrealengine.com/5.3/en-US/using-umg-ui-designer-in-unreal-engine/)

#### `UCharacterStatusWidget`
1. Create a Blueprint and bind the progress bars/text blocks defined by the C++ properties.
2. Style each bound widget to match your HUD theme.
3. Call `SetStatusComponent` and `SetSkillSystemComponent` after creating the widget.
4. Bind `RefreshFromStatus` to `UCharacterStatusComponent::OnVitalsUpdated` to update values in real time.
5. Override `RefreshSkillSummaries` in Blueprint to add additional formatting or tooltips.
   * Resources: [UMG Data Binding](https://docs.unrealengine.com/5.3/en-US/umg-binding-in-unreal-engine/)

#### `UCharacterSkillMenu`
1. Derive a Blueprint and bind `KnowledgeList`, `SkillList`, and info widgets through BindWidget.
2. Provide a `SkillEntryWidgetClass` that extends `USkillListEntryWidget` for list entries.
3. Call `InitWithSkills` when opening the menu to register the owning `USkillSystemComponent`.
4. Wire `SetSkillSystemComponent` and delegate bindings so the menu refreshes on skill changes.
5. Hook menu toggles (e.g., from the HUD) to `SetSkillMenuVisibility` for user access.
   * Resources: [Widget Blueprints](https://docs.unrealengine.com/5.3/en-US/widget-blueprints-in-unreal-engine/)

#### `USkillListEntryWidget`
1. Build a Blueprint for list row styling and bind `SkillIcon`, `SkillName`, `SkillRank`, and `SkillInfoButton` widgets.
2. Style hover/selected states via Slate brushes or animations.
3. Relay `OnInfoRequested` in Blueprint to show lore or progression tips.
4. Assign the Blueprint to `UCharacterSkillMenu::SkillEntryWidgetClass`.
5. Customize display logic in Blueprint if additional formatting is required.
   * Resources: [List Views in UMG](https://docs.unrealengine.com/5.3/en-US/list-view-widget-in-unreal-engine/)

#### `UInventoryWidget`
1. Create a Blueprint subclass with a `UUniformGridPanel` bound to `SlotsContainer`.
2. Assign `SlotWidgetClass` to your `UInventorySlotWidget` Blueprint for slot instantiation.
3. Bind the weight/volume text blocks to display aggregated stats from `UpdateInventoryStats`.
4. Enable `bAutoBindToOwningPawn` for HUD usage so it tracks the player's inventory automatically.
5. Implement or extend `OnUpdateInventory` for custom refresh animations or sorting.
   * Resources: [UMG Layout](https://docs.unrealengine.com/5.3/en-US/umg-layout-and-anchors-in-unreal-engine/)

#### `UInventorySlotWidget`
1. Design a Blueprint slot layout with an icon, quantity badge, and optional overlays.
2. Bind `ItemIcon`, `QuantityText`, and `QuantityBadge` to your designer widgets.
3. Call `InitializeSlot` from the owning inventory grid to connect inventory data.
4. Expose slot actions (split, drop, destroy, inspect) through buttons or radial menus wired to the C++ helpers.
5. Register the widget with `IInventoryUpdateInterface` so `SetItemStack` runs on inventory updates.
   * Resources: [Drag and Drop in UMG](https://docs.unrealengine.com/5.3/en-US/drag-and-drop-in-umg/)

#### `UInventorySlotMenuWidget`
1. Construct a Blueprint context menu with buttons for split, drop, destroy, and inspect actions.
2. Call `SetOwningSlot` immediately after spawning the menu to provide slot context.
3. Bind button events to the `Handle*` functions or override them in Blueprint for custom handling.
4. Use `DismissMenu` to play an outro animation before removing the widget.
5. Assign the Blueprint to `UInventorySlotWidget::ContextMenuClass` for automatic spawning.
   * Resources: [Context Menus in UMG](https://docs.unrealengine.com/5.3/en-US/umg-widget-interaction-in-unreal-engine/)

#### `UInventorySlotDragVisual`
1. Create a Blueprint for the drag preview and bind the `ItemIcon` image.
2. Optionally add quantity text or rarity borders to the preview.
3. Update visuals via `SetDraggedStack` when the drag operation starts.
4. Assign the Blueprint to `UInventorySlotWidget::DragVisualClass`.
5. Verify the widget follows the cursor during PIE drag-and-drop tests.
   * Resources: [Drag and Drop in UMG](https://docs.unrealengine.com/5.3/en-US/drag-and-drop-in-umg/)

#### `UInventorySlotDragOperation`
1. Use this class as the `DragDropOperation` when starting inventory drags.
2. Call `InitializeOperation` to capture the source inventory, slot index, and stack.
3. Cast the operation in drop targets to retrieve payload data.
4. Pair with `UInventorySlotDragVisual` for the cursor preview.
5. Extend the Blueprint if you need to carry additional metadata like permissions or container references.
   * Resources: [Drag and Drop in UMG](https://docs.unrealengine.com/5.3/en-US/drag-and-drop-in-umg/)

#### `UInventoryUpdateInterface`
1. Implement the interface in any UMG widget that should refresh when inventory data changes.
2. Override `OnUpdateInventory` in Blueprint to rebuild slot widgets or stats.
3. Register widgets with `UInventoryComponent::OnInventoryChanged` so updates trigger automatically.
4. Combine with `UInventoryWidget` or custom grids to keep UI synchronized with replicated inventories.
5. Remove bindings in `Destruct` to prevent dangling references.
   * Resources: [Blueprint Interfaces](https://docs.unrealengine.com/5.3/en-US/blueprint-interfaces-in-unreal-engine/)

#### `UWorldActorContextMenuWidget`
1. Design a Blueprint layout (vertical list or radial menu) for context-sensitive actions.
2. Populate actions via `InitializeMenu` using inspection data and additional callbacks.
3. Bind `OnMenuDismissed` to resume player input or hide overlays.
4. Implement `DismissMenu` in Blueprint to play exit animations before removing the widget.
5. Integrate with `USkillSystemComponent` to expose inspection discoveries.
   * Resources: [Widget Interaction](https://docs.unrealengine.com/5.3/en-US/umg-widget-interaction-in-unreal-engine/)

#### `UGameMenuWidget`
1. Build a Blueprint with buttons bound to `NewGameButton`, `LoadGameButton`, etc.
2. Provide a `FocusContainer` to host nested widgets like the save slot browser.
3. Assign `SaveGameMenuClass` to your `USaveGameMenuWidget` Blueprint.
4. Call `SetSaveSubsystem` after creation to link persistence functionality.
5. Extend `Handle*Clicked` events in Blueprint for confirmation dialogs or transitions.
   * Resources: [Menus in UMG](https://docs.unrealengine.com/5.3/en-US/creating-menus-in-unreal-engine/)

#### `USaveGameMenuWidget`
1. Derive a Blueprint and add a `UScrollBox` bound to `SaveList`.
2. Set `SaveEntryWidgetClass` to your `USaveGameDataWidget` Blueprint.
3. Wire `SetSaveSubsystem` when the menu opens so it can query summaries.
4. Call `RefreshSaveEntries` after the subsystem reports new data.
5. Bind `OnSaveLoaded` to close the menu or update the HUD when a load completes.
   * Resources: [Save Game UI Patterns](https://docs.unrealengine.com/5.3/en-US/saving-and-loading-your-game-in-unreal-engine/)

#### `USaveGameDataWidget`
1. Create a Blueprint card layout with text blocks bound to slot metadata.
2. Bind the `LoadButton` to trigger `OnLoadRequested`.
3. Feed `FSaveGameSummary` data into `SetSummary` when populating the save list.
4. Format timestamps locally or override `FormatDateTime` in Blueprint for localization.
5. Listen to `OnLoadRequested` in your menu to initiate `UMO56SaveSubsystem::LoadGameBySlot`.
   * Resources: [UMG Buttons](https://docs.unrealengine.com/5.3/en-US/umg-button-widget-in-unreal-engine/)

#### `UInspectionCountdownWidget`
1. Build a Blueprint overlay with bound text and progress bar widgets.
2. Register the class via `UHUDWidget::ConfigureInspectionCountdown` before use.
3. Call `ShowCountdown`, `UpdateCountdown`, and `HideCountdown` from Blueprint to manage visibility.
4. Hook `OnCancelRequested` to stop inspections through `USkillSystemComponent::CancelCurrentInspection`.
5. Extend Blueprint logic for animations or audio cues around countdown events.
   * Resources: [Progress Bars in UMG](https://docs.unrealengine.com/5.3/en-US/umg-progress-bar-widget-in-unreal-engine/)

#### `UActionCountdownWidget`
1. Derive a Blueprint if you need alternate styling for non-inspection timers.
2. Reuse the countdown API from `UInspectionCountdownWidget` for generic timed actions.
3. Register the widget with the HUD when substituting different countdown visuals.
4. Trigger countdown updates from gameplay Blueprints just like inspection timers.
5. Listen to `OnCancelRequested` to abort the underlying action when players dismiss the overlay.
   * Resources: [Progress Bars in UMG](https://docs.unrealengine.com/5.3/en-US/umg-progress-bar-widget-in-unreal-engine/)

### Additional References
- [Unreal Engine Documentation](https://docs.unrealengine.com/5.3/en-US/)
- [Epic Dev Community Tutorials](https://dev.epicgames.com/community/unreal-engine)
- [Unreal Online Learning](https://www.unrealengine.com/en-US/onlinelearning-courses)
