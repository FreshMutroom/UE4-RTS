// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSPlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Public/Blueprint/UserWidget.h"
#include "Classes/Kismet/KismetMathLibrary.h"
#include "Public/Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/HUD.h"
#include "Components/DecalComponent.h"
#include "Public/TimerManager.h"
#include "EngineUtils.h"
#include "UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/InputSettings.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Slate/SceneViewport.h"

#include "Miscellaneous/PlayerCamera.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"
#include "UI/MarqueeHUD.h"
#include "MapElements/GhostBuilding.h"
#include "Managers/FogOfWarManager.h"
#include "Managers/ObjectPoolingManager.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSGameMode.h"
#include "GameFramework/Selectable.h"
#include "Statics/Statics.h"
#include "GameFramework/FactionInfo.h"
#include "MapElements/Building.h"
#include "UI/MainMenu/Menus.h"
#include "Settings/DevelopmentSettings.h"
#include "UI/InGameWidgetBase.h"
#include "UI/RTSHUD.h"
#include "MapElements/Invisible/RTSLevelVolume.h"
#include "Settings/ProjectSettings.h"
#include "UI/MainMenu/GameMessageWidget.h"
#include "MapElements/Invisible/PlayerCamera/PlayerCameraMovement.h"
#include "Statics/DamageTypes.h"
#include "UI/InMatchDeveloperWidget.h"
#include "UI/InMatchWidgets/Components/MyButton.h"
#include "UI/InMatchWidgets/CommanderSkillTreeNodeWidget.h"
#include "UI/InMatchWidgets/GlobalSkillsPanelButton.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "MapElements/Infantry.h"
#include "MapElements/ResourceSpot.h"
#include "MapElements/InventoryItem.h"
#include "Managers/UpgradeManager.h"
#include "UI/InMatchWidgets/PauseMenu.h"
#include "UI/InMatchWidgets/InMatchConfirmationWidget.h"
#include "Settings/KeyMappings.h"
#include "UI/MainMenuAndInMatch/KeyMappingMenu.h"
#include "UI/MainMenuAndInMatch/AudioSettingsMenu.h"
#include "UI/MainMenuAndInMatch/ControlSettingsMenu.h"
#include "UI/MainMenuAndInMatch/SettingsMenu.h"
#include "UI/MainMenuAndInMatch/VideoSettingsMenu.h"


/* --------------------------------
*	*FCtrlGroupArray*
*  --------------------------------
*/

TArray < AActor * > & FCtrlGroupList::GetArray()
{
	return List;
}

int32 FCtrlGroupList::GetNum() const
{
	return List.Num();
}


/* --------------------------------
*	*FTryBindActionResult*
*  --------------------------------
*/

FTryBindActionResult::FTryBindActionResult(const FKeyWithModifiers & InKey)
	: KeyWeAreTryingToAssign(InKey)
	, AlreadyBoundToKey_Axis(EKeyMappingAxis::None)
	, AlreadyBoundToKey_Action
							{
								EKeyMappingAction::None,
								EKeyMappingAction::None,
								EKeyMappingAction::None,
								EKeyMappingAction::None,
								EKeyMappingAction::None,
								EKeyMappingAction::None,
								EKeyMappingAction::None,
								EKeyMappingAction::None
							}
	, Warning(EGameWarning::None)
{
}


//=============================================================================================
//	RTS Player Controller Implementation
//=============================================================================================

ARTSPlayerController::ARTSPlayerController()
{
	/* Default values */

	// Ticking is enabled when possessing a camera
	PrimaryActorTick.bCanEverTick = true;
	/* Ticking will be enabled during Client_SetupForMatch */
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Set default values

	MaxLineTraceDistance = 5000.f;

	CurrentSelected = nullptr;
	GhostBuilding = nullptr;
	MouseDecal = nullptr;
	MouseDecalHeight = 128.f;

	PendingContextAction = EContextButton::None;

	PendingKeyRebind_Action = EKeyMappingAction::None;
	PendingKeyRebind_Axis = EKeyMappingAxis::None;
	PendingKeyRebind_PressedModifierFlags_Left = EKeyModifiers::None;
	PendingKeyRebind_PressedModifierFlags_Right = EKeyModifiers::None;
	PendingKeyRebind_TimeSpentTryingToCancel = 1.f;

	LastSelectControlGroupButtonReleased = -1;
	LastSelectControlGroupButtonReleaseTime = 0.f;

	GhostRotationDirection = ERotationDirection::NoDirectionEstablished;
}

void ARTSPlayerController::SetPlayer(UPlayer * InPlayer)
{
	Super::SetPlayer(InPlayer);
}

void ARTSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->GetFirstPlayerController() == this)
	{
		GI = CastChecked<URTSGameInstance>(GetGameInstance());
		
		CameraMouseWheelZoomCurve = GI->GetCameraMouseWheelZoomCurve();
		ResetCameraCurve = GI->GetCameraResetCurve();

		SetupCameraCurves();

		InitCtrlGroups();
	}
}

void ARTSPlayerController::Tick(float DeltaTime)
{
	/* Controller starts with tick disabled and is only activated when on local machine, so
	tick should only ever run for local player controllers */
	assert(IsLocalController());

	Super::Tick(DeltaTime);

	/* Wanted this as a timer handle instead but they don't run while game is paused */
	if (PendingKeyRebind_TimeSpentTryingToCancel >= 0.f)
	{
		PendingKeyRebind_TimeSpentTryingToCancel += DeltaTime;
		if (PendingKeyRebind_TimeSpentTryingToCancel > 0.8f)
		{
			OnCancelPendingKeyRebindButtonHeld();
		}
	}

	if (GetWorld()->IsPaused())
	{
		/* If game is paused then stop here. Note this function seems like it does quite a lot. 
		Consider creating my own bIsPaused variable and using that instead */
		return;
	}

	/* TODO: check if selected is still selectable. Just Selected[0] should be enough to check
	since cant marquee hostiles e.g. enemy building is now in fog or enemy unit is now in fog,
	and if they're no longer able to be selected then deselect them */

	/* Set this to the default cursor. It will be assigned values later on based on whether 
	the mouse is at edge of screen or hovering a UI element */
	ScreenLocationCurrentCursor = DefaultCursor;
	/* The cursor that will be shown this tick */
	const FCursorInfo * CursorToShow = ScreenLocationCurrentCursor;

	// Do any edge scrolling
	MoveIfAtEdgeOfScreen(DeltaTime, CursorToShow);
	 
	//-------------------------------------------------------------------------------
	// Setting camera rotation to target rotation

	float ResetCurveValue;

	if (ResetCameraCurve != nullptr && (bIsResettingCameraRotation || bIsResettingCameraZoom))
	{
		// Might need to clamp this to curve's max X axis value. Actually not needed I think
		ResetCameraCurveAccumulatedTime += DeltaTime * ResetCameraToDefaultRate;

		// Will need this value for two lerps later on
		ResetCurveValue = ResetCameraCurve->GetFloatValue(ResetCameraCurveAccumulatedTime);
	}

	/* After Super rotation input would have been applied. If MMB is pressed then make sure
	that we do not try to reset to default camera rot by setting target to current.
	The !bIsResettingCameraRotation is important to stop MMB look-around while camera is
	resetting rotation */
	if (bIsCameraFreeLookEnabled && !bIsResettingCameraRotation)
	{
		TargetResetRotation = ControlRotation;
	}

	// TODO using same equality func as in SetControlRotation but check if == operator works
	if (!ControlRotation.Equals(TargetResetRotation, 1e-3f))
	{
		// Need to head towards target rotation

		FRotator NewRot = TargetResetRotation;

		// Get value from timeline if set and valid rate
		if (ResetCameraToDefaultRate > 0.f && ResetCameraCurve != nullptr)
		{
			NewRot = FMath::Lerp<FRotator, float>(StartResetRotation, TargetResetRotation,
				ResetCurveValue);
		}

		SetControlRotation(NewRot);
	}
	/* TODO Currently resetting camera rotation when already at default rot still has to wait
	for timeline duration to complete even though camera is already at desired rot. Could
	change the logic to do SetControlRottion first, and then check if
	ControlRotation.Equals(TargetResetRotation). Have to consider zoom too though... If both
	rot and zoom are at desired from teh start then do not lock camera while timeline
	completes. Maybe could only set bools bIsResettingCameraRotation and
	bIsResettingCameraZOom when input is done only if zoom and rot and not already at default.
	UNrelated Scheduale: also can do camera edge of screen movement */


	//-------------------------------------------------------------------------------
	// Setting camera zoom amount to target zoom 

	/* Do same for camera zoom */

	if (bIsResettingCameraZoom)
	{
		// Zoom target set by pressing reset button

		if (ResetCameraToDefaultRate > 0.f && ResetCameraCurve != nullptr)
		{
			SetSpringArmTargetArmLength(FMath::Lerp<float, float>(StartZoomAmount, TargetZoomAmount,
				ResetCurveValue));
		}
		else
		{
			SetSpringArmTargetArmLength(TargetZoomAmount);
		}
	}
	else if (NumPendingScrollZooms != 0)
	{
		// Zoom target was set by scrolling with mouse wheel

		if (CameraZoomSpeed > 0.f && CameraMouseWheelZoomCurve != nullptr)
		{
			MouseWheelZoomCurveAccumulatedTime += DeltaTime * CameraZoomSpeed;

			// Check if have reached target zoom
			if (MouseWheelZoomCurveAccumulatedTime >= MouseWheelZoomCurveMax)
			{
				MouseWheelZoomCurveAccumulatedTime = 0.f;
				NumPendingScrollZooms = 0;

				SetSpringArmTargetArmLength(TargetZoomAmount);
			}
			else
			{
				const float ZoomCurveValue = CameraMouseWheelZoomCurve->GetFloatValue(MouseWheelZoomCurveAccumulatedTime);
				float NewZoomAmount = FMath::Lerp<float, float>(StartZoomAmount, TargetZoomAmount, ZoomCurveValue);

				SetSpringArmTargetArmLength(NewZoomAmount);
			}
		}
		else
		{
			SetSpringArmTargetArmLength(TargetZoomAmount);
		}
	}

	//-------------------------------------------------------------------------------
	// Checking if resetting camera has finished

	// Check if camera resetting to default rotation/zoom has completed
	if ((bIsResettingCameraRotation || bIsResettingCameraZoom)
		&& (ResetCameraCurve == nullptr || (ResetCameraCurveAccumulatedTime >= ResetCameraCurveMax)))
	{
		ResetCameraCurveAccumulatedTime = 0.f;
		bIsResettingCameraRotation = false;
		bIsResettingCameraZoom = false;

		/* The curve has finished apparently. Check they ended on the right values */
		assert(ControlRotation.Equals(DefaultCameraRotation, 1e-3f));
		assert(SpringArm->TargetArmLength == TargetZoomAmount);
	}

	//-------------------------------------------------------------------------------
	// Other stuff not related to camera rotation/zoom

	if (IsPlacingGhost())
	{
		/* Unhighlight object under mouse if there is one */
		UnhoverPreviouslyHoveredSelectable();

		const bool bLineTraceHit = LineTraceUnderMouse(GROUND_CHANNEL);

		if (bIsLMBPressed)
		{
			/* Because these are currently only used for ghost rotation they are here but could 
			be moved to start of tick func */
			MouseLocLastFrame = MouseLocThisFrame;
			MouseLocThisFrame = GetMouseCoords();

			if (bNeedToRecordGhostLocOnNextTick)
			{
				bNeedToRecordGhostLocOnNextTick = false;
				
				if (bLineTraceHit)
				{
					// Last move of ghost. Will not move again until LMB is released
					MoveGhostBuilding();
					
					// Store the screen loc of ghost building in GhostScreenSpaceLoc
					bool bResult = ProjectWorldLocationToScreen(GhostBuilding->GetActorLocation(),
						GhostScreenSpaceLoc);
					assert(bResult);
				}
			}
			
			RotateGhost(DeltaTime);
		}
		else
		{
			if (bLineTraceHit)
			{
				MoveGhostBuilding();
			}
		}

		/* Change appearance of ghost based on whether it is at a buildable location or not */
		const bool bIsBuildableLoc = Statics::IsBuildableLocation(GetWorld(), GS, PS, FactionInfo, 
			GhostBuilding->GetType(), GhostBuilding->GetActorLocation(), 
			GhostBuilding->GetActorRotation(), false);
		GhostBuilding->TrySetAppearanceForBuildLocation(bIsBuildableLoc);
	}
	else
	{
		/* Check if player is hovering a UI button (or any widget really). If yes then we want to set the mouse cursor 
		to the default match cursor even if a context action is pending. If the context action
		uses a decal then I guess we will leave it where it is. Alternative could be to hide it */
		if (IsHoveringUIElement())
		{
			ScreenLocationCurrentCursor = DefaultCursor;
			CursorToShow = DefaultCursor;
			UnhoverPreviouslyHoveredSelectable();
		}
		else
		{
			if (IsContextActionPending())
			{
				const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(PendingContextAction);
				AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);
				if (AbilityInfo.GetMouseAppearanceOption() == EAbilityMouseAppearance::CustomMouseCursor
					&& AbilityInfo.RequiresSelectableTarget())
				{
					LineTraceUnderMouse(SELECTABLES_CHANNEL);

					/* Set cursor based on what is being hovered over but only if this is a targeting type
					ability that uses a custom mouse cursor. Also the cursor path has to have been set
					by user otherwise silent fail */

					AActor * const HitActor = HitResult.GetActor();

					/* Assuming at least the default cursor will be valid */
					CursorToShow = &AbilityInfo.GetDefaultCursorInfo();

					/* Update the mouse cursor. Dependant on what is being hovered over */
					if (Statics::IsValid(HitActor) && IsASelectable(HitActor))
					{
						bool bIsHoveredTargetable = false;

						const bool bIsHoveredFriendly = Statics::IsFriendly(HitActor, PS->GetTeamTag());

						/* Check if our affiliation is targetable */
						bool bIsTargetableSoFar = (bIsHoveredFriendly && AbilityInfo.CanTargetFriendlies())
							|| (!bIsHoveredFriendly && AbilityInfo.CanTargetEnemies());

						if (bIsTargetableSoFar)
						{
							/* Now check if the target's type is ok */
							if (Statics::CanTypeBeTargeted(HitActor, AbilityInfo.GetAcceptableTargetFNames()))
							{
								/* Check any custom checks the ability requires. If at least one
								selected selectable can successfully use the ability then we will
								choose the "acceptable target" mouse cursor */
								const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();
								const bool bCheckIfTargetIsSelf = !AbilityInfo.CanTargetSelf() && AbilityInfo.CanTargetFriendlies();
								ISelectable * HitActorAsSelectable = CastChecked<ISelectable>(HitActor);
								for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
								{
									AActor * Elem = SelectedAndSupportPendingContextAction[i];

									if (Statics::IsValid(Elem))
									{
										ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

										const bool bSelfTargetOk = (bCheckIfTargetIsSelf == false)
											|| (bCheckIfTargetIsSelf == true && Elem != HitActor);

										const bool bRangeOk = (bCheckRange == false)
											|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, HitActorAsSelectable));

										/* Unlike the Input_LMBPressed checks we don't check the self
										state ability custom check. We also will not check selectable
										resources. If desired could change this though */
										if (bSelfTargetOk && bRangeOk && EffectActor->IsUsable_TargetChecks(AsSelectable, HitActorAsSelectable))
										{
											bIsHoveredTargetable = true;
											break;
										}
									}
									else
									{
										/* Book keep and remove non-valids as we go */
										SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
									}
								}
							}
						}

						/* Set the mouse cursor to show if valid */
						if (bIsHoveredTargetable)
						{
							if (AbilityInfo.GetAcceptableTargetCursorInfo().ContainsCustomCursor())
							{
								CursorToShow = &AbilityInfo.GetAcceptableTargetCursorInfo();
							}
						}
						else
						{
							if (AbilityInfo.GetUnacceptableTargetCursorInfo().ContainsCustomCursor())
							{
								CursorToShow = &AbilityInfo.GetUnacceptableTargetCursorInfo();
							}
						}
					}
				}
				else if (AbilityInfo.RequiresSelectableTarget() == false)
				{
					/* This may be able to be moved into the if below (or maybe it needs to go 
					in both the if and else if since I added the else if case more recently). I 
					had a legacy comment 
					saying that changed an assert to that if and have not really bothered 
					checking whether this can be moved or not */
					LineTraceUnderMouse(GROUND_CHANNEL);

					if (AbilityInfo.GetMouseAppearanceOption() == EAbilityMouseAppearance::HideAndShowDecal)
					{
						/* Decide what decal we should show based on whether the mouse's current location
						is at an acceptable location or not */

						/* HitResult.Location may not be 100% correct. UpdateMouseDecalLocation
						also uses it and that seems ok but if issues arise may actually need to
						do another line trace against environment channel to get correct location */
						if (AbilityInfo.CanLocationBeInsideFog() == false
							&& Statics::IsLocationOutsideFogLocallyNotChecked(HitResult.Location, FogOfWarManager) == false)
						{
							SetContextDecalType(AbilityInfo, EAbilityDecalType::NotUsableLocation);
						}
						else 
						{
							// Max range == 0 implies unlimited range therefore no need to check
							const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();

							EAbilityDecalType DecalToUse = EAbilityDecalType::NotUsableLocation;
							/* Here we check range (if bCheckRange is true) and I guess we also check
							whether the custom ability requirements are fulfilled */
							for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
							{
								AActor * Elem = SelectedAndSupportPendingContextAction[i];

								if (Statics::IsValid(Elem))
								{
									ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

									const bool bRangeOk = (bCheckRange == false)
										|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, HitResult.Location));

									if (bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable))
									{
										/* One selectable being able to use ability is good enough to
										make decal the "good" type */
										DecalToUse = EAbilityDecalType::UsableLocation;
										break;
									}
								}
								else
								{
									// Book keep and remove non-valids along the way
									SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
								}
							}

							/* If here then no selected selectable can use the ability <--- Wrong */
							SetContextDecalType(AbilityInfo, DecalToUse);
						}

						/* Update the decal's location */
						UpdateMouseDecalLocation(DeltaTime);
					}
					else if (AbilityInfo.GetMouseAppearanceOption() == EAbilityMouseAppearance::CustomMouseCursor)
					{
						/* Note do not use the acceptable target cursor anywhere below */

						if (AbilityInfo.CanLocationBeInsideFog() == false
							&& Statics::IsLocationOutsideFogLocally(HitResult.Location, FogOfWarManager) == false)
						{
							CursorToShow = &AbilityInfo.GetUnacceptableTargetCursorInfo();
						}
						else
						{
							bool bAcceptableTarget = false;
							
							const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();
							
							for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
							{
								AActor * Elem = SelectedAndSupportPendingContextAction[i];

								if (Statics::IsValid(Elem))
								{
									ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

									const bool bRangeOk = (bCheckRange == false)
										|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, HitResult.Location));

									if (bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable))
									{
										/* One selectable being able to use ability is good enough to
										make decal the "good" type */
										bAcceptableTarget = true;
										break;
									}
								}
								else
								{
									// Book keep and remove non-valids along the way
									SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
								}
							}

							CursorToShow = bAcceptableTarget ? &AbilityInfo.GetDefaultCursorInfo() : &AbilityInfo.GetUnacceptableTargetCursorInfo();
						}

						SetMouseCursor(*CursorToShow);
					}
				}
				else
				{
					LineTraceUnderMouse(SELECTABLES_CHANNEL);
				}
			}
			else if (IsGlobalSkillsPanelAbilityPending())
			{
				const FCommanderAbilityInfo * AbilityInfo = PendingCommanderAbility->GetAbilityInfo();
				const EAbilityTargetingMethod TargetingMethod = AbilityInfo->GetTargetingMethod();
				/* Abilities that target players don't need to do any of this */
				if (TargetingMethod != EAbilityTargetingMethod::RequiresPlayer)
				{
					const EAbilityMouseAppearance MouseAppearance = AbilityInfo->GetMouseAppearance();
					if (MouseAppearance == EAbilityMouseAppearance::CustomMouseCursor)
					{ 
						if (TargetingMethod == EAbilityTargetingMethod::RequiresSelectable)
						{
							LineTraceUnderMouse(SELECTABLES_CHANNEL);

							AActor * const HitActor = HitResult.GetActor();
							if (Statics::IsValid(HitActor) && IsASelectable(HitActor))
							{
								if (Statics::CanTypeBeTargeted(HitActor, AbilityInfo->GetAcceptableSelectableTargetFNames()))
								{
									ISelectable * HitActorAsSelectable = CastChecked<ISelectable>(HitActor);
									const ETeam TargetsTeam = HitActorAsSelectable->GetAttributesBase().GetTeam();

									/* Check if target is the right affiliation */
									if (TargetsTeam != Team)
									{
										if (AbilityInfo->CanTargetEnemies() == false)
										{
											CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
										}
										else
										{
											CursorToShow = &AbilityInfo->GetAcceptableTargetMouseCursorInfo();
										}
									}
									else
									{
										if (AbilityInfo->CanTargetFriendlies() == false)
										{
											CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
										}
										else
										{
											CursorToShow = &AbilityInfo->GetAcceptableTargetMouseCursorInfo();
										}
									}
								}
								else
								{
									CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
								}
							}
							else
							{
								CursorToShow = &AbilityInfo->GetDefaultMouseCursorInfo();
							}
						}
						else if (TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocation)
						{
							if (AbilityInfo->CanTargetInsideFog() == false)
							{
								// In terms of this ability I'm pretty sure we don't need to do this 
								// trace unless we're checking if outside fog, but if things look 
								// ugly then maybe try moving it to above the if statement
								LineTraceUnderMouse(GROUND_CHANNEL);

								if (Statics::IsLocationOutsideFogLocally(HitResult.Location, FogOfWarManager))
								{
									/* Can use acceptable target one here instead if preferred */
									CursorToShow = &AbilityInfo->GetDefaultMouseCursorInfo();
								}
								else
								{
									CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
								}
							}
							else
							{
								/* Can use acceptable target one here instead if preferred */
								CursorToShow = &AbilityInfo->GetDefaultMouseCursorInfo();
							}
						}
						else // Assumed RequiresWorldLocationOrSelectable 
						{
							assert(TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable);

							/* BTW the behavior here is: if the mouse is hovering over an 
							unacceptable selectable target then we do OT allow the ability to 
							go through and target the ground. May want to add a bool to 
							FCommanderAbilityInfo and allow both types of behavior in future. */

							/* For the same reason as the ground channel trace above I think 
							this only has to be done against selectables channel */
							LineTraceUnderMouse(SELECTABLES_AND_GROUND_CHANNEL);

							AActor * const HitActor = HitResult.GetActor();
							if (Statics::IsValid(HitActor))
							{
								if (IsASelectable(HitActor))
								{
									if (Statics::CanTypeBeTargeted(HitActor, AbilityInfo->GetAcceptableSelectableTargetFNames()))
									{
										ISelectable * HitActorAsSelectable = CastChecked<ISelectable>(HitActor);
										const ETeam TargetsTeam = HitActorAsSelectable->GetAttributesBase().GetTeam();

										/* Check if target is the right affiliation */
										if (TargetsTeam != Team)
										{
											if (AbilityInfo->CanTargetEnemies() == false)
											{
												CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
											}
											else
											{
												CursorToShow = &AbilityInfo->GetAcceptableTargetMouseCursorInfo();
											}
										}
										else
										{
											if (AbilityInfo->CanTargetFriendlies() == false)
											{
												CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
											}
											else
											{
												CursorToShow = &AbilityInfo->GetAcceptableTargetMouseCursorInfo();
											}
										}
									}
									else
									{
										CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
									}
								}
								else // Have hit ground 
								{
									if (AbilityInfo->CanTargetInsideFog() == false)
									{
										if (Statics::IsLocationOutsideFogLocally(HitResult.Location, FogOfWarManager))
										{
											/* Can use acceptable target one here instead if preferred */
											CursorToShow = &AbilityInfo->GetDefaultMouseCursorInfo();
										}
										else
										{
											CursorToShow = &AbilityInfo->GetUnacceptableTargetMouseCursorInfo();
										}
									}
									else
									{
										/* Can use acceptable target one here instead if preferred */
										CursorToShow = &AbilityInfo->GetDefaultMouseCursorInfo();
									}
								}
							}
							else
							{
								CursorToShow = &AbilityInfo->GetDefaultMouseCursorInfo();
							}
						}
					}
					else if (MouseAppearance == EAbilityMouseAppearance::HideAndShowDecal)
					{
						/* I think this assert is sensible. It doesn't make sense for an ability 
						to be able to target selecables but show a decal IMO. */
						assert(TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocation);

						EAbilityDecalType DecalType;

						LineTraceUnderMouse(GROUND_CHANNEL);

						if (AbilityInfo->CanTargetInsideFog() == false)
						{
							if (Statics::IsLocationOutsideFogLocally(HitResult.Location, FogOfWarManager))
							{
								DecalType = EAbilityDecalType::UsableLocation;
							}
							else
							{
								DecalType = EAbilityDecalType::NotUsableLocation;
							}
						}
						else
						{
							DecalType = EAbilityDecalType::UsableLocation;
						}

						SetCommanderAbilityDecalType(*AbilityInfo, DecalType);
						UpdateMouseDecalLocation(DeltaTime);
					}
					else // Assumed NoChange
					{
						assert(MouseAppearance == EAbilityMouseAppearance::NoChange);
						
						if (TargetingMethod == EAbilityTargetingMethod::RequiresSelectable)
						{
							LineTraceUnderMouse(SELECTABLES_CHANNEL);
						}
						else if (TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocation)
						{
							LineTraceUnderMouse(GROUND_CHANNEL);
						}
						else // Assumed RequiresWorldLocationOrSelectable
						{
							assert(TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable);

							LineTraceUnderMouse(SELECTABLES_AND_GROUND_CHANNEL);
						}
					}
				}
				else
				{
					LineTraceUnderMouse(SELECTABLES_CHANNEL);
				}
			}
			else /* No abilities pending */
			{
				LineTraceUnderMouse(SELECTABLES_CHANNEL);

				if (HasSelection() && IsSelectionControlledByThis())
				{
					/* Update right-click commands mouse cursor. */
					AActor * HitActor = HitResult.GetActor();
					if (Statics::IsValid(HitActor) && IsASelectable(HitActor))
					{
						ISelectable * HitActorAsSelectable = CastChecked<ISelectable>(HitActor);
						const EAffiliation Affiliation = HitActorAsSelectable->GetAttributesBase().GetAffiliation();
						GetMouseCursor_NoAbilitiesPending(HitActor, HitActorAsSelectable, Affiliation, CursorToShow);
					}
				}
			}

			UpdateSelectableUnderMouse(DeltaTime);
		}
	}

#if WITH_EDITOR
	// Do not override a development action cursor
	if (DevelopmentInputIntercept_LMBPress == EDevelopmentAction::None)
#endif
	{
		/* Set the mouse cursor for this frame. 
		In regards to the ContainsCustomCursor() call: Only show mouse cursor if it was set by user. For
		performance only check once here instead of every time when assigning to CursorToShow. 
		Basically we assume the user has set custom cursors for everything. If this isn't true 
		perhaps things won't look as expected */
		if (CursorToShow->ContainsCustomCursor()) 
		{
			SetMouseCursor(*CursorToShow);
		}
	}
	
	//-----------------------------------------------------------------------------------------
	//	Tooltip updating stuff
	//-----------------------------------------------------------------------------------------
	//	Looks hard to read. Consider just using regular if/else instead of #if/#else/#endif

	if (IsHoveringUIElement())
	{
#if !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS

		AccumulatedTimeSpentHoveringUIElement += DeltaTime;
		if (AccumulatedTimeSpentHoveringUIElement >= TooltipOptions::UI_ELEMENT_HOVER_TIME_REQUIREMENT)
		{
			if (!bShowingUIElementTooltip)
			{
				if (HoveredUserWidget == nullptr)
				{
					HUD->ShowTooltipWidget(HoveredUIElement, HoveredUIElement->GetPurpose());
				}
				else
				{
					HUD->ShowTooltipWidget(HoveredUserWidget, HoveredUIElement->GetPurpose());
				}
				
				bShowingUIElementTooltip = true;
			}
		}

#endif // !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS

#if !INSTANT_SHOWING_SELECTABLE_TOOLTIPS

#if USING_SELECTABLE_HOVER_TIME_DECAY_DELAY
		TimeSpentNotHoveringSelectable += DeltaTime;

		if (TimeSpentNotHoveringSelectable >= TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_DELAY)
		{
			// Decay accumulated hover time
			AccumulatedTimeSpentHoveringSelectable -= DeltaTime * TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_RATE;
		}
#else
		// Decay accumulated hover time
		AccumulatedTimeSpentHoveringSelectable -= DeltaTime * TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_RATE;
#endif

#endif // !INSTANT_SHOWING_SELECTABLE_TOOLTIPS 
	}
	else
	{
#if !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS

#if USING_UI_ELEMENT_HOVER_TIME_DECAY_DELAY
		TimeSpentNotHoveringUIElement += DeltaTime; 

		if (TimeSpentNotHoveringUIElement >= TooltipOptions::UI_ELEMENT_HOVER_TIME_DECAY_DELAY)
		{
			// Decay accumulated hover time
			AccumulatedTimeSpentHoveringUIElement -= DeltaTime * TooltipOptions::UI_ELEMENT_HOVER_TIME_DECAY_RATE;
		}
#else
		// Decay accumulated hover time
		AccumulatedTimeSpentHoveringUIElement -= DeltaTime * TooltipOptions::UI_ELEMENT_HOVER_TIME_DECAY_RATE;
#endif	

#endif // !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS

#if !INSTANT_SHOWING_SELECTABLE_TOOLTIPS
		// If hovering a selectable...
		if (ObjectUnderMouseOnTick != nullptr)
		{
			AccumulatedTimeSpentHoveringSelectable += DeltaTime;
			if (AccumulatedTimeSpentHoveringSelectable >= TooltipOptions::SELECTABLE_HOVER_TIME_REQUIREMENT)
			{
				if (!bShowingSelectablesTooltip)
				{
					ObjectUnderMouseOnTick->ShowTooltip(HUD);
					bShowingSelectablesTooltip = true;
				}
			}
		}
		else
		{
#if USING_SELECTABLE_HOVER_TIME_DECAY_DELAY
			TimeSpentNotHoveringSelectable += DeltaTime;

			if (TimeSpentNotHoveringSelectable >= TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_DELAY)
			{
				// Decay accumulated hover time
				AccumulatedTimeSpentHoveringSelectable -= DeltaTime * TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_RATE;
			}
#else
			// Decay accumulated hover time
			AccumulatedTimeSpentHoveringSelectable -= DeltaTime * TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_RATE;
#endif
		}
#endif // !INSTANT_SHOWING_SELECTABLE_TOOLTIPS
	}
}

void ARTSPlayerController::SetupInputComponent()
{
	if (Type == EPlayerType::Player)
	{
		/* Putting this here to remove any bindings that may have been added because player was 
		an observer last game. Not sure if PC will carry over between games though */
		InputComponent->ClearActionBindings();

		for (const auto & Elem : KeyMappings::ActionInfos)
		{
			if (WITH_EDITOR || !Elem.bIsEditorOnly)
			{
				if (Elem.bIsForPlayer)
				{
					if (Elem.HasActionForKeyPress())
					{
						FInputActionBinding AB(Elem.ActionName, IE_Pressed);
						AB.ActionDelegate.BindDelegate(this, Elem.OnPressedAction);
						FInputActionBinding & Binding = InputComponent->AddActionBinding(AB);
						Binding.bExecuteWhenPaused = Elem.bExecuteWhenPaused;
					}
					if (Elem.HasActionForKeyRelease())
					{
						FInputActionBinding AB(Elem.ActionName, IE_Released);
						AB.ActionDelegate.BindDelegate(this, Elem.OnReleasedAction);
						FInputActionBinding & Binding = InputComponent->AddActionBinding(AB);
						Binding.bExecuteWhenPaused = Elem.bExecuteWhenPaused;
					}
				}
			}
		}

		for (const auto & Elem : AxisMappings::AxisInfos)
		{
			if (WITH_EDITOR || !Elem.bIsEditorOnly)
			{
				if (Elem.bIsForPlayer)
				{
					FInputAxisBinding AB(Elem.ActionName);
					AB.AxisDelegate.BindDelegate(this, Elem.FunctionPtr);
					InputComponent->AxisBindings.Add(AB);
					InputComponent->AxisBindings.Last().bExecuteWhenPaused = Elem.bExecuteWhenPaused;
				}
			}
		}
	}
	else if (Type == EPlayerType::Observer)
	{
		/* Putting this here to remove any bindings that may have been added because player was
		a player last game. Not sure if PC will carry over between games though */
		InputComponent->ClearActionBindings();
		
		for (const auto & Elem : KeyMappings::ActionInfos)
		{
			if (WITH_EDITOR || !Elem.bIsEditorOnly)
			{
				if (Elem.bIsForObserver)
				{
					if (Elem.HasActionForKeyPress())
					{
						FInputActionBinding AB(Elem.ActionName, IE_Pressed);
						AB.ActionDelegate.BindDelegate(this, Elem.OnPressedAction);
						FInputActionBinding & Binding = InputComponent->AddActionBinding(AB);
						Binding.bExecuteWhenPaused = Elem.bExecuteWhenPaused;
					}
					if (Elem.HasActionForKeyRelease())
					{
						FInputActionBinding AB(Elem.ActionName, IE_Released);
						AB.ActionDelegate.BindDelegate(this, Elem.OnReleasedAction);
						FInputActionBinding & Binding = InputComponent->AddActionBinding(AB);
						Binding.bExecuteWhenPaused = Elem.bExecuteWhenPaused;
					}
				}
			}
		}

		for (const auto & Elem : AxisMappings::AxisInfos)
		{
			if (WITH_EDITOR || !Elem.bIsEditorOnly)
			{
				if (Elem.bIsForObserver)
				{
					FInputAxisBinding AB(Elem.ActionName);
					AB.AxisDelegate.BindDelegate(this, Elem.FunctionPtr);
					InputComponent->AxisBindings.Add(AB);
					InputComponent->AxisBindings.Last().bExecuteWhenPaused = Elem.bExecuteWhenPaused;
				}
			}
		}
	}
	else
	{
		Super::SetupInputComponent();
	}
}

void ARTSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ARTSPlayerController::OnLMBPressed_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		/* Asserts I believe to be true, but may need to remove them if not and account for
		them with some if/else statements instead */
		assert(!IsPlacingGhost());
		assert(!IsContextActionPending());
		assert(MouseMovement == 0.f);

		ButtonPressedOnLMBPressed = nullptr;

		OnPersistentTabButtonLeftClicked(ButtonWidget);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget)
{
	if (OnRMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnRMBPressed = nullptr;
		
		OnPersistentTabButtonRightClicked(ButtonWidget);
	}
	else
	{
		/* Do this in case the release was not over same button */
		ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
		ButtonPressedOnRMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnLMBPressed_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		ButtonWidget->OnClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		/* Asserts I believe to be true, but may need to remove them if not and account for
		them with some if/else statements instead */
		assert(MouseMovement == 0.f);

		ButtonPressedOnLMBPressed = nullptr;

		OnContextButtonClick(*ButtonWidget->GetButtonType());
	}
	else // Wasn't considered a click
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PrimarySelected_InventoryButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget; 
}

void ARTSPlayerController::OnLMBReleased_PrimarySelected_InventoryButton(UMyButton * ButtonWidget, UInventoryItemButton * ButtonUserWidget)
{
	DoOnEveryLMBRelease();
	
	/* Check if a click on the button happened */
	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		/* Asserts I believe to be true, but may need to remove them if not and account for
		them with some if/else statements instead */
		assert(MouseMovement == 0.f);
		
		ButtonPressedOnLMBPressed = nullptr;

		// Do some inventory OnClicked logic
		OnInventorySlotButtonLeftClicked(*const_cast<FInventorySlotState*>(ButtonUserWidget->GetInventorySlot()),
			ButtonUserWidget->GetServerSlotIndex());
	}
	else // Wasn't considered a click
	{
		// Could run most of the On_LMB_Released logic, but will choose to do nothing
		
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PrimarySelected_InventoryButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PrimarySelected_InventoryButton(UMyButton * ButtonWidget, UInventoryItemButton * ButtonUserWidget)
{
	/* Check if we did a click on a button */
	if (OnRMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		/* Kind of don't have to do this actually. Same with LMB one. If choosing not to 
		update it just be aware to check if bIsLMBPressed is true first */
		ButtonPressedOnRMBPressed = nullptr;
		
		// Run some on clicked logic like dropping item
		OnInventorySlotButtonRightClicked(ButtonUserWidget);
	}
	else
	{
		ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
		ButtonPressedOnRMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnLMBPressed_PrimarySelected_ShopButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PrimarySelected_ShopButton(UMyButton * ButtonWidget, UItemOnDisplayInShopButton * ButtonUserWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		/* Asserts I believe to be true, but may need to remove them if not and account for
		them with some if/else statements instead */
		assert(!IsPlacingGhost());
		assert(!IsContextActionPending());
		assert(MouseMovement == 0.f);

		ButtonPressedOnLMBPressed = nullptr;

		OnShopSlotButtonLeftClicked(ButtonUserWidget);
	}
	else // Wasn't considered a click
	{	
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PrimarySelected_ShopButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PrimarySelected_ShopButton(UMyButton * ButtonWidget, UItemOnDisplayInShopButton * ButtonUserWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		
		// TODO queue cancelling behavior
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PauseGame(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PauseGame(UMyButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		/* It's quite possible this button isn't even clickable with the pause menu showing 
		overtop but just in case we check */
		if (HUD->IsPauseMenuShowingOrPlayingShowAnimation())
		{
			ResumePlay();
		}
		else
		{
			PauseGameAndShowPauseMenu();
		}
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PauseGame(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PauseGame(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_CommanderSkillTreeShowButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();

	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_CommanderSkillTreeShowButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		HUD->OnToggleCommanderSkillTreeButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_CommanderSkillTreeShowButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_CommanderSkillTreeShowButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_GlobalSkillsPanelButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_GlobalSkillsPanelButton(UMyButton * ButtonWidget, UGlobalSkillsPanelButton * UserWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		OnGlobalSkillsPanelButtonLeftClicked(UserWidget);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_GlobalSkillsPanelButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_GlobalSkillsPanelButton(UMyButton * ButtonWidget, UGlobalSkillsPanelButton * UserWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PlayerTargetingPanelButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PlayerTargetingPanelButton(UMyButton * ButtonWidget, 
	ARTSPlayerState * AbilityTarget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		OnPlayerTargetingPanelButtonLeftClicked(AbilityTarget);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PlayerTargetingPanelButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PlayerTargetingPanelButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_HidePlayerTargetingPanel(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_HidePlayerTargetingPanel(UMyButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		CancelPendingGlobalSkillsPanelAbility();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_HidePlayerTargetingPanel(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_HidePlayerTargetingPanel(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		ButtonWidget->OnClicked(this, PS);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget, UGarrisonedUnitInfo * SingleGarrisonInfoWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		SingleGarrisonInfoWidget->OnUnloadButtonClicked(this);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_UnloadGarrisonButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_UnloadGarrisonButton(UMyButton * ButtonWidget, UGarrisonInfo * GarrisonInfoWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		GarrisonInfoWidget->OnUnloadGarrisonButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_UnloadGarrisonButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_UnloadGarrisonButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PauseMenu_Resume(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PauseMenu_Resume(UMyButton * ButtonWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		ResumePlay();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PauseMenu_Resume(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PauseMenu_Resume(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget, UPauseMenu * PauseMenuWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		PauseMenuWidget->ShowSettingsMenu();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget, UPauseMenu * PauseMenuWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		if (PauseMenuWidget->HasConfirmExitToMainMenuWidget())
		{
			PauseMenuWidget->ShowConfirmExitToMainMenuWidget(HUD);
		}
		else
		{
			// Return to main menu. Not sure if I have a func for this already or not TODO
		}
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget, UPauseMenu * PauseMenuWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		if (PauseMenuWidget->HasConfirmExitToOperatingSystemWidget())
		{
			PauseMenuWidget->ShowConfirmExitToOperatingSystemWidget(HUD);
		}
		else
		{
			GI->OnQuitGameInitiated();
		}
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_ConfirmationWidgetYesButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_ConfirmationWidgetYesButton(UMyButton * ButtonWidget, UInMatchConfirmationWidget * ConfirmationWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		switch (ConfirmationWidget->GetPurpose())
		{
		case EConfirmationWidgetPurpose::ExitToMainMenu:
		{
			// TODO return to main menu
			break;
		}
		case EConfirmationWidgetPurpose::ExitToOperatingSystem:
		{
			GI->OnQuitGameInitiated();
			break;
		}
		}
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_ConfirmationWidgetYesButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_ConfirmationWidgetYesButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_ConfirmationWidgetNoButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_ConfirmationWidgetNoButton(UMyButton * ButtonWidget, UInMatchConfirmationWidget * ConfirmationWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;

		ConfirmationWidget->OnNoButtonClicked(HUD);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_ConfirmationWidgetNoButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_ConfirmationWidgetNoButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget, USettingsWidget * SettingsWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingsWidget->OnSaveChangesAndReturnButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget, USettingsWidget * SettingsWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingsWidget->OnDiscardChangesAndReturnButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget, USettingsWidget * SettingsMenu)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingsMenu->OnResetToDefaultsButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget, USettingsConfirmationWidget_ResetToDefaults * ConfirmationWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		ConfirmationWidget->OnYesButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget, USettingsConfirmationWidget_ResetToDefaults * ConfirmationWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		ConfirmationWidget->OnNoButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget, USettingsConfirmationWidget_Exit * ConfirmationWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		ConfirmationWidget->OnSaveButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget, USettingsConfirmationWidget_Exit * ConfirmationWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		ConfirmationWidget->OnDiscardButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget, USettingsConfirmationWidget_Exit * ConfirmationWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		ConfirmationWidget->OnCancelButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_SwitchSettingsSubmenu(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_SwitchSettingsSubmenu(UMyButton * ButtonWidget, 
	USettingsWidget * SettingsWidget, UWidget * WidgetToSwitchTo)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingsWidget->OnSwitchToSubmenuButtonClicked(WidgetToSwitchTo);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_SwitchSettingsSubmenu(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_SwitchSettingsSubmenu(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_AdjustVideoSettingLeft(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_AdjustVideoSettingLeft(UMyButton * ButtonWidget, USingleVideoSettingWidget * SettingWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingWidget->OnAdjustLeftButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_AdjustVideoSettingLeft(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_AdjustVideoSettingLeft(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_AdjustVideoSettingRight(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_AdjustVideoSettingRight(UMyButton * ButtonWidget, USingleVideoSettingWidget * SettingWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingWidget->OnAdjustRightButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_AdjustVideoSettingRight(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_AdjustVideoSettingRight(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_DecreaseAudioQuality(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_DecreaseAudioQuality(UMyButton * ButtonWidget, UAudioSettingsWidget * AudioWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		AudioWidget->OnDecreaseAudioQualityButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_DecreaseAudioQuality(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_DecreaseAudioQuality(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_IncreaseAudioQuality(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_IncreaseAudioQuality(UMyButton * ButtonWidget, UAudioSettingsWidget * AudioWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		AudioWidget->OnIncreaseAudioQualityButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_IncreaseAudioQuality(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_IncreaseAudioQuality(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_DecreaseVolumeButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_DecreaseVolumeButton(UMyButton * ButtonWidget, USingleAudioClassWidget * AudioWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		AudioWidget->OnDecreaseVolumeButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_DecreaseVolumeButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_DecreaseVolumeButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_IncreaseVolumeButton(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_IncreaseVolumeButton(UMyButton * ButtonWidget, USingleAudioClassWidget * AudioWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		AudioWidget->OnIncreaseVolumeButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_IncreaseVolumeButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_IncreaseVolumeButton(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget, USingleBoolControlSettingWidget * SettingWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingWidget->OnAdjustLeftButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_AdjustBoolControlSettingRight(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_AdjustBoolControlSettingRight(UMyButton * ButtonWidget, USingleBoolControlSettingWidget * SettingWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingWidget->OnAdjustRightButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_AdjustBoolControlSettingRight(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_AdjustBoolControlSettingRight(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_DecreaseControlSetting_Float(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_DecreaseControlSetting_Float(UMyButton * ButtonWidget, USingleFloatControlSettingWidget * SettingWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingWidget->OnDecreaseButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_DecreaseControlSetting_Float(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_DecreaseControlSetting_Float(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_IncreaseControlSetting_Float(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_IncreaseControlSetting_Float(UMyButton * ButtonWidget, USingleFloatControlSettingWidget * SettingWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		SettingWidget->OnIncreaseButtonClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_IncreaseControlSetting_Float(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_IncreaseControlSetting_Float(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_RemapKey(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_RemapKey(UMyButton * ButtonWidget, USingleKeyBindingWidget * KeyBindingWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		KeyBindingWidget->OnClicked();
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_RemapKey(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_RemapKey(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget, 
	URebindingCollisionWidget * CollisionWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
 
		FTryBindActionResult Result = FTryBindActionResult(PendingKeyRebind_KeyBindingsWidget->GetPendingKey());
		bool bSuccess;
		if (PendingKeyRebind_Action != EKeyMappingAction::None)
		{
			bSuccess = TryRebindAction(PendingKeyRebind_Action, PendingKeyRebind_KeyBindingsWidget->GetPendingKey(), true, Result);
		}
		else
		{
			bSuccess = TryRebindAction(PendingKeyRebind_Axis, PendingKeyRebind_KeyBindingsWidget->GetPendingKey(), true, Result);
		}
		assert(bSuccess);

		PendingKeyRebind_KeyBindingsWidget->OnKeyBindAttempt(GI, this, bSuccess, Result);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget, URebindingCollisionWidget * CollisionWidget)
{
	DoOnEveryLMBRelease();

	if (OnLMBReleased_WasUIButtonClicked(ButtonWidget))
	{
		ButtonPressedOnLMBPressed = nullptr;
		CollisionWidget->OnCancelButtonClicked(this);
	}
	else
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}
}

void ARTSPlayerController::OnRMBPressed_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::OnLMBPressed_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget)
{
	DoOnEveryLMBPress();
	ButtonPressedOnLMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnLMBReleased_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget)
{
	// TODO
}

void ARTSPlayerController::OnRMBPressed_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed = ButtonWidget;
}

void ARTSPlayerController::OnRMBReleased_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget)
{
	ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
	ButtonPressedOnRMBPressed = nullptr;
}

void ARTSPlayerController::ListenForKeyRemappingInputEvents(EKeyMappingAction ActionToRebind, UKeyBindingsWidget * InKeyBindingsWidget)
{
	PendingKeyRebind_Action = ActionToRebind;
	PendingKeyRebind_Axis = EKeyMappingAxis::None;
	PendingKeyRebind_KeyBindingsWidget = InKeyBindingsWidget;
}

void ARTSPlayerController::ListenForKeyRemappingInputEvents(EKeyMappingAxis ActionToRebind, UKeyBindingsWidget * InKeyBindingsWidget)
{
	PendingKeyRebind_Action = EKeyMappingAction::None;
	PendingKeyRebind_Axis = ActionToRebind;
	PendingKeyRebind_KeyBindingsWidget = InKeyBindingsWidget;
}

void ARTSPlayerController::CancelPendingKeyRebind()
{
	PendingKeyRebind_Action = EKeyMappingAction::None;
	PendingKeyRebind_Axis = EKeyMappingAxis::None;
	PendingKeyRebind_bWaitingForConfirmation = false;
	PendingKeyRebind_PressedModifierFlags_Left = EKeyModifiers::None;
	PendingKeyRebind_PressedModifierFlags_Right = EKeyModifiers::None;
	PendingKeyRebind_KeyBindingsWidget = nullptr;
}

void ARTSPlayerController::OnCancelPendingKeyRebindButtonHeld()
{
	PendingKeyRebind_Action = EKeyMappingAction::None;
	PendingKeyRebind_Axis = EKeyMappingAxis::None;
	PendingKeyRebind_bWaitingForConfirmation = false;
	PendingKeyRebind_PressedModifierFlags_Left = EKeyModifiers::None;
	PendingKeyRebind_PressedModifierFlags_Right = EKeyModifiers::None;
	if (PendingKeyRebind_KeyBindingsWidget != nullptr)
	{
		PendingKeyRebind_KeyBindingsWidget->OnPendingKeyBindCancelledViaCancelKey();
		PendingKeyRebind_KeyBindingsWidget = nullptr;
	}
	PendingKeyRebind_TimeSpentTryingToCancel = -1.f;
}

bool ARTSPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	/* Mostly a copy of APlayerController::InputKey. Have removed VR stuff near top */

	if (PendingKeyRebind_bWaitingForConfirmation)
	{
		// Is true the right value? Does it matter? Find out next time on dragon ball Z!
		return true;
	}
	/* Check if player wants to rebind action/axis mapping */
	else if ((PendingKeyRebind_Action != EKeyMappingAction::None) | (PendingKeyRebind_Axis != EKeyMappingAxis::None))
	{
		/* PlayerInput->InputKey updates what keys are pressed and carries out the bound action. 
		I don't want to carry out the bound action but I do need to keep track of whether 
		any modifiers are pressed so I'm rolling my own bookkeeping for that */
		const bool bModifierKeyEvent = PendingKeyRebind_UpdatePressedModifierFlags(Key, EventType);
		
		bool bTryRebind = false;
		const EKeyModifiers ModifierFlags = PendingKeyRebind_GetAllKeyModifiers();
		FKeyWithModifiers KeyWithModifiers = FKeyWithModifiers(Key, ModifierFlags);

		URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
		// Check if the event is for the 'cancel' key. Holding down the cancel key will cancel trying to bind the key
		if (Settings->GetBoundAction(KeyWithModifiers) == EKeyMappingAction::OpenPauseMenuSlashCancel)
		{
			if (EventType == IE_Pressed)
			{
				/* Holding down the cancel key will cancel trying to bind the key. 
				Gets queried in Tick */
				PendingKeyRebind_TimeSpentTryingToCancel = 0.f;
			}
			else if (EventType == IE_Released)
			{
				/* If the player presses then releases the cancel key quick enough they
				can bind the action to it */
				bTryRebind = true;
			}
		}
		/* For non-modifer keys we rebind on their press. */
		else if (!bModifierKeyEvent && EventType == IE_Pressed)
		{
			bTryRebind = true;
		}
		/* For modifier keys we rebind on their release */
		else if (bModifierKeyEvent && EventType == IE_Released)
		{
			bTryRebind = true;
			/* Set modifier flags to none so the UI doesn't display like ALT + ALT as your binding. 
			Also by doing this we don't allow bindings that are just made up of 2+ modifier 
			keys. Whether that's a good or bad thing I don't know */
			KeyWithModifiers = FKeyWithModifiers(Key, EKeyModifiers::None);
		}

		if (bTryRebind)
		{
			FTryBindActionResult Result = FTryBindActionResult(KeyWithModifiers);
			bool bSuccess;
			if (PendingKeyRebind_Action != EKeyMappingAction::None)
			{
				bSuccess = TryRebindAction(PendingKeyRebind_Action, KeyWithModifiers, false, Result);
			}
			else
			{
				bSuccess = TryRebindAction(PendingKeyRebind_Axis, KeyWithModifiers, false, Result);
			}

			if (PendingKeyRebind_KeyBindingsWidget != nullptr)
			{
				PendingKeyRebind_bWaitingForConfirmation = PendingKeyRebind_KeyBindingsWidget->OnKeyBindAttempt(GI, this, bSuccess, Result);
			}

			PendingKeyRebind_TimeSpentTryingToCancel = -1.f;
		}

		// Stop here to prevent PlayerInput->InputKey from processing it.
		return true;
	}

	bool bResult = false;
	if (PlayerInput != nullptr)
	{
		bResult = PlayerInput->InputKey(Key, EventType, AmountDepressed, bGamepad);
		if (bEnableClickEvents && (ClickEventKeys.Contains(Key) || ClickEventKeys.Contains(EKeys::AnyKey)))
		{
			FVector2D MousePosition;
			UGameViewportClient * ViewportClient = CastChecked<ULocalPlayer>(Player)->ViewportClient;
			if (ViewportClient && ViewportClient->GetMousePosition(MousePosition))
			{
				UPrimitiveComponent * ClickedPrimitive = nullptr;
				if (bEnableMouseOverEvents)
				{
					ClickedPrimitive = CurrentClickablePrimitive.Get();
				}
				else
				{
					FHitResult Hit;
					const bool bHit = GetHitResultAtScreenPosition(MousePosition, CurrentClickTraceChannel, true, Hit);
					if (bHit)
					{
						ClickedPrimitive = Hit.Component.Get();
					}
				}
				if (GetHUD())
				{
					if (GetHUD()->UpdateAndDispatchHitBoxClickEvents(MousePosition, EventType))
					{
						ClickedPrimitive = nullptr;
					}
				}

				if (ClickedPrimitive)
				{
					switch (EventType)
					{
					case IE_Pressed:
					case IE_DoubleClick:
						ClickedPrimitive->DispatchOnClicked(Key);
						break;

					case IE_Released:
						ClickedPrimitive->DispatchOnReleased(Key);
						break;

					case IE_Axis:
					case IE_Repeat:
						break;
					}
				}

				bResult = true;
			}
		}
	}

	return bResult;
}

bool ARTSPlayerController::PendingKeyRebind_UpdatePressedModifierFlags(FKey Key, EInputEvent EventType)
{
	/* Because on a modifier key release we alreadys try bind something the IE_Released cases 
	can have some stuff commented out */
	
	if (Key == EKeys::LeftControl)
	{
		switch (EventType)
		{
		case IE_Pressed:
			PendingKeyRebind_PressedModifierFlags_Left = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Left) | static_cast<uint8>(EKeyModifiers::CTRL));
			return true;
		case IE_Released:
			//PendingKeyRebind_PressedModifierFlags_Left = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Left) & static_cast<uint8>(EKeyModifiers::CTRL));
			return true;
		default:
			return false;
		}
	}
	else if (Key == EKeys::RightControl)
	{
		switch (EventType)
		{
		case IE_Pressed:
			PendingKeyRebind_PressedModifierFlags_Right = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Right) | static_cast<uint8>(EKeyModifiers::CTRL));
			return true;
		case IE_Released:
			//PendingKeyRebind_PressedModifierFlags_Right = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Right) & static_cast<uint8>(EKeyModifiers::CTRL));
			return true;
		default:
			return false;
		}
	}
	else if (Key == EKeys::LeftAlt)
	{
		switch (EventType)
		{
		case IE_Pressed:
			PendingKeyRebind_PressedModifierFlags_Left = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Left) | static_cast<uint8>(EKeyModifiers::ALT));
			return true;
		case IE_Released:
			//PendingKeyRebind_PressedModifierFlags_Left = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Left) & static_cast<uint8>(EKeyModifiers::ALT));
			return true;
		default:
			return false;
		}
	}
	else if (Key == EKeys::RightAlt)
	{
		switch (EventType)
		{
		case IE_Pressed:
			PendingKeyRebind_PressedModifierFlags_Right = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Right) | static_cast<uint8>(EKeyModifiers::ALT));
			return true;
		case IE_Released:
			//PendingKeyRebind_PressedModifierFlags_Right = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Right) & static_cast<uint8>(EKeyModifiers::ALT));
			return true;
		default:
			return false;
		}
	}
	else if (Key == EKeys::LeftShift)
	{
		switch (EventType)
		{
		case IE_Pressed:
			PendingKeyRebind_PressedModifierFlags_Left = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Left) | static_cast<uint8>(EKeyModifiers::SHIFT));
			return true;
		case IE_Released:
			//PendingKeyRebind_PressedModifierFlags_Left = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Left) & static_cast<uint8>(EKeyModifiers::SHIFT));
			return true;
		default:
			return false;
		}
	}
	else if (Key == EKeys::RightShift)
	{
		switch (EventType)
		{
		case IE_Pressed:
			PendingKeyRebind_PressedModifierFlags_Right = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Right) | static_cast<uint8>(EKeyModifiers::SHIFT));
			return true;
		case IE_Released:
			//PendingKeyRebind_PressedModifierFlags_Right = static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Right) & static_cast<uint8>(EKeyModifiers::SHIFT));
			return true;
		default:
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool ARTSPlayerController::TryRebindAction(EKeyMappingAction Action, const FKeyWithModifiers & Key, 
	bool bForce, FTryBindActionResult & OutResult)
{
	const FInputActionInfo & ActionInfo = KeyMappings::ActionInfos[KeyMappings::ActionTypeToArrayIndex(Action)];
	/* I already checked this in the remapping widget code path but checking again in case this 
	is called from a different code path */
	if (ActionInfo.bCanBeRemapped)
	{
		URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
		return GameUserSettings->RemapKeyBinding(Action, Key, bForce, OutResult);
	}
	else
	{
		OutResult.Warning = EGameWarning::NotAllowedToRemapAction;
		return false;
	}
}

bool ARTSPlayerController::TryRebindAction(EKeyMappingAxis Action, const FKeyWithModifiers & Key, bool bForce, FTryBindActionResult & OutResult)
{
	const FInputAxisInfo & ActionInfo = AxisMappings::AxisInfos[AxisMappings::AxisTypeToArrayIndex(Action)];
	if (ActionInfo.bCanBeRemapped)
	{
		URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
		return GameUserSettings->RemapKeyBinding(Action, Key.Key, bForce, OutResult);
	}
	else
	{
		OutResult.Warning = EGameWarning::NotAllowedToRemapAction;
		return false;
	}
}

void ARTSPlayerController::DoOnEveryLMBPress()
{
#if WITH_EDITOR
	Development_bIsLMBPressed = true;
#endif

	bIsLMBPressed = true;
	SetPerformMarqueeNextTick(false);
}

void ARTSPlayerController::DoOnEveryLMBRelease()
{
#if WITH_EDITOR
	Development_bIsLMBPressed = false;
#endif

	bIsLMBPressed = false;
	bIsMarqueeActive = false;
	bWantsMarquee = false;
}

EKeyModifiers ARTSPlayerController::PendingKeyRebind_GetAllKeyModifiers() const
{
	return static_cast<EKeyModifiers>(static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Left) | static_cast<uint8>(PendingKeyRebind_PressedModifierFlags_Right));
}

void ARTSPlayerController::Input_LMBPressed()
{
	/* Some behaviour info: context actions happen the moment
	the mouse is pressed */

	MESSAGE("LMB press called, player id", PS->GetPlayerID());
	MESSAGE("Team number", ENUM_TO_STRING(ETeam, PS->GetTeam()));

#if WITH_EDITOR

	Development_bIsLMBPressed = true;

	/* Check for any development actions we may want to do and if we do one then just stop here */
	if (ExecuteDevelopmentInputInterceptAction_LMBPress())
	{
		return;
	}

#endif // WITH_EDITOR

	DoOnEveryLMBPress();

	if (IsPlacingGhost())
	{
		assert(Statics::IsValid(GhostBuilding));
		
		/* Store this now. If calling GetMouseCoords on tick always results in same result then 
		this can be removed and in tick for the bNeedToRecordGhostLocOnNextTick case we would 
		record mouse coords then not bother calling RotateGhost for that tick */
		MouseLocThisFrame = GetMouseCoords();
		
		bNeedToRecordGhostLocOnNextTick = true;
	}
	else
	{
		/* Single select data */
		FHitResult Hit;
		bool bResult = LineTraceUnderMouse(SELECTABLES_AND_GROUND_CHANNEL, Hit);
		assert(bResult);
		AActor * const HitActor = Hit.GetActor();
		if (Statics::IsValid(HitActor))
		{
			ObjectUnderMouseOnLMB = HitActor;
		}

		if (IsContextActionPending())
		{
			/* TODO: Make ray trace happen before this. Consider moving raycast
			in if statement outside to accomodate this */

			/* Check to see if clicked on ground or something */
			AActor * const ClickTarget = IsASelectable(ObjectUnderMouseOnLMB)
				? ObjectUnderMouseOnLMB
				: nullptr;

			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(PendingContextAction);
			AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);

			/* We'll branch on how the ability is being executed i.e. context menu, inventory
			item use, etc. Item uses implementation is quite special right now because they are
			only issued to the primary selected + they need to check a few extra things */
			if (PendingContextActionUseMethod == EAbilityUsageType::SelectablesInventory)
			{
				/* This block is written with the assumption that only the primary selected
				uses the item */

				ISelectable * TargetAsSelectable = (ClickTarget == nullptr) ? nullptr : CastChecked<ISelectable>(ClickTarget);
				FInventorySlotState * InvSlot = static_cast<FInventorySlotState*>(PendingContextActionAuxilleryDataPtr);
				const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(InvSlot->GetItemType());

				bool bCanUseItem = false;
				EGameWarning Warning = EGameWarning::None;
				EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;
				bool bWarningCancelsPendingAbility = false;

				/* Check this bool. It is flagged whenever the primary selected is killed. We
				could instead cancel the pending action but I prefer the behavior of leaving
				it pending and showing the warning now */
				if (!bHasPrimarySelectedChanged)
				{
					// Not really sure whether clients can assert this
					assert(Statics::IsValid(Selected[0]) && !Statics::HasZeroHealth(Selected[0]));

					/* Check that:
					- inventory slot still has an item in it
					- the inventory slot still has the same type of item in it. Note this check
					isn't checking that the item is the exact same one, just that the type is the
					same. We could have lost the item, then aquired another one of the same type,
					and put it in the slot and this check would still pass.
					- the item still has enough charges to be used
					We do this because users may create abilities that manipulate inventories */
					if (InvSlot->HasItem())
					{
						if (InvSlot->GetItemType() == static_cast<EInventoryItem>(PendingContextActionAuxilleryData))
						{
							if (InvSlot->GetNumCharges() != 0)
							{
								/* Inventory checks over. Now check ability's checks. Hope I
								didn't leave one out */

								if (AbilityInfo.RequiresSelectableTarget())
								{
									if (ClickTarget != nullptr)
									{
										const bool bCheckIfTargetIsSelf = !AbilityInfo.CanTargetSelf() && AbilityInfo.CanTargetFriendlies();
										if (!bCheckIfTargetIsSelf || ClickTarget != Selected[0])
										{
											const bool bIsTargetFriendly = Statics::IsFriendly(ClickTarget, PS->GetTeamTag());

											// Check if target's affiliation is targetable
											const bool bIsTargetableSoFar = (bIsTargetFriendly && AbilityInfo.CanTargetFriendlies())
												|| (!bIsTargetFriendly && AbilityInfo.CanTargetEnemies());

											// Also check that targeting type of target is acceptable
											if (bIsTargetableSoFar && Statics::CanTypeBeTargeted(ClickTarget, AbilityInfo.GetAcceptableTargetFNames()))
											{
												if (CurrentSelected->HasEnoughSelectableResources(AbilityInfo))
												{
													ISelectable * CurrSelected = ToSelectablePtr(CurrentSelected);

													const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();
													const bool bRangeOk = (bCheckRange == false)
														|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, CurrSelected, TargetAsSelectable));
													if (bRangeOk)
													{
														if (EffectActor->IsUsable_SelfChecks(CurrSelected, CustomWarning))
														{
															if (EffectActor->IsUsable_TargetChecks(CurrSelected, TargetAsSelectable, CustomWarning))
															{
																bCanUseItem = true;
															}
															// No else needed here
														}
														// No else needed here
													}
													else
													{
														Warning = EGameWarning::AbilityOutOfRange;
													}
												}
												else
												{
													Warning = EGameWarning::NotEnoughSelectableResources;
												}
											}
											else
											{
												/* Could perhaps go more specific here and be like either:
												"CannotTargetFriendlies" or "CannotTargetEnemies" */
												Warning = EGameWarning::InvalidTarget;
											}
										}
										else
										{
											Warning = EGameWarning::AbilityCannotTargetSelf;
										}
									}
									else
									{
										// Target not a selectable
										Warning = EGameWarning::NoTarget;
									}
								}
								// Ability that requires a location in the world
								else
								{
									if (AbilityInfo.CanLocationBeInsideFog() == true || Statics::IsLocationOutsideFogLocally(Hit.Location, FogOfWarManager) == true)
									{
										if (CurrentSelected->HasEnoughSelectableResources(AbilityInfo))
										{
											ISelectable * CurrSelected = ToSelectablePtr(CurrentSelected);

											const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();
											const bool bRangeOk = (bCheckRange == false)
												|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, CurrSelected, Hit.Location));

											if (bRangeOk)
											{
												if (EffectActor->IsUsable_SelfChecks(CurrSelected, CustomWarning))
												{
													bCanUseItem = true;
												}
												// No else needed here, IsUsable_SelfChecks assigns value to CustomWarning
											}
											else
											{
												Warning = EGameWarning::AbilityOutOfRange;
											}
										}
										else
										{
											Warning = EGameWarning::NotEnoughSelectableResources;
										}
									}
									else
									{
										Warning = EGameWarning::AbilityLocationInsideFog;
									}
								}
							}
							else
							{
								Warning = EGameWarning::ItemOutOfCharges;
								bWarningCancelsPendingAbility = true;
							}
						}
						else
						{
							Warning = EGameWarning::ItemNoLongerInInventory;
							bWarningCancelsPendingAbility = true;
						}
					}
					else
					{
						Warning = EGameWarning::ItemNoLongerInInventory;
						bWarningCancelsPendingAbility = true;
					}
				}
				else
				{
					Warning = EGameWarning::UserNoLongerAlive;
					bWarningCancelsPendingAbility = true;
				}


				//--------------------------------------------------------
				// Final stages - issue command or show warning
				//--------------------------------------------------------

				if (bCanUseItem)
				{
					// Carry out use of item
					if (GetWorld()->IsServer())
					{
						const uint8 InvSlotIndex = static_cast<uint8>(PendingContextActionMoreAuxilleryData);

						if (ClickTarget == nullptr)
						{
							IssueUseInventoryItemCommandChecked(*InvSlot, InvSlotIndex, *ItemsInfo, AbilityInfo,
								Hit.Location);
						}
						else
						{
							IssueUseInventoryItemCommandChecked(*InvSlot, InvSlotIndex, *ItemsInfo, AbilityInfo,
								TargetAsSelectable);
						}
					}
					else
					{
						// Send RPC to server
						// Use ClickTarget to decide which type of ability it was 
						if (ClickTarget == nullptr)
						{
							Server_IssueLocationTargetingUseInventoryItemCommand(CurrentSelected->GetSelectableID(),
								PendingContextActionMoreAuxilleryData, InvSlot->GetItemType(), Hit.Location);
						}
						else
						{
							Server_IssueSelectableTargetingUseInventoryItemCommand(CurrentSelected->GetSelectableID(),
								PendingContextActionMoreAuxilleryData, InvSlot->GetItemType(), ClickTarget);
						}
					}

					UnhighlightHighlightedButton();
					ResetMouseAppearance();
					PendingContextAction = EContextButton::RecentlyExecuted;
				}
				else
				{
					// Show warning
					if (Warning != EGameWarning::None)
					{
						PS->OnGameWarningHappened(Warning);
					}
					else // Assumed custom warning then
					{
						PS->OnGameWarningHappened(CustomWarning);
					}

					if (bWarningCancelsPendingAbility)
					{
						// Reset mouse appearance and cancel
						CancelPendingContextCommand();
					}
				}
			}
			else // Assumed SelectablesContextMenu
			{
				assert(PendingContextActionUseMethod == EAbilityUsageType::SelectablesActionBar);

				if (AbilityInfo.RequiresSelectableTarget())
				{
					if (ClickTarget != nullptr)
					{
						const bool bIsTargetFriendly = Statics::IsFriendly(ClickTarget, PS->GetTeamTag());

						/* Check if our affiliation is targetable */
						bool bIsTargetableSoFar = (bIsTargetFriendly && AbilityInfo.CanTargetFriendlies())
							|| (!bIsTargetFriendly && AbilityInfo.CanTargetEnemies());

						/* Check if target is an acceptable target for ability, both its affiliation
						and targeting type */
						if (bIsTargetableSoFar && Statics::CanTypeBeTargeted(ClickTarget, AbilityInfo.GetAcceptableTargetFNames()))
						{
							/* Check custom checks the ability requires. Check against all the selected selectables
							that have this ability on their context menu. If at least one of them can
							successfully use it then we will issue an order to use it */
							ISelectable * TargetAsSelectable = CastChecked<ISelectable>(ClickTarget);
							/* This bool is true if the ability is one where the user will not move
							closer to their target before using it i.e. they must be in range now */
							const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();
							const bool bCheckIfTargetIsSelf = !AbilityInfo.CanTargetSelf() && AbilityInfo.CanTargetFriendlies();
							/* There are two ways this is done depending on whether we are the server
							or a remove client. If we're the server we also save which selected
							selectables can use the ability and pass this on to a command issuing
							func. If we're a remote client there's no point saving which selectables
							can use the ability since the server will check them anyway. So once we
							know at least one selectable can use the ability we just go straight on
							to calling the command issuing RPC. */
							if (GetWorld()->IsServer())
							{
								TArray < ISelectable * > CanUseAbility;

								if (AbilityInfo.IsIssuedToAllSelected())
								{
									// Worst case allocation
									CanUseAbility.Reserve(SelectedAndSupportPendingContextAction.Num());

									for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
									{
										AActor * Elem = SelectedAndSupportPendingContextAction[i];

										if (Statics::IsValid(Elem))
										{
											ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

											const bool bSelfTargetOk = (bCheckIfTargetIsSelf == false)
												|| (bCheckIfTargetIsSelf == true && Elem != ClickTarget);

											if (AsSelectable->HasEnoughSelectableResources(AbilityInfo))
											{
												const bool bRangeOk = (bCheckRange == false)
													|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, TargetAsSelectable));

												if (bSelfTargetOk && bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable)
													&& EffectActor->IsUsable_TargetChecks(AsSelectable, TargetAsSelectable))
												{
													CanUseAbility.Emplace(AsSelectable);
												}
											}
										}
										else
										{
											/* Book keep and remove non-valids as we go */
											SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
										}
									}
								}
								else
								{
									for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
									{
										AActor * Elem = SelectedAndSupportPendingContextAction[i];

										if (Statics::IsValid(Elem))
										{
											ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

											const bool bSelfTargetOk = (bCheckIfTargetIsSelf == false)
												|| (bCheckIfTargetIsSelf == true && Elem != ClickTarget);

											if (AsSelectable->HasEnoughSelectableResources(AbilityInfo))
											{
												const bool bRangeOk = (bCheckRange == false)
													|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, TargetAsSelectable));

												if (bSelfTargetOk && bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable)
													&& EffectActor->IsUsable_TargetChecks(AsSelectable, TargetAsSelectable))
												{
													CanUseAbility.Emplace(AsSelectable);

													/* Ability is only issued to one selectable so stop here */
													break;
												}
											}
										}
										else
										{
											/* Book keep and remove non-valids as we go */
											SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
										}
									}
								}

								if (CanUseAbility.Num() > 0)
								{
									UnhighlightHighlightedButton();
									ResetMouseAppearance();

									/* This function should just issue the commands without checking
									anything. Also it's server only */
									IssueTargetRequiredContextCommandChecked(AbilityInfo, Hit.Location,
										ClickTarget, CanUseAbility);

									// What was previously here before
									//OnContextCommandIssued(PendingContextAction, Hit.Location, ClickTarget);

									PendingContextAction = EContextButton::RecentlyExecuted;
								}
								else
								{
									// Not accurate
									PS->OnGameWarningHappened(EGameWarning::InvalidTarget);
								}
							}
							else
							{
								for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
								{
									AActor * Elem = SelectedAndSupportPendingContextAction[i];

									if (Statics::IsValid(Elem))
									{
										ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

										const bool bSelfTargetOk = (bCheckIfTargetIsSelf == false)
											|| (bCheckIfTargetIsSelf == true && Elem != ClickTarget);

										if (AsSelectable->HasEnoughSelectableResources(AbilityInfo))
										{
											const bool bRangeOk = (bCheckRange == false)
												|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, TargetAsSelectable));

											if (bSelfTargetOk && bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable)
												&& EffectActor->IsUsable_TargetChecks(AsSelectable, TargetAsSelectable))
											{
												UnhighlightHighlightedButton();
												ResetMouseAppearance();

												PrepareContextCommandRPC(PendingContextAction, Hit.Location, ClickTarget);

												PendingContextAction = EContextButton::RecentlyExecuted;

												/* Once we know one selectable can do the action we stop here.
												It is enough to send RPC. Server will decide what units can
												and cannot do the ability.
												OnContextCommandIssued will send RPC to make command happen */
												break;
											}
										}
									}
									else
									{
										// Book keep and remove non-valids along the way
										SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
									}
								}
							}
						}
						else
						{
							PS->OnGameWarningHappened(EGameWarning::InvalidTarget);
						}
					}
					else
					{
						// Not a selectable
						PS->OnGameWarningHappened(EGameWarning::NoTarget);
					}
				}
				else
				{
					if (AbilityInfo.CanLocationBeInsideFog() == false)
					{
						// Possibly wrong func? Use IsLocationVisible(PS->GetTeam()) instead
						if (Statics::IsLocationOutsideFogLocally(Hit.Location, FogOfWarManager) == false)
						{
							PS->OnGameWarningHappened(EGameWarning::AbilityLocationInsideFog);

							return;
						}
					}

					const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();

					if (GetWorld()->IsServer())
					{
						TArray < ISelectable * > CanUseAbility;

						if (AbilityInfo.IsIssuedToAllSelected())
						{
							// Worst case allocation
							CanUseAbility.Reserve(SelectedAndSupportPendingContextAction.Num());

							for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
							{
								AActor * Elem = SelectedAndSupportPendingContextAction[i];

								if (Statics::IsValid(Elem))
								{
									ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

									if (AsSelectable->HasEnoughSelectableResources(AbilityInfo))
									{
										const bool bRangeOk = (bCheckRange == false)
											|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, Hit.Location));

										if (bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable))
										{
											CanUseAbility.Emplace(AsSelectable);
										}
									}
								}
								else
								{
									// Book keep and remove non-valids along the way
									SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
								}
							}
						}
						else
						{
							if (CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::First)
							{
								for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
								{
									AActor * Elem = SelectedAndSupportPendingContextAction[i];

									if (Statics::IsValid(Elem))
									{
										ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

										if (AsSelectable->HasEnoughSelectableResources(AbilityInfo))
										{
											const bool bRangeOk = (bCheckRange == false)
												|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, Hit.Location));

											if (bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable))
											{
												CanUseAbility.Emplace(AsSelectable);

												/* Ability is only issued to one selectable so stop here */
												break;
											}
										}
									}
									else
									{
										// Book keep and remove non-valids along the way
										SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
									}
								}
							}
							else // Assumed Closest
							{
								assert(CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::Closest);

								ISelectable * BestSelectable = nullptr;
								float BestDistanceSqr = FLT_MAX;
								for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
								{
									AActor * Elem = SelectedAndSupportPendingContextAction[i];

									if (Statics::IsValid(Elem))
									{
										ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

										if (AsSelectable->HasEnoughSelectableResources(AbilityInfo))
										{
											if (EffectActor->IsUsable_SelfChecks(AsSelectable))
											{
												const float DistanceSqr = Statics::GetSelectablesDistanceForAbilitySquared(AbilityInfo, AsSelectable, Hit.Location);

												if (DistanceSqr < BestDistanceSqr)
												{
													BestSelectable = AsSelectable;
													BestDistanceSqr = DistanceSqr;
												}
											}
										}
									}
									else
									{
										// Book keep and remove non-valids along the way
										SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
									}
								}

								if (bCheckRange)
								{
									if (Statics::IsSelectableInRangeForAbility(AbilityInfo, BestSelectable, Hit.Location))
									{
										CanUseAbility.Emplace(BestSelectable);
									}
									else
									{
										// Nothing was in range
									}
								}
								else
								{
									CanUseAbility.Emplace(BestSelectable);
								}
							}
						}

						if (CanUseAbility.Num() > 0)
						{
							UnhighlightHighlightedButton();
							ResetMouseAppearance();

							/* This function should just issue the commands without checking
							anything. Also it's server only */
							IssueLocationRequiredContextCommandChecked(AbilityInfo, Hit.Location,
								CanUseAbility);

							PendingContextAction = EContextButton::RecentlyExecuted;
						}
						else
						{
							/* Very broad type of message. Cannot be more specific */
							PS->OnGameWarningHappened(EGameWarning::CannotUseAbility);
						}
					}
					else
					{
						/* Look for at least one selectable that can use this ability */
						for (int32 i = SelectedAndSupportPendingContextAction.Num() - 1; i >= 0; --i)
						{
							AActor * Elem = SelectedAndSupportPendingContextAction[i];

							if (Statics::IsValid(Elem))
							{
								ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

								if (AsSelectable->HasEnoughSelectableResources(AbilityInfo))
								{
									const bool bRangeOk = (bCheckRange == false)
										|| (bCheckRange == true && Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, Hit.Location));

									if (bRangeOk && EffectActor->IsUsable_SelfChecks(AsSelectable))
									{
										UnhighlightHighlightedButton();
										ResetMouseAppearance();

										PrepareContextCommandRPC(PendingContextAction, Hit.Location);

										PendingContextAction = EContextButton::RecentlyExecuted;

										/* Knowing at least one selectable can use the ability is enough
										to send RPC - no need to check others */
										break;
									}
								}
							}
							else
							{
								/* Book keep and remove non-valids along the way */
								SelectedAndSupportPendingContextAction.RemoveAtSwap(i, 1, false);
							}
						}
					}
				}
			}
		}
		else if (IsGlobalSkillsPanelAbilityPending())
		{
			/* We traced on both the selectables and ground channel earlier. Probably want 
			to delay doing the trace until we know what the ability targets */

			const FCommanderAbilityInfo * AbilityInfo = PendingCommanderAbility->GetAbilityInfo();
			if (AbilityInfo->GetTargetingMethod() == EAbilityTargetingMethod::RequiresSelectable)
			{
				if (Statics::IsValid(HitActor))
				{
					if (IsASelectable(HitActor))
					{
						ISelectable * HitActorAsSelectable = CastChecked<ISelectable>(HitActor);
						const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(PendingCommanderAbility, HitActor, HitActorAsSelectable, true);
						if (Warning == EGameWarning::None)
						{
							if (GetWorld()->IsServer())
							{
								UnhighlightHighlightedButton();
								ResetMouseAppearance();

								ExecuteCommanderAbility(PendingCommanderAbility, *AbilityInfo, HitActor, HitActorAsSelectable);

								PendingCommanderAbility = nullptr;
								
								/* Do this so on LMB release we don't select something */
								PendingContextAction = EContextButton::RecentlyExecuted;
							}
							else
							{
								UnhighlightHighlightedButton();
								ResetMouseAppearance();

								// RPC to server requesting it be carried out
								Server_RequestExecuteCommanderAbility_SelectableTargeting(AbilityInfo->GetType(), HitActor);

								PendingCommanderAbility = nullptr;

								/* Do this so on LMB release we don't select something */
								PendingContextAction = EContextButton::RecentlyExecuted;
							}
						}
						else
						{
							PS->OnGameWarningHappened(Warning);
						}
					}
					else
					{
						// Did not press on a selectable. Do we want to show a warning here?
					}
				}
			}
			else if (AbilityInfo->GetTargetingMethod() == EAbilityTargetingMethod::RequiresWorldLocation)
			{
				if (Statics::IsValid(HitActor))
				{
					const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(PendingCommanderAbility, Hit.Location, true);
					if (Warning == EGameWarning::None)
					{
						if (GetWorld()->IsServer())
						{
							UnhighlightHighlightedButton();
							ResetMouseAppearance();

							ExecuteCommanderAbility(PendingCommanderAbility, *AbilityInfo, Hit.Location);

							PendingCommanderAbility = nullptr;

							/* Do this so on LMB release we don't select something */
							PendingContextAction = EContextButton::RecentlyExecuted;
						}
						else
						{
							UnhighlightHighlightedButton();
							ResetMouseAppearance();

							Server_RequestExecuteCommanderAbility_LocationTargeting(AbilityInfo->GetType(), Hit.Location);

							PendingCommanderAbility = nullptr;

							/* Do this so on LMB release we don't select something */
							PendingContextAction = EContextButton::RecentlyExecuted;
						}
					}
					else
					{
						PS->OnGameWarningHappened(Warning);
					}
				}
			}
			else if (AbilityInfo->GetTargetingMethod() == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable)
			{
				if (Statics::IsValid(HitActor))
				{
					if (IsASelectable(HitActor))
					{
						ISelectable * HitActorAsSelectable = CastChecked<ISelectable>(HitActor);
						const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(PendingCommanderAbility, HitActor, HitActorAsSelectable, true);
						if (Warning == EGameWarning::None)
						{
							if (GetWorld()->IsServer())
							{
								UnhighlightHighlightedButton();
								ResetMouseAppearance();

								ExecuteCommanderAbility(PendingCommanderAbility, *AbilityInfo, Hit.Location, HitActor, HitActorAsSelectable);

								PendingCommanderAbility = nullptr;

								/* Do this so on LMB release we don't select something */
								PendingContextAction = EContextButton::RecentlyExecuted;
							}
							else
							{
								UnhighlightHighlightedButton();
								ResetMouseAppearance();

								// RPC to server requesting it be carried out
								Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingSelectable(AbilityInfo->GetType(), HitActor);

								PendingCommanderAbility = nullptr;

								/* Do this so on LMB release we don't select something */
								PendingContextAction = EContextButton::RecentlyExecuted;
							}
						}
						else
						{
							PS->OnGameWarningHappened(Warning);
						}
					}
					else // Did not press on selectable
					{
						const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(PendingCommanderAbility, Hit.Location, true);
						if (Warning == EGameWarning::None)
						{
							if (GetWorld()->IsServer())
							{
								UnhighlightHighlightedButton();
								ResetMouseAppearance();

								ExecuteCommanderAbility(PendingCommanderAbility, *AbilityInfo, Hit.Location, nullptr, nullptr);

								PendingCommanderAbility = nullptr;

								/* Do this so on LMB release we don't select something */
								PendingContextAction = EContextButton::RecentlyExecuted;
							}
							else
							{
								UnhighlightHighlightedButton();
								ResetMouseAppearance();

								// RPC to server requesting it be carried out
								Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingLocation(AbilityInfo->GetType(), Hit.Location);

								PendingCommanderAbility = nullptr;

								/* Do this so on LMB release we don't select something */
								PendingContextAction = EContextButton::RecentlyExecuted;
							}
						}
						else
						{
							PS->OnGameWarningHappened(Warning);
						}
					}
				}
			}
		}
		else
		{
			/* Send click location to MarqueeHUD */
			MarqueeHUD->SetClickLocation(GetMouseCoords());
			bWantsMarquee = true;
		}
	}
}

void ARTSPlayerController::Input_LMBReleased()
{
	assert(HUD != nullptr);

#if WITH_EDITOR

	Development_bIsLMBPressed = false;

	/* Check for any development actions we may want to do and if we do one then just stop here */
	if (ExecuteDevelopmentInputInterceptAction_LMBRelease())
	{
		return;
	}

#endif // WITH_EDITOR

	DoOnEveryLMBRelease();
	if (ButtonPressedOnLMBPressed != nullptr)
	{
		ButtonPressedOnLMBPressed->SetIsPressedByLMB(false);
		ButtonPressedOnLMBPressed = nullptr;
	}

	if (IsPlacingGhost())
	{
		bNeedToRecordGhostLocOnNextTick = false;
		bIsGhostRotationActive = false;
#if GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD
		// @See CancelGhost
		GhostAccumulatedMovementTowardsDirection = 0.f;
#endif

		const EBuildingType GhostType = GhostBuilding->GetType();
		const FBuildingInfo * BuildingInfo = FactionInfo->GetBuildingInfo(GhostType);
		const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

		/* Either:
		- give a command to move to the build site and lay foundations
		- place building now
		depending on the build method */
		if (BuildMethod == EBuildingBuildMethod::LayFoundationsWhenAtLocation
			|| BuildMethod == EBuildingBuildMethod::Protoss)
		{
			const FVector Location = GhostBuilding->GetActorLocation();
			const FRotator Rotation = GhostBuilding->GetActorRotation();

			if (CanPlaceBuilding(GhostType, CurrentSelected->GetSelectableID(),
				Location, Rotation, true))
			{
				// Destroy ghost. But perhaps create a ghost on world also like in SCII
				CancelGhost();

				// Possibly play a sound. Will need a currentSelected null/zeroHealth check here probably too
				if (ShouldPlayCommandSound(CurrentSelected, EContextButton::BuildBuilding))
				{
					PlayCommandSound(CurrentSelected, EContextButton::BuildBuilding);
				}

				// Give command 
				Server_GiveLayFoundationCommand(GhostType, Location, Rotation,
					CurrentSelected->GetSelectableID());
			}
			else
			{
				// Warning should have been handled by CanPlaceBuilding. Keep ghost
			}
		}
		else
		{
			/* Server will do checks anyway later and does not need to do them now */
			if (GetWorld()->IsServer())
			{
				Server_PlaceBuilding(GhostType, GhostBuilding->GetActorLocation(),
					GhostBuilding->GetActorRotation(), GhostInstigatorID);
			}
			// Clients
			else if (CanPlaceBuilding(GhostType, GhostInstigatorID,
				GhostBuilding->GetActorLocation(), GhostBuilding->GetActorRotation(), true))
			{
				/* Possibly play a sound. I think best behavior is play sound before result comes through
				just like other commands. Chance of server rejecting the command is rare anyway */
				// Check if it's a unit instigating this
				if (GhostInstigatorID != 0 && CurrentSelected->GetInfantryAttributes() != nullptr)
				{
					// Will need null/ZeroHealth check here probs too
					if (ShouldPlayCommandSound(CurrentSelected, EContextButton::BuildBuilding))
					{
						PlayCommandSound(CurrentSelected, EContextButton::BuildBuilding);
					}
				}

				// RPC to place building
				Server_PlaceBuilding(GhostType, GhostBuilding->GetActorLocation(),
					GhostBuilding->GetActorRotation(), GhostInstigatorID);
			}
			else
			{
				// Building cannot be placed here
			}
		}
	}
	else if (IsContextActionPending())
	{
		/* This is here to prevent losing selection when clicking somewhere/someone to use an
		ability but the target/location/whatever is not valid and therefore the ability is not used */
	}
	else if (PendingContextAction == EContextButton::RecentlyExecuted)
	{
		/* On LMB press a context action was executed */

		PendingContextAction = EContextButton::None;
	}
	else if (IsGlobalSkillsPanelAbilityPending())
	{
		// Do nothing for the same reason as IsContextActionPending() block
	}
	else if (WasMouseClick())
	{
		FHitResult Hit;
		LineTraceUnderMouse(SELECTABLES_CHANNEL, Hit);
		AActor * const HitActor = Hit.GetActor();
		/* Unless clicking outside the map HitActor should always be
		valid */
		if (HitActor != nullptr)
		{
			if (HitActor == ObjectUnderMouseOnLMB)
			{
				if (IsASelectable(HitActor) && Statics::CanBeSelected(HitActor))
				{
					SingleSelect(HitActor);
				}
				else
				{
					/* Something like the ground was clicked on. Remove all selections */
					RemoveSelection();
					HUD->OnPlayerNoSelection(); 
				}
			}
		}
		else
		{
			/* Trace hit nothing. We'll remove selection anyway. */
			RemoveSelection();
			HUD->OnPlayerNoSelection();
		}
	}
	else
	{
		SetPerformMarqueeNextTick(true);
	}

	MouseMovement = 0.f;
}

void ARTSPlayerController::Input_RMBPressed()
{
#if WITH_EDITOR
	if (ExecuteDevelopmentInputInterceptAction_RMBPress())
	{
		return;
	}
#endif

	/* TODO: implement some behaviour for when RMB is pressed while
	doing marquee select */

	if (IsPlacingGhost())
	{
		CancelGhost();
	}
	else if (HasSelection())
	{
		if (IsSelectionControlledByThis())
		{
			if (CurrentSelected->GetAttributesBase().GetBuildingType() != EBuildingType::NotBuilding 
				&& IsContextActionPending() == false)
			{
				// Have a building selected and context action not pending. Want to try set its rally point

				ABuilding * Building = CastChecked<ABuilding>(Selected[0]);

				if (Building->CanChangeRallyPoint())
				{
					FHitResult Hit;
					if (LineTraceUnderMouse(GROUND_CHANNEL, Hit))
					{
						const FVector_NetQuantize & ClickLoc = Hit.ImpactPoint;

						if (GetWorld()->IsServer())
						{
							Building->ChangeRallyPointLocation(ClickLoc);
						}
						else
						{
							/* Before sending RPC check if location has changed enough. This is to
							save on bandwidth if clicking same location multiple times. */
							if (FVector::DistSquared(ClickLoc, Building->GetRallyPointLocation()) > FMath::Square(16.f))
							{
								/* Send RPC to change location */
								Building->Server_ChangeRallyPointLocation(ClickLoc);
							}
						}

						// Possibly play a sound
						if (ShouldPlayChangeRallyPointSound())
						{
							PlayChangeRallyPointSound();
						}
					}
				}
			}
			else
			{
				// Assumed have unit(s) selected

				FHitResult Hit;
				LineTraceUnderMouse(SELECTABLES_AND_GROUND_CHANNEL, Hit);
				AActor * HitActor = Hit.GetActor();
				if (Statics::IsValid(HitActor))
				{
					// Null HitActor if not a selectable or is in fog or invisible
					if (!IsASelectable(HitActor) || !Statics::CanBeSelected(HitActor))
					{
						HitActor = nullptr;
					}

					/* Check must come after IsPlacingGhost */
					if (IsContextActionPending())
					{
						CancelPendingContextCommand();
					}
					else if (IsGlobalSkillsPanelAbilityPending())
					{
						CancelPendingGlobalSkillsPanelAbility();
					}
					else
					{
						/* I do this earlier in the func, 99% sure redundant calling it again here */
						//if (IsSelectionControlledByThis())
						//{
						OnRightClickCommand(Hit.Location, HitActor);
						//}
					}
				}
			}
		}
	}
	else if (IsGlobalSkillsPanelAbilityPending())
	{
		CancelPendingGlobalSkillsPanelAbility();
	}
}

void ARTSPlayerController::Input_RMBReleased()
{
	if (ButtonPressedOnRMBPressed != nullptr)
	{
		ButtonPressedOnRMBPressed->SetIsPressedByRMB(false);
		ButtonPressedOnRMBPressed = nullptr;
	}
}

void ARTSPlayerController::Axis_MoveCameraRight(float Value)
{
	if (Value != 0.f)
	{
		bIsLeftRightCameraMovementKeyPressed = true;

		float Multiplier = 1.f;

		// Adjust for zoom amount
		Multiplier *= GetCameraMovementMultiplierDueToZoom();

		if (bIsForwardBackwardCameraMovementKeyPressed)
		{
			Multiplier *= 1.f / FMath::Sqrt(2.f);
			Multiplier *= URTSGameUserSettings::KEYBOARD_DIAGONAL_MOVEMENT_SPEED_MULTIPLIER;
		}

		MoveCameraRight(CameraMoveSpeed * Value * Multiplier);
	}
	else
	{
		bIsLeftRightCameraMovementKeyPressed = false;
	}
}

void ARTSPlayerController::Axis_MoveCameraForward(float Value)
{
	if (Value != 0.f)
	{
		bIsForwardBackwardCameraMovementKeyPressed = true;

		float Multiplier = 1.f;

		// Adjust for zoom amount i.e. the further we're zoomed in the less we move
		Multiplier *= GetCameraMovementMultiplierDueToZoom();

		if (bIsLeftRightCameraMovementKeyPressed)
		{
			Multiplier *= 1.f / FMath::Sqrt(2.f);
			Multiplier *= URTSGameUserSettings::KEYBOARD_DIAGONAL_MOVEMENT_SPEED_MULTIPLIER;
		}

		MoveCameraForward(CameraMoveSpeed * Value * Multiplier);
	}
	else
	{
		bIsForwardBackwardCameraMovementKeyPressed = false;
	}
}

void ARTSPlayerController::Input_ZoomCameraIn()
{
	// TODO get multiple scroll zooming in/out to work much better

	/* Do not allow wheel zooming while camera is resetting to default rot/zoom */
	if (bIsResettingCameraZoom)
	{
		return;
	}

	// If already zoomed in as far as we can go then return
	if (SpringArm->TargetArmLength == URTSGameUserSettings::MIN_CAMERA_ZOOM_AMOUNT)
	{
		return;
	}

	if (NumPendingScrollZooms == 0)
	{
		NumPendingScrollZooms--;

		// Set this and let lerp in tick do zoom
		StartZoomAmount = SpringArm->TargetArmLength;
		TargetZoomAmount -= CameraZoomIncrementalAmount;
		TargetZoomAmount = FMath::Max<float>(URTSGameUserSettings::MIN_CAMERA_ZOOM_AMOUNT, TargetZoomAmount);
	}
	else if (NumPendingScrollZooms < 0)
	{
		NumPendingScrollZooms--;

		TargetZoomAmount -= CameraZoomIncrementalAmount;
		TargetZoomAmount = FMath::Max<float>(URTSGameUserSettings::MIN_CAMERA_ZOOM_AMOUNT, TargetZoomAmount);
	}
	else
	{
		/* We have scrolled in the opposite direction of the way the camera was zooming e.g. we
		scrolled up when the player had recently scrolled down and the camera has not completed
		zooming out. Behavior I have chosen here is that that scroll will cause the camera to
		stop where it is */

		NumPendingScrollZooms = 0;
		TargetZoomAmount = SpringArm->TargetArmLength;
	}
}

void ARTSPlayerController::Input_ZoomCameraOut()
{
	/* Do not allow wheel zooming while camera is resetting to default rot/zoom */
	if (bIsResettingCameraZoom)
	{
		return;
	}

	// If already zoomed out as far as we can go then return
	if (SpringArm->TargetArmLength == URTSGameUserSettings::MAX_CAMERA_ZOOM_AMOUNT)
	{
		return;
	}

	if (NumPendingScrollZooms == 0)
	{
		NumPendingScrollZooms++;

		// Set this and let lerp in tick do zoom
		StartZoomAmount = SpringArm->TargetArmLength;
		TargetZoomAmount += CameraZoomIncrementalAmount;
		TargetZoomAmount = FMath::Min<float>(URTSGameUserSettings::MAX_CAMERA_ZOOM_AMOUNT, TargetZoomAmount);
	}
	else if (NumPendingScrollZooms > 0)
	{
		NumPendingScrollZooms++;

		TargetZoomAmount += CameraZoomIncrementalAmount;
		TargetZoomAmount = FMath::Min<float>(URTSGameUserSettings::MAX_CAMERA_ZOOM_AMOUNT, TargetZoomAmount);
	}
	else // Have scrolled in opposite direction camera was zooming towards 
	{
		NumPendingScrollZooms = 0;
		TargetZoomAmount = SpringArm->TargetArmLength;
	}
}

void ARTSPlayerController::Input_EnableCameraFreeLook()
{
	bIsCameraFreeLookEnabled = true;
}

void ARTSPlayerController::Input_DisableCameraFreeLook()
{
	bIsCameraFreeLookEnabled = false;
}

void ARTSPlayerController::Input_ResetCameraRotationToOriginal()
{
	// Do nothing if already resetting rotation/zoom
	if (!ShouldResetCameraRotation())
	{
		return;
	}

	// Like saying "reset timeline"
	MouseWheelZoomCurveAccumulatedTime = 0.f;

	bIsResettingCameraRotation = true;

	// Leave lerp in tick change rotation
	StartResetRotation = ControlRotation;
	if (StartResetRotation.Yaw >= 180.f)
	{
		/* This helps the lerp in tick always choose fastest direction instead of always going
		counter clockwise */
		StartResetRotation -= FRotator(0.f, 360.f, 0.f);
	}
	TargetResetRotation = DefaultCameraRotation;
}

void ARTSPlayerController::Input_ResetCameraZoomToOriginal()
{
	// Do nothing if already resetting rotation/zoom
	if (!ShouldResetCameraZoom())
	{
		return;
	}

	MouseWheelZoomCurveAccumulatedTime = 0.f;

	bIsResettingCameraZoom = true;

	// Leave lerp in tick change zoom
	StartZoomAmount = SpringArm->TargetArmLength;
	TargetZoomAmount = DefaultCameraZoomAmount;
}

void ARTSPlayerController::Input_ResetCameraRotationAndZoomToOriginal()
{
	// Do nothing if already resetting rotation/zoom
	if (!ShouldResetCameraZoom() && !ShouldResetCameraRotation())
	{
		return;
	}

	MouseWheelZoomCurveAccumulatedTime = 0.f;

	bIsResettingCameraRotation = true;

	StartResetRotation = ControlRotation;
	if (StartResetRotation.Yaw >= 180.f)
	{
		/* This helps the lerp in tick always choose fastest direction instead of always going
		counter clockwise */
		StartResetRotation -= FRotator(0.f, 360.f, 0.f);
	}
	TargetResetRotation = DefaultCameraRotation;

	bIsResettingCameraZoom = true;
	StartZoomAmount = SpringArm->TargetArmLength;
	TargetZoomAmount = DefaultCameraZoomAmount;
}

void ARTSPlayerController::Input_OpenTeamChat()
{
	if (IsChatInputOpen())
	{
		// Tell HUD to send what is typed into chat input
		//@See Input_OpenAllChat
		//HUD->SendChatMessage();
	}
	else
	{
		OpenChatInput(EMessageRecipients::TeamOnly);
	}
}

void ARTSPlayerController::Input_OpenAllChat()
{
	if (IsChatInputOpen())
	{
		/* Right now the text commit func for the text input widget will send a message, so
		perhaps do nothing since don't want to send message again, although maybe think of
		some behavior since if the chat input widget doesn't have focus then we will be
		doing nothing. However I think if enter is pressed while the chat input has focus
		then this key mapping func will not be called so should be ok to send message in
		this func - it shouldn't send twice */

		// Tell HUD to send what is typed into chat input. If this key is bound to Enter then 
		// it makes sense but it might not. Not sure whether anything should be done here 
		//HUD->SendChatMessage();
	}
	else
	{
		OpenChatInput(EMessageRecipients::Everyone);
	}
}

void ARTSPlayerController::Input_Cancel()
{
	/* Bear in mind this is not bound until match starts so this will never be called if in
	main menu/lobby. Same with all other input funcs */

	/* ********************************************************** */
	/* Leaving out the ability cancelling code. I don't like it */
	/* ********************************************************** */
	//if (IsPlacingGhost())
	//{
	//	CancelGhost();
	//}
	//else if (IsContextActionPending())
	//{
	//	/* This line explains why the player targeting panel is always the first widget 
	//	to hide */
	//	CancelPendingContextCommand();
	//
	//	/* It's odd. When I cancel an ability like this the mouse cursor is hidden until 
	//	I move it. However when I cancel a pending context command by right clicking 
	//	it appears instantly like it should */
	//}
	//else if (IsGlobalSkillsPanelAbilityPending())
	//{
	//	CancelPendingGlobalSkillsPanelAbility();
	//}
	/* Check if any menus are open that the HUD wants to close */
	/*else*/if (HUD->TryCloseEscapableWidget())
	{
		// HUD closed a menu. My rule is 'one menu close per ESC press' so return
		return;
	}
	else
	{
		// HUD->TryCloseEscapableWidget() would have returned true if the pause menu was showing so assert this
		assert(HUD->IsPauseMenuShowingOrPlayingShowAnimation() == false);

		// Show the pause menu
		PauseGameAndShowPauseMenu();
	}
}

void ARTSPlayerController::Input_QuitGame()
{
	/* Instantly quit game */
	URTSGameInstance::QuitGame(GetWorld());
}

void ARTSPlayerController::Input_ToggleDevelopmentCheatWidget()
{
#if WITH_EDITOR
	/* Can be null if no BP was set for it */
	if (DevelopmentCheatWidget != nullptr)
	{
		/* Toggle visibility of the development cheat widget */
		if (DevelopmentCheatWidget->GetVisibility() == ESlateVisibility::Hidden)
		{
			DevelopmentCheatWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else // Assuming SelfHitTestInvisible
		{
			assert(DevelopmentCheatWidget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);

			DevelopmentCheatWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
#endif
}

void ARTSPlayerController::Input_OpenCommanderSkillTree()
{
	HUD->TryShowCommanderSkillTree();
}

void ARTSPlayerController::Input_CloseCommanderSkillTree()
{
	HUD->TryHideCommanderSkillTree();
}

void ARTSPlayerController::Input_ToggleCommanderSkillTree()
{
	HUD->OnToggleCommanderSkillTreeVisibilityKeyPressed();
}

void ARTSPlayerController::Input_AssignControlGroupButtonPressed_0() { OnAssignControlGroupButtonPressed(0); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_1() { OnAssignControlGroupButtonPressed(1); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_2() { OnAssignControlGroupButtonPressed(2); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_3() { OnAssignControlGroupButtonPressed(3); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_4() { OnAssignControlGroupButtonPressed(4); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_5() { OnAssignControlGroupButtonPressed(5); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_6() { OnAssignControlGroupButtonPressed(6); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_7() { OnAssignControlGroupButtonPressed(7); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_8() { OnAssignControlGroupButtonPressed(8); }
void ARTSPlayerController::Input_AssignControlGroupButtonPressed_9() { OnAssignControlGroupButtonPressed(9); }

void ARTSPlayerController::Input_SelectControlGroupButtonPressed_0() { OnSelectControlGroupButtonPressed(0); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_1() { OnSelectControlGroupButtonPressed(1); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_2() { OnSelectControlGroupButtonPressed(2); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_3() { OnSelectControlGroupButtonPressed(3); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_4() { OnSelectControlGroupButtonPressed(4); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_5() { OnSelectControlGroupButtonPressed(5); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_6() { OnSelectControlGroupButtonPressed(6); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_7() { OnSelectControlGroupButtonPressed(7); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_8() { OnSelectControlGroupButtonPressed(8); }
void ARTSPlayerController::Input_SelectControlGroupButtonPressed_9() { OnSelectControlGroupButtonPressed(9); }

void ARTSPlayerController::Input_SelectControlGroupButtonReleased_0() { OnSelectControlGroupButtonReleased(0); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_1() { OnSelectControlGroupButtonReleased(1); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_2() { OnSelectControlGroupButtonReleased(2); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_3() { OnSelectControlGroupButtonReleased(3); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_4() { OnSelectControlGroupButtonReleased(4); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_5() { OnSelectControlGroupButtonReleased(5); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_6() { OnSelectControlGroupButtonReleased(6); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_7() { OnSelectControlGroupButtonReleased(7); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_8() { OnSelectControlGroupButtonReleased(8); }
void ARTSPlayerController::Input_SelectControlGroupButtonReleased_9() { OnSelectControlGroupButtonReleased(9); }

void ARTSPlayerController::On_Mouse_Move_X(float Value)
{
	/* This logic may best be moved to tick. HUD only draws when
	it ticks so there isn't much point updating its rectangle info
	every time the mouse moves, but I should test this and see if
	sending data in here feels any more responsive than sending it
	in tick */

	/* Intersting... if a mouse press is on a widget that is visible then this will 
	not be called during that press */

	if (Value != 0.f)
	{
		if (bWantsMarquee /*bIsLMBPressed && !IsContextActionPending() <-- Old code*/)
		{
			MouseMovement += FMath::Abs(Value);
			SendMouseLocToMarqueeHUD();
			bIsMarqueeActive = true;
		}
		/* Only change rot if MMB pressed and not already resetting towards default rot due to
		button press */
		else if (bIsCameraFreeLookEnabled && !bIsResettingCameraRotation)
		{
			AddYawInput(Value);
		}
	}
}

void ARTSPlayerController::On_Mouse_Move_Y(float Value)
{
	/* This logic may best be moved to tick. HUD only draws when
	it ticks so there isn't much point updating its rectangle info
	every time the mouse moves, but I should test this and see if
	sending data in here feels any more responsive than sending it
	in tick */

	if (Value != 0.f)
	{
		if (bWantsMarquee /*bIsLMBPressed && !IsContextActionPending() <-- Old code*/)
		{
			MouseMovement += FMath::Abs(Value);
			SendMouseLocToMarqueeHUD();
			bIsMarqueeActive = true;
		}
		/* Only change rot if MMB pressed and not already resetting towards default rot due to
		button press */
		else if (bIsCameraFreeLookEnabled && !bIsResettingCameraRotation)
		{
			AddPitchInput(Value);
		}
	}
}

float ARTSPlayerController::GetCameraMovementMultiplierDueToZoom() const
{
	// Is TargetArmLength current arm length or a value the arm is heading towards but possibly 
	// not actually at yet? I want the former
	
	UCurveFloat * Curve = GI->GetCamaraZoomToMoveSpeedCurve();
	
	return (Curve == nullptr) ? 1.f : Curve->GetFloatValue(SpringArm->TargetArmLength);
}

void ARTSPlayerController::MoveCameraForward(float Value)
{
	/* Basically ignoring Z axis so as to not move upwards */
	FVector Vector = FVector(GetPawn()->GetActorForwardVector().X,
		GetPawn()->GetActorForwardVector().Y, 0.f);

	/* Without this move speed will vary based on look Z oritentation */
	Vector.Normalize();

	GetPawn()->AddMovementInput(Vector, Value);
}

void ARTSPlayerController::MoveCameraRight(float Value)
{
	/* Basically ignoring Z axis so as to not move upwards */
	FVector Vector = FVector(GetPawn()->GetActorRightVector().X,
		GetPawn()->GetActorRightVector().Y, 0.f);

	/* Without this move speed will vary based on look Z oritentation */
	Vector.Normalize();

	GetPawn()->AddMovementInput(Vector, Value);
}

void ARTSPlayerController::MoveIfAtEdgeOfScreen(float DeltaTime, const FCursorInfo *& OutCursorToShowThisFrame)
{
	// Do not try scroll if player is already moving camera with keyboard
	if (bIsForwardBackwardCameraMovementKeyPressed || bIsLeftRightCameraMovementKeyPressed)
	{
		return;
	}

	/* Do not scroll if the player has camera free look going (default key is MMB) */
	if (bIsCameraFreeLookEnabled)
	{
		return;
	}

	// Get coords of mouse
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		// Movement to apply to camera
		FVector MovementVector = FVector::ZeroVector;

		// Whether moving in a diagonal because mouse is in a corner
		bool bDiagonalMovement = false;

		/* See the comment above FI::EdgeScrollingCursor_Infos for why I choose how this gets set below */
		int32 CursorArrayIndex = -1;

		// Check if mouse is near left/right side of screen
		if (MouseX < CameraEdgeThreshold * ViewportSize_X)
		{
			MovementVector += FVector(-GetPawn()->GetActorRightVector().X,
				-GetPawn()->GetActorRightVector().Y, 0.f);

			CursorArrayIndex += 1;
		}
		else if (MouseX > (1.f - CameraEdgeThreshold) * ViewportSize_X)
		{
			MovementVector += FVector(GetPawn()->GetActorRightVector().X,
				GetPawn()->GetActorRightVector().Y, 0.f);

			CursorArrayIndex += 2;
		}
		// Check if mouse is near top/bottom of screen
		if (MouseY < CameraEdgeThreshold * ViewportSize_Y)
		{
			bDiagonalMovement = (MovementVector != FVector::ZeroVector);

			MovementVector += FVector(GetPawn()->GetActorForwardVector().X,
				GetPawn()->GetActorForwardVector().Y, 0.f);

			CursorArrayIndex += 3;
		}
		else if (MouseY > (1.f - CameraEdgeThreshold) * ViewportSize_Y)
		{
			bDiagonalMovement = (MovementVector != FVector::ZeroVector);

			MovementVector += FVector(-GetPawn()->GetActorForwardVector().X,
				-GetPawn()->GetActorForwardVector().Y, 0.f);

			CursorArrayIndex += 6;
		}

		if (MovementVector != FVector::ZeroVector)
		{
			MovementVector.Normalize();

			if (bDiagonalMovement)
			{
				DeltaTime *= URTSGameUserSettings::MOUSE_DIAGONAL_MOVEMENT_SPEED_MULTIPLIER;
			}

			GetPawn()->AddMovementInput(MovementVector, CameraEdgeMoveSpeed * DeltaTime);

			/* Set the screen location mouse cursor */
			ScreenLocationCurrentCursor = &FactionInfo->GetEdgeScrollingCursorInfo(CursorArrayIndex);
			OutCursorToShowThisFrame = ScreenLocationCurrentCursor;
		}
	}
}

bool ARTSPlayerController::ShouldResetCameraRotation() const
{
	/* The equals clause means if already at default rotation and button is pressed then user
	won't be locked from rotation while timeline completes */
	return !bIsResettingCameraRotation && !ControlRotation.Equals(DefaultCameraRotation, 1e-3f);
}

bool ARTSPlayerController::ShouldResetCameraZoom() const
{
	return !bIsResettingCameraZoom && SpringArm->TargetArmLength != DefaultCameraZoomAmount;
}

void ARTSPlayerController::OnAssignControlGroupButtonPressed(int32 ControlGroupNumber)
{
	if (IsSelectionControlledByThis())
	{
		CreateControlGroup(ControlGroupNumber);
	}
}

void ARTSPlayerController::OnSelectControlGroupButtonPressed(int32 ControlGroupNumber)
{
	// Check for double click/press whatever you want to call it
	if (WasDoublePress(ControlGroupNumber))
	{
		OnSelectControlGroupButtonDoublePress(ControlGroupNumber);
	}
	else
	{
		OnSelectControlGroupButtonSinglePress(ControlGroupNumber);
	}
}

void ARTSPlayerController::OnSelectControlGroupButtonReleased(int32 ControlGroupNumber)
{
	// Record these for double press purposes
	LastSelectControlGroupButtonReleased = ControlGroupNumber;
	LastSelectControlGroupButtonReleaseTime = GetWorld()->GetRealTimeSeconds();
}

/**
 *	These two functions have been written with the assumption that: 
 *	1. When doing a double press the single press func will be called followed by the double 
 *	press func 
 */
void ARTSPlayerController::OnSelectControlGroupButtonSinglePress(int32 ControlGroupNumber)
{	
	if (GetNumCtrlGroupMembers(ControlGroupNumber) > 0)
	{
		SelectControlGroup(ControlGroupNumber);
	}
}

void ARTSPlayerController::OnSelectControlGroupButtonDoublePress(int32 ControlGroupNumber)
{
	if (GetNumCtrlGroupMembers(ControlGroupNumber) > 0)
	{
		SnapViewToControlGroup(ControlGroupNumber);
	}
}

void ARTSPlayerController::CreateControlGroup(int32 ControlGroupNumber)
{
	if (!HasSelection())
	{
		return;
	}

	TArray <AActor *> & CtrlGroupArray = CtrlGroups[ControlGroupNumber].GetArray();

	// Copy Selected into ctrl group array
	CtrlGroupArray = Selected;
}

bool ARTSPlayerController::SelectControlGroup(int32 ControlGroupNumber)
{
	// Need to deselect things that are selected and not in ctrl group
	// May want to eventually review this func and see if it can be improved
	// O(n)

	// True if player will have something selected by the end of this func
	bool bSelectedSomething = false;
	// True if we know the player's selection will change as a result of this func
	bool bDidSelectionChange = false;

	TArray <AActor *> & CtrlGroupArray = CtrlGroups[ControlGroupNumber].GetArray();
	
	AActor * FirstInArray = CtrlGroupArray[0];

	/* Check for the easy case where ctrl group is a single building */
	if (Statics::IsValid(FirstInArray) && Statics::IsABuilding(FirstInArray) 
		&& !Statics::HasZeroHealth(FirstInArray))
	{
		if (HasSelection())
		{
			/* Check for easy case where this building was already selected */
			if (FirstInArray == Selected[0])
			{
				// Easy case. Do nothing
			}
			else
			{
				/* Deselect everything that we already have selected */
				for (int32 i = Selected.Num() - 1; i >= 0; --i)
				{
					ISelectable * AsSelectable = CastChecked<ISelectable>(Selected[i]);
					AsSelectable->OnDeselect();

					Selected.RemoveAt(i, 1, false);
				}

				SelectedIDs.Reset();

				bDidSelectionChange = true;
			}
		}
		else
		{
			bDidSelectionChange = true;
		}

		bSelectedSomething = true;

		ISelectable * AsSelectable = CastChecked<ISelectable>(FirstInArray);
		uint8 SelectableID;
		AsSelectable->OnCtrlGroupSelect(SelectableID);
		SelectedIDs.Emplace(SelectableID);

		assert(Selected.Num() == 0);

		// This is expected to be the first elem added to array
		Selected.Emplace(FirstInArray);
	}
	else
	{
		// Hard case: ctrl group is not just a single building
		
		// Will reset this now and use it to know who to call OnDeselect on
		SelectedIDs.Reset();

		EUnitType BestType = EUnitType::None;
		int32 BestIndex = -1;

		for (int32 i = CtrlGroupArray.Num() - 1; i >= 0; --i)
		{
			AActor * Selectable = CtrlGroupArray[i];
			
			// Bookkeeping: Remove invalids and zero healthers
			if (!Statics::IsValid(Selectable) || Statics::HasZeroHealth(Selectable))
			{
				// Hoping this doesn't crash if array num is 1
				CtrlGroupArray.RemoveAtSwap(i, 1, false);
				continue;
			}
			
			bSelectedSomething = true;

			ISelectable * AsSelectable = CastChecked<ISelectable>(Selectable);
			uint8 SelectableID;
			const EUnitType UnitType = AsSelectable->OnCtrlGroupSelect(SelectableID);

			// For choosing who will be CurrentSelected
			if (UnitType > BestType)
			{
				BestType = UnitType;
				BestIndex = i;
			}

			SelectedIDs.Emplace(SelectableID);
		}

		/* Swap what should be CurrentSelected into index 0 */
		if (BestIndex != -1)
		{
			CtrlGroupArray.SwapMemory(0, BestIndex);
		}

		const int32 SelectedNum = Selected.Num();
		const int32 CtrlGroupNum = CtrlGroupArray.Num();

		/* Update the Selected array and call OnDeselect on anything that lost selection */
		if (CtrlGroupNum == SelectedNum)
		{
			for (int32 i = 0; i < SelectedNum; ++i)
			{
				AActor * Elem = Selected[i];

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				if (!SelectedIDs.Contains(AsSelectable->GetSelectableID()))
				{
					AsSelectable->OnDeselect();

					bDidSelectionChange = true;
				}
				
				Selected[i] = CtrlGroupArray[i];
			}
		}
		else if (CtrlGroupNum < SelectedNum)
		{
			bDidSelectionChange = true;
			
			for (int32 i = 0; i < CtrlGroupNum; ++i)
			{
				AActor * Elem = Selected[i];

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				if (!SelectedIDs.Contains(AsSelectable->GetSelectableID()))
				{
					AsSelectable->OnDeselect();
				}

				Selected[i] = CtrlGroupArray[i];
			}

			// For rest of Selected array call OnDeselect on anything applicable
			for (int32 i = CtrlGroupNum; i < SelectedNum; ++i)
			{
				AActor * Elem = Selected[i];

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				if (!SelectedIDs.Contains(AsSelectable->GetSelectableID()))
				{
					AsSelectable->OnDeselect();
				}
			}
		}
		else // CtrlGroupNum > SelectedNum
		{
			bDidSelectionChange = true;
			
			for (int32 i = 0; i < SelectedNum; ++i)
			{
				AActor * Elem = Selected[i];

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				if (!SelectedIDs.Contains(AsSelectable->GetSelectableID()))
				{
					AsSelectable->OnDeselect();
				}

				Selected[i] = CtrlGroupArray[i];
			}

			for (int32 i = 0; i < CtrlGroupNum; ++i)
			{
				Selected.Emplace(CtrlGroupArray[i]);
			}
		}
	}
	
	//--------------------------------
	// Final stages
	//--------------------------------

	if (bSelectedSomething)
	{
		CurrentSelected = TScriptInterface<ISelectable>(Selected[0]);
		
		/* Got to do this: ISelectable::OnCtrlGroupSelect and OnDeselect each flag the selectable 
		as not the current selected */
		CurrentSelected->GetAttributesBase().SetIsPrimarySelected(true);

		// Possibly play a sound
		if (ShouldPlaySelectionSound(CurrentSelected->GetAttributesBase()))
		{
			PlaySelectablesSelectionSound();
		}

		TimeOfLastSelection_Owned = GetWorld()->GetRealTimeSeconds();
	}
	else
	{
		CurrentSelected = nullptr;
	}
	
	if (bDidSelectionChange)
	{
		HUD->OnPlayerCurrentSelectedChanged(CurrentSelected);
	}

	return bDidSelectionChange;
}

bool ARTSPlayerController::SnapViewToControlGroup(int32 NumberKeyPressed)
{	
	TArray <AActor *> & CtrlGroupArray = CtrlGroups[NumberKeyPressed].GetArray();
	const int32 CtrlGroupNum = CtrlGroupArray.Num();

	/* Could remove unvalid/zeroHealthers now, but can also just leave it up to the ctrl group 
	selection logic which is what I will do. 
	Logic here is to just use location of first valid selectable in ctrl group */
	for (int32 i = 0; i < CtrlGroupNum; ++i)
	{
		AActor * Selectable = CtrlGroupArray[i];

		if (Statics::IsValid(Selectable) && !Statics::HasZeroHealth(Selectable))
		{
			MatchCamera->SnapToLocation(Selectable->GetActorLocation(), CameraOptions::bGlueToGround);

			return true;
		}
	}
	
	return false;
}

int32 ARTSPlayerController::GetNumCtrlGroupMembers(int32 CtrlGroupNumber) const
{
	// This doesn't take into account whether they are valid and/or above zero health
	return CtrlGroups[CtrlGroupNumber].GetNum();
}

void ARTSPlayerController::InitCtrlGroups()
{
	CtrlGroups.Init(FCtrlGroupList(), 10);
}

bool ARTSPlayerController::WasDoublePress(int32 ControlGroupButtonPressed)
{
	if (ControlGroupButtonPressed == LastSelectControlGroupButtonReleased)
	{
		/* Want to avoid triple/quad... etc presses from counting as a double press so check if 
		last press was counted as a double press */
		if (bWasLastSelectControlGroupButtonPressADoublePress)
		{
			// Not going to allow a double click here

			bWasLastSelectControlGroupButtonPressADoublePress = false;

			return bWasLastSelectControlGroupButtonPressADoublePress;
		}
		else
		{
			// Use what is defined in project settings
			const float DoubleClickTime = GetDefault<UInputSettings>()->DoubleClickTime;
			
			const float TimeSinceLastRelease = GetWorld()->GetRealTimeSeconds() - LastSelectControlGroupButtonReleaseTime;

			bWasLastSelectControlGroupButtonPressADoublePress = (TimeSinceLastRelease < DoubleClickTime);

			return bWasLastSelectControlGroupButtonPressADoublePress;
		}
	}
	else
	{
		bWasLastSelectControlGroupButtonPressADoublePress = false;
		return bWasLastSelectControlGroupButtonPressADoublePress;
	}
}

void ARTSPlayerController::RotateGhost(float DeltaTime)
{
	assert(GhostBuilding != nullptr);

	if (!bIsGhostRotationActive)
	{
		/* Check if mouse has moved enough to activate ghost rotation now. Alternative to
		doing it this way is to do it like marquee select and measure amount mouse has
		moved not the distance */
		bIsGhostRotationActive = FVector2D::DistSquared(MouseLocThisFrame, GhostScreenSpaceLoc) > FMath::Square(GhostRotationRadius);

#if GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD
		if (bIsGhostRotationActive)
		{
			/* Set what the initial direction is. Just basing it off X axis movement not Y.
			Left means clockwise, right means means counterclockwise */
			if (MouseLocThisFrame.X >= GhostScreenSpaceLoc.X)
			{
				GhostRotationDirection = ERotationDirection::CounterClockwise;
			}
			else
			{
				GhostRotationDirection = ERotationDirection::Clockwise;
			}

			/* Set this as zero but could use a positive number that's lower than 
			MaxAccumulatedMovementTowardsDirection or the distance */
			GhostAccumulatedMovementTowardsDirection = 0.f;
		}
#endif
	}

	if (bIsGhostRotationActive)
	{
// switch (GHOST_ROTATION_METHOD)
#if GHOST_ROTATION_METHOD == BASIC_GHOST_ROT_METHOD
		{
			/* Check how much mouse has moved since last frame */
			const FVector2D Difference = MouseLocThisFrame - MouseLocLastFrame;

			const float YawRot = -Difference.X * GhostRotationSpeed * DeltaTime;

			// Apply rotation to ghost 
			GhostBuilding->AddActorLocalRotation(FRotator(0.f, YawRot, 0.f));

		}// End EGhostRotationMethod::Basic
		
#elif GHOST_ROTATION_METHOD == SNAP_GHOST_ROT_METHOD
		{
			/* Calculate screen space angle between ghost loc and mouse loc.
			0 degrees = 3 o'clock, 90 degrees = 12 o'clock, etc */
			const float Delta_X = MouseLocThisFrame.X - GhostScreenSpaceLoc.X;
			const float Delta_Y = GhostScreenSpaceLoc.Y - MouseLocThisFrame.Y;
			const float ScreenSpaceAngle = FMath::RadiansToDegrees<float>(FMath::Atan2(Delta_Y, Delta_X));

			/* ScreenSpaceAngle will be in range (-180, 180). The +90 in the next funcs param is
			so angle maps to something like this:
			6 o'clock (= -90) --> keep default ghost rot
			3 o'clock (= 0) --> rotate ghost 90 degrees counter-clockwise
			9 o'clock (= 180) --> rotate ghost 90 degrees clockwise */

			/* Set rotation of ghost */
			GhostBuilding->SetRotationUsingOffset(ScreenSpaceAngle + 90.f);

		} // End EGhostRotationMethod::Snap

#elif GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD
		{	
			// Check everything is as expected
			assert(GhostOptions::StdRot::AngleLenienceInner >= 0.f && GhostOptions::StdRot::AngleLenienceInner <= 45.f);
			assert(GhostOptions::StdRot::ChangeDirectionThreshold <= GhostOptions::StdRot::NoRotationThreshold);
			assert(GhostOptions::StdRot::NoRotationThreshold <= GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection);
			assert(GhostOptions::StdRot::AngleLenienceInner < GhostOptions::StdRot::AngleLenienceOuter);
			assert(GhostOptions::StdRot::FalloffDistanceInner < GhostOptions::StdRot::FalloffDistanceMiddle);
			assert(GhostOptions::StdRot::FalloffDistanceMiddle < GhostOptions::StdRot::FalloffDistanceOuter);
			assert(GhostOptions::StdRot::AngleMinFalloffMultiplier >= 0.f && GhostOptions::StdRot::AngleMinFalloffMultiplier <= 1.f)

			//==============================================================================
			//	Assign commonly used constants
			//==============================================================================

			/* Check how much mouse has moved since last frame */
			const FVector2D Difference = MouseLocThisFrame - MouseLocLastFrame;
			
			// How far mouse has moved since last frame
			const float Distance2D = FVector2D::Distance(MouseLocThisFrame, MouseLocLastFrame);

			// Distance mouse is from ghost
			const float Distance2DFromGhost = FVector2D::Distance(MouseLocThisFrame, GhostScreenSpaceLoc);

			/* Calculate screen space angle between mouse pos this frame and ghost loc. 
			0 = 3 o'clock, 90 = 12 o'clock, etc. Range I think is (-180, 180) */
			const float Delta_X = MouseLocThisFrame.X - GhostScreenSpaceLoc.X;
			const float Delta_Y = GhostScreenSpaceLoc.Y - MouseLocThisFrame.Y;
			const float ScreenSpaceAngleWithGhost = FMath::RadiansToDegrees<float>(FMath::Atan2(Delta_Y, Delta_X));

			/* 
			Case 01: Mouse is moving left + up,		mouse is above and left of ghost loc
			Case 02: Mouse is moving left + up,		mouse is above and right of ghost loc
			Case 03: Mouse is moving left + up,		mouse is below and left of ghost loc
			Case 04: Mouse is moving left + up,		mouse is below and right of ghost loc
			Case 05: Mouse is moving left + down,	mouse is above and left of ghost loc
			Case 06: Mouse is moving left + down,	mouse is above and right of ghost loc
			Case 07: Mouse is moving left + down,	mouse is below and left of ghost loc
			Case 08: Mouse is moving left + down,	mouse is below and right of ghost loc
			Case 09: Mouse is moving right + up,	mouse is above and left of ghost loc
			Case 10: Mouse is moving right + up,	mouse is above and right of ghost loc
			Case 11: Mouse is moving right + up,	mouse is below and left of ghost loc
			Case 12: Mouse is moving right + up,	mouse is below and right of ghost loc
			Case 13: Mouse is moving right + down,	mouse is above and left of ghost loc
			Case 14: Mouse is moving right + down,	mouse is above and right of ghost loc
			Case 15: Mouse is moving right + down,	mouse is below and left of ghost loc
			Case 16: Mouse is moving right + down,	mouse is below and right of ghost loc
			*/

			const bool bRotatingClockwise = (GhostRotationDirection == ERotationDirection::Clockwise);
			const bool bRotatingCounterClockwise = (GhostRotationDirection == ERotationDirection::CounterClockwise);
			const bool bNoDirectionEstablished = (GhostRotationDirection == ERotationDirection::NoDirectionEstablished);


			//==================================================================================
			//	Implementation
			//==================================================================================

			float YawRot;

			// Note: negative YawRot means counter-clockwise rot and positive means clockwise

			if (Difference.X < 0.f)
			{
				if (Difference.Y < 0.f)
				{
					if (MouseLocThisFrame.Y < GhostScreenSpaceLoc.Y) 
					{
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 1
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D, 
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
						else // Case 2
						{
							YawRot = GhostRotStandard_AssignYawRotForCounterClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
					}
					else 
					{
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 3
						{
							YawRot = GhostRotStandard_AssignYawRotForClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
						else // Case 4
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
					}
				}
				else
				{
					if (MouseLocThisFrame.Y < GhostScreenSpaceLoc.Y) 
					{
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 5
						{
							YawRot = GhostRotStandard_AssignYawRotForCounterClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
						else // Case 6
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
					}
					else 
					{
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 7
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
						else // Case 8
						{
							YawRot = GhostRotStandard_AssignYawRotForClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
					}
				}
			}
			else
			{
				if (Difference.Y <= 0.f)
				{
					if (MouseLocThisFrame.Y < GhostScreenSpaceLoc.Y) 
					{	
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 9
						{
							YawRot = GhostRotStandard_AssignYawRotForClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
						else // Case 10
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
					}
					else 
					{
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 11
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
						else // Case 12
						{
							YawRot = GhostRotStandard_AssignYawRotForCounterClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
					}
				}
				else
				{
					if (MouseLocThisFrame.Y < GhostScreenSpaceLoc.Y)
					{
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 13
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
						else // Case 14
						{
							YawRot = GhostRotStandard_AssignYawRotForClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
					}
					else
					{
						if (MouseLocThisFrame.X < GhostScreenSpaceLoc.X) // Case 15
						{
							YawRot = GhostRotStandard_AssignYawRotForCounterClockwiseTypeMouseMovement(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, bNoDirectionEstablished, 
								Distance2DFromGhost);
						}
						else // Case 16
						{
							YawRot = GhostRotStandard_AssignNeutralDirectionYawRot(Distance2D,
								bRotatingClockwise, bRotatingCounterClockwise, ScreenSpaceAngleWithGhost, 
								Distance2DFromGhost);
						}
					}
				}
			}

			/* Honestly not multiplying by delta time here may still be ok. */
			YawRot *= GhostRotationSpeed * DeltaTime;

			// Apply rotation to ghost 
			GhostBuilding->AddActorLocalRotation(FRotator(0.f, YawRot, 0.f));
		} // End EGhostRotationMethod::Standard

#else // GHOST_ROTATION_METHOD == EGhostRotationMethod::Standard
		UE_LOG(RTSLOG, Fatal, TEXT("Unimplemented ghost rotation method \"%d\" "),
			GHOST_ROTATION_METHOD);
#endif
	}
}

#if GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD
float ARTSPlayerController::GhostRotStandard_GetAngleFalloffMultiplier(float ScreenSpaceAngleWithGhost) const
{	
	// Angle = ScreenSpaceAngleWithGhost in range [0, 45]
	// Angle of 50 deg becomes 5 
	float Angle = FMath::Abs(ScreenSpaceAngleWithGhost);
	Angle = FMath::Fmod(Angle, 90.f);
	Angle -= 90.f;
	Angle = FMath::Abs(Angle);
	Angle = (Angle > 45.f) ? FMath::Abs(Angle - 90.f) : Angle;

	float FalloffMulti;

	if (Angle <= GhostOptions::StdRot::AngleLenienceInner)
	{
		FalloffMulti = 1.f;
	}
	else if (Angle <= GhostOptions::StdRot::AngleLenienceOuter)
	{
		// Some falloff will be applied 
		const float Percent = (Angle - GhostOptions::StdRot::AngleLenienceInner) / (GhostOptions::StdRot::AngleLenienceOuter - GhostOptions::StdRot::AngleLenienceInner);
		
		FalloffMulti = (1.f - Percent) * GhostOptions::StdRot::AngleMinFalloffMultiplier + GhostOptions::StdRot::AngleMinFalloffMultiplier;
	}
	else
	{
		FalloffMulti = GhostOptions::StdRot::AngleMinFalloffMultiplier;
	}

	return FalloffMulti;
}

float ARTSPlayerController::GhostRotStandard_GetDistanceFalloffMultiplier(float Distance2DFromGhost) const
{
	float Multiplier;

	if (Distance2DFromGhost < GhostOptions::StdRot::FalloffDistanceInner)
	{
		// Range (0, 1]
		const float Percent = (GhostOptions::StdRot::FalloffDistanceInner - Distance2DFromGhost) / GhostOptions::StdRot::FalloffDistanceInner;
			
		Multiplier = (1.f - Percent) * (1.f - GhostOptions::StdRot::DistanceMinFalloffMultiplierInner) + GhostOptions::StdRot::DistanceMinFalloffMultiplierInner;
	}
	else if (Distance2DFromGhost < GhostOptions::StdRot::FalloffDistanceMiddle)
	{
		Multiplier = 1.f;
	}
	else if (Distance2DFromGhost < GhostOptions::StdRot::FalloffDistanceOuter)
	{
		const float Percent = (Distance2DFromGhost - GhostOptions::StdRot::FalloffDistanceMiddle) / (GhostOptions::StdRot::FalloffDistanceOuter - GhostOptions::StdRot::FalloffDistanceMiddle);

		Multiplier = (1.f - Percent) * (1.f - GhostOptions::StdRot::DistanceMinFalloffMultiplierOuter) + GhostOptions::StdRot::DistanceMinFalloffMultiplierOuter;
	}
	else
	{
		Multiplier = GhostOptions::StdRot::DistanceMinFalloffMultiplierOuter;
	}

	return Multiplier;
}

float ARTSPlayerController::GhostRotStandard_AssignNeutralDirectionYawRot(float Distance2D, 
	bool bRotatingClockwise, bool bRotatingCounterClockwise, float ScreenSpaceAngleWithGhost, 
	float Distance2DFromGhost)
{
	if (bRotatingClockwise || bRotatingCounterClockwise)
	{	
		const float Sign = bRotatingClockwise ? 1.f : -1.f;

		// Rotation falloff due to angle mouse loc makes with ghost loc
		const float AngleFalloffMultiplier = GhostRotStandard_GetAngleFalloffMultiplier(ScreenSpaceAngleWithGhost);

		// Rotation falloff due to distance mouse is from ghost
		const float DistanceFalloffMultiplier = GhostRotStandard_GetDistanceFalloffMultiplier(Distance2DFromGhost);

		return Sign * AngleFalloffMultiplier * DistanceFalloffMultiplier * Distance2D;
	}
	else
	{
		/* Not already rotating in any direction. Hard to know what direction the
		player wants so return no rotation */
		return 0.f;
	}
}

float ARTSPlayerController::GhostRotStandard_AssignYawRotForClockwiseTypeMouseMovement(float Distance2D, 
	bool bRotatingClockwise, bool bRotatingCounterClockwise, bool bNotRotating, float Distance2DFromGhost)
{
	if (bRotatingClockwise)
	{
		// Add to "momentum" and clamp at max value
		GhostAccumulatedMovementTowardsDirection += Distance2D;
		if (GhostAccumulatedMovementTowardsDirection > GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection)
		{
			GhostAccumulatedMovementTowardsDirection = GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection;
		}

		// Also apply distance falloff
		return GhostRotStandard_GetDistanceFalloffMultiplier(Distance2DFromGhost) * Distance2D;
	}
	else if (bRotatingCounterClockwise)
	{
		/* Do not apply rotation instead chip away at accumulated direction */
		GhostAccumulatedMovementTowardsDirection -= Distance2D;

		/* Check if mouse has moved enough for it to be considered player wants to
		change direction of rotation */
		if (GhostAccumulatedMovementTowardsDirection < GhostOptions::StdRot::ChangeDirectionThreshold)
		{
			// Change rotation direction
			GhostAccumulatedMovementTowardsDirection = 0.f;
			GhostRotationDirection = ERotationDirection::Clockwise;

			// Return rotation in clockwise direction
			return GhostRotStandard_GetDistanceFalloffMultiplier(Distance2DFromGhost) * Distance2D;
		}
		else if (GhostAccumulatedMovementTowardsDirection < GhostOptions::StdRot::NoRotationThreshold)
		{
			// Set direction as "no direction"
			GhostAccumulatedMovementTowardsDirection = 0.f;
			GhostRotationDirection = ERotationDirection::NoDirectionEstablished;
		}

		return 0.f;
	}
	else // Assumed bNotRotating
	{
		assert(bNotRotating);

		// Add to "momentum" and clamp at max value
		GhostAccumulatedMovementTowardsDirection += Distance2D;
		if (GhostAccumulatedMovementTowardsDirection > GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection)
		{
			GhostAccumulatedMovementTowardsDirection = GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection;

			// Have reached threshold. Can start applying movement now
			GhostRotationDirection = ERotationDirection::Clockwise;

			// Return rotation in clockwise direction
			return GhostRotStandard_GetDistanceFalloffMultiplier(Distance2DFromGhost) * Distance2D;
		}

		return 0.f;
	}
}

float ARTSPlayerController::GhostRotStandard_AssignYawRotForCounterClockwiseTypeMouseMovement(float Distance2D,
	bool bRotatingClockwise, bool bRotatingCounterClockwise, bool bNotRotating, float Distance2DFromGhost)
{
	if (bRotatingClockwise)
	{
		/* Do not apply rotation instead chip away at accumulated direction */
		GhostAccumulatedMovementTowardsDirection -= Distance2D;

		/* Check if mouse has moved enough for it to be considered player wants to
		change direction of rotation */
		if (GhostAccumulatedMovementTowardsDirection < GhostOptions::StdRot::ChangeDirectionThreshold)
		{
			// Change rotation direction
			GhostAccumulatedMovementTowardsDirection = 0.f;
			GhostRotationDirection = ERotationDirection::CounterClockwise;

			// Return rotation in counter clockwise direction
			return GhostRotStandard_GetDistanceFalloffMultiplier(Distance2DFromGhost) * -Distance2D;

		}
		else if (GhostAccumulatedMovementTowardsDirection < GhostOptions::StdRot::NoRotationThreshold)
		{
			// Set direction as "no direction"
			GhostAccumulatedMovementTowardsDirection = 0.f;
			GhostRotationDirection = ERotationDirection::NoDirectionEstablished;
		}

		return 0.f;
	}
	else if (bRotatingCounterClockwise)
	{
		// Add to "momentum" and clamp at max value
		GhostAccumulatedMovementTowardsDirection += Distance2D;
		if (GhostAccumulatedMovementTowardsDirection > GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection)
		{
			GhostAccumulatedMovementTowardsDirection = GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection;
		}

		// Also apply distance falloff
		return GhostRotStandard_GetDistanceFalloffMultiplier(Distance2DFromGhost) * -Distance2D;
	}
	else // Assumed bNotRotating
	{
		assert(bNotRotating);

		// Add to "momentum" and clamp at max value
		GhostAccumulatedMovementTowardsDirection += Distance2D;
		if (GhostAccumulatedMovementTowardsDirection > GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection)
		{
			GhostAccumulatedMovementTowardsDirection = GhostOptions::StdRot::MaxAccumulatedMovementTowardsDirection;

			// Have reached threshold. Can start applying movement now
			GhostRotationDirection = ERotationDirection::CounterClockwise;

			// Return rotation in counter clockwise direction
			return GhostRotStandard_GetDistanceFalloffMultiplier(Distance2DFromGhost) * -Distance2D;
		}

		return 0.f;
	}
}
#endif // GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD

FVector2D ARTSPlayerController::GetMouseCoords()
{
	float MouseX, MouseY;
	bool bResult = GetMousePosition(MouseX, MouseY);
	assert(bResult); // TODO crash in standalone when mouse goes over edge
	return FVector2D(MouseX, MouseY);
}

void ARTSPlayerController::SpawnGhostBuilding(EBuildingType BuildingType, uint8 InstigatorsID)
{
	/* Assumed any checks such as prereqs, resources etc have been made already. Checks will also
	happen when ghost is placed both on client and server */

	/* If a ghost already exists this will create a new one */

	const FBuildingInfo * const BuildingInfo = FactionInfo->GetBuildingInfo(BuildingType);
	assert(BuildingInfo != nullptr);

	FHitResult Hit;
	if (LineTraceUnderMouse(ECC_Visibility, Hit))
	{
		/* Shift building up by half its height */
		FVector SpawnLoc = Hit.Location;
		SpawnLoc.Z += BuildingInfo->GetBoundsHeight() / 2;

		/* Take ghost from object pool */
		GhostBuilding = GhostPool[BuildingType];
		GhostBuilding->OnExitPool(BuildingType);

		PendingContextAction = EContextButton::PlacingGhost;
		GhostInstigatorID = InstigatorsID;
	}
	else
	{
		// Line trace did not hit something within range
	}
}

bool ARTSPlayerController::IsHoveringUIElement() const
{
	return HoveredUIElement != nullptr;
}

bool ARTSPlayerController::WasMouseClick()
{
	return MouseMovement < MouseMovementThreshold;
}

bool ARTSPlayerController::LineTraceUnderMouse(ECollisionChannel Channel)
{
	/* HitResult is not reset to default values before this, so make sure
	to check if this returns true before considering using HitResult */

	GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(Channel), false, HitResult);
	if (HitResult.GetActor() != nullptr && HitResult.Distance <= MaxLineTraceDistance)
	{
		// Trace hit something and hit isn't considered too far away
		return true;
	}

	return false;
}

bool ARTSPlayerController::LineTraceUnderMouse(ECollisionChannel Channel, FHitResult & Hit)
{
	GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(Channel), false, Hit);
	if (Hit.GetActor() != nullptr && Hit.Distance <= MaxLineTraceDistance)
	{
		// Trace hit something and hit isn't considered too far away
		return true;
	}

	return false;
}

void ARTSPlayerController::MoveGhostBuilding()
{
	/* Move ghost only if it is still a ghost */
	if (GhostBuilding != nullptr)
	{
		const FBuildingInfo * BuildingInfo = FactionInfo->GetBuildingInfo(GhostBuilding->GetType());

		/* Shift up by half mesh height */
		FVector NewLoc = HitResult.Location;
		NewLoc.Z += BuildingInfo->GetBoundsHeight() / 2;

		GhostBuilding->SetActorLocation(NewLoc);
	}
}

bool ARTSPlayerController::CanPlaceBuilding(EBuildingType BuildingType, uint8 InstigatorsID,
	const FVector & Location, const FRotator & Rotation, bool bShowHUDMessages, ABuilding *& OutProducer,
	EProductionQueueType & OutQueueType) const
{
	if (PS->IsAtSelectableCap(true, bShowHUDMessages) == true)
	{
		return false;
	}
	if (PS->IsAtUniqueBuildingCap(BuildingType, true, bShowHUDMessages) == true)
	{
		return false;
	}
	if (PS->ArePrerequisitesMet(BuildingType, bShowHUDMessages) == false)
	{
		return false;
	}
	if (PS->HasEnoughResources(BuildingType, bShowHUDMessages) == false)
	{
		return false;
	}
	
	const FBuildingInfo * BuildInfo = FactionInfo->GetBuildingInfo(BuildingType);
	const FContextButton Button = FContextButton(BuildingType);

	/* 0 is code for 'built from HUD persistent panel'. If non-zero then we need to check that
	the selectable is still valid and able to produce the building */
	if (InstigatorsID == 0)
	{
		/* Choose the construction yard type building to build from, or the player state queue.
		Return false if cannot find a valid actor to build from */
		PS->GetActorToBuildFrom(Button, bShowHUDMessages, OutProducer, OutQueueType);
		if (Statics::IsValid(OutProducer) == false)
		{
			return false;
		}
	}
	else
	{
		AActor * const Builder = PS->GetSelectableFromID(InstigatorsID);
		if (!Statics::IsValid(Builder) || Statics::HasZeroHealth(Builder))
		{
			PS->OnGameWarningHappened(EGameWarning::BuilderDestroyed);
			return false;
		}

		ISelectable * AsSelectable = CastChecked<ISelectable>(Builder);

		/* Check the selectable supports building this building. Do this by checking if
		cooldown remaining equals -2. More suited in the _Validate version of function */
		if (AsSelectable->GetCooldownRemaining(Button) == -2.f)
		{
			return false;
		}
	}

	if (!Statics::IsBuildableLocation(GetWorld(), GS, PS, FactionInfo, BuildingType, Location, 
		Rotation, bShowHUDMessages))
	{
		return false;
	}

	return true;
}

bool ARTSPlayerController::CanPlaceBuilding(EBuildingType BuildingType, uint8 InstigatorsID,
	const FVector & Location, const FRotator & Rotation, bool bShowHUDMessages) const
{
	ABuilding * B = nullptr;
	EProductionQueueType Q;

	return CanPlaceBuilding(BuildingType, InstigatorsID, Location, Rotation, bShowHUDMessages,
		B, Q);
}

void ARTSPlayerController::Server_PlaceBuilding_Implementation(EBuildingType BuildingType,
	const FVector_NetQuantize10 & Location, const FRotator & Rotation, uint8 ConstructionInstigatorID)
{
	/* If everything goes ok this is the building we spawn */
	ABuilding * BuildingWeSpawned = nullptr;
	
	UWorld * World = GetWorld();
	const FBuildingInfo * const BuildingInfo = FactionInfo->GetBuildingInfo(BuildingType);
	const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

	/* For BuildsInTab build method only. Actor that produced building */
	ABuilding * BuildsInTabProducer_Building = nullptr;

	/* Rare, but will need to check if the building has finished production server-side. If it
	hasn't then timer handles are off by a bit and player is damn fast at clicking the button
	and placing their ghost somewhere. Could also mean modified client. Can't be 100% sure though
	so we assume timers are off for now. This needs to happen for the build method BuildsInTab
	only */
	if (BuildMethod == EBuildingBuildMethod::BuildsInTab)
	{
		bool bFoundViableQueue = false;

		/* Check queues that say they have completed BuildsInTab production, searching for one
		with the building we want */
		TArray <ABuilding *> & PotentialConstructors = PS->GetBuildsInTabCompleteBuildings();
		int32 ArrayNum = PotentialConstructors.Num(); // TODO make sure to add these when production completes and remove any entries when building is destroyed
		FProductionQueue * Queue = nullptr;
		int32 GoodIndex = -1;
		for (int32 i = ArrayNum - 1; i >= 0; --i)
		{
			ABuilding * Building = PotentialConstructors[i];

			/* Buildings should be removed from here when they are destroyed */
			assert(Statics::IsValid(Building));

			Queue = Building->GetPersistentProductionQueue();

			if (Queue->Num() > 0 && Queue->Peek().GetBuildingType() == BuildingType)
			{
				/* This queue has a completed building at the front of it that is the type we want. */
				bFoundViableQueue = true;

				/* Save this so we can tell client to remove it */
				BuildsInTabProducer_Building = Building;

				// Save index. Do not remove from array yet because we have to do a few more checks to see if building can be placed
				GoodIndex = i;
				break;
			}
		}
		
		/* This is the situation where timers are off between server and client */
		if (!bFoundViableQueue)
		{
			PS->OnGameWarningHappened(EGameWarning::BuildingNotReadyYet);
			return;
		}
		else
		{
			/* Some checks. Prereqs and resources can probably be skipped for server player. 
			In fact I think everything can. Selectable cap and uniqueness were incremented 
			the moment we added the building to queue */
			
			if (this != World->GetFirstPlayerController())
			{
				/* Check selectable cap, uniqueness, resources, prereqs. Actually we won't */
			}
			
			/* Looking through code it looks like this needs to be called here regardless of 
			whether it is server player or client player this is on */
			if (Statics::IsBuildableLocation(World, GS, PS, FactionInfo, BuildingType, Location,
				Rotation, true) == false)
			{
				return;
			}
			
			/* Remove from array to say 'we built it' */
			PotentialConstructors.RemoveAt(GoodIndex, 1, false);

			/* Remove item from queue */
			Queue->OnBuildsInTabBuildingPlaced();
		}
	}
	else
	{
		/* For pass by reference */
		ABuilding * ProductionBuilding = nullptr;
		EProductionQueueType QueueType;

		if (!CanPlaceBuilding(BuildingType, ConstructionInstigatorID, Location, Rotation, true,
			ProductionBuilding, QueueType))
		{
			/* Did not succeed in placing building. Notify selectable trying to place building
			so they can adjust their behavior */
			if (ConstructionInstigatorID != 0)
			{
				AActor * Builder = PS->GetSelectableFromID(ConstructionInstigatorID);
				ISelectable * AsSelectable = CastChecked<ISelectable>(Builder);
				AsSelectable->OnContextMenuPlaceBuildingResult(nullptr, *BuildingInfo, BuildMethod);
			}

			return;
		}

		/* If here building will be spawned */

		/* Depending on the buildings build method do things differently */
		if (BuildMethod == EBuildingBuildMethod::BuildsItself)
		{
			/* If removing this line consider removing the if (BuildMethod != EBuildingBuildMethod::BuildsItself) 
			that wraps the SpawnBuilding call below */
			BuildingWeSpawned = Statics::SpawnBuilding(BuildingInfo, Location, Rotation, PS, GS, 
				World, ESelectableCreationMethod::Production);
			
			const FContextButton Button = FContextButton(BuildingType);

			// It is assumed this func will deduct resource cost
			ProductionBuilding->Server_OnProductionRequest(Button, QueueType, false, BuildingWeSpawned);
		}
		else
		{
			/* Build method where do not need to add anything to a production queue */

			/* Deduct building cost */
			PS->SpendResources(BuildingInfo->GetCosts());
		}
	}

	// Check because we already spawned it for this method
	if (BuildMethod != EBuildingBuildMethod::BuildsItself)
	{
		BuildingWeSpawned = Statics::SpawnBuilding(BuildingInfo, Location, Rotation, PS, GS, World, 
			ESelectableCreationMethod::Production);
	}

	/* Check whether this controller belongs to server player or not */
	if (this == World->GetFirstPlayerController())
	{
		/* Was getting crashes here when worker gets to location for LayFoundationWhenAtLocation
		and Protoss build methods so wrapping this in an if */
		if (BuildMethod == EBuildingBuildMethod::BuildsInTab
			|| BuildMethod == EBuildingBuildMethod::BuildsItself
			|| BuildMethod == EBuildingBuildMethod::LayFoundationsInstantly)
		{
			assert(PendingContextAction == EContextButton::PlacingGhost);
			CancelGhost();
		}

		if (BuildsInTabProducer_Building != nullptr)
		{
			/* Update HUD */
			HUD->OnBuildsInTabBuildingPlaced(FTrainingInfo(BuildingType));
		}
	}
	else
	{
		/* Signal placing building was successful. May be able to merge this RPC into the one that
		goes for the production queue */
		if (BuildsInTabProducer_Building != nullptr)
		{
			Client_OnBuildsInTabPlaceBuildingSuccess(BuildsInTabProducer_Building, BuildingType);
		}
		else
		{
			Client_OnPlaceBuildingSuccess();
		}
	}

	/* Check if this was a command from a context menu and if so order let the instigator
	know the building was placed successfully so they can head over and work on it. Because
	of guaranteed RPC ordering and the fact that all commands go through PC this shouldn't
	interrupt anything */
	if (ConstructionInstigatorID != 0)
	{
		// Should be valid; check was made in CanPlaceBuilding()
		AActor * Builder = PS->GetSelectableFromID(ConstructionInstigatorID);
		ISelectable * AsSelectable = CastChecked<ISelectable>(Builder);
		AsSelectable->OnContextMenuPlaceBuildingResult(BuildingWeSpawned, *BuildingInfo, BuildMethod);
	}
}

void ARTSPlayerController::Client_OnPlaceBuildingSuccess_Implementation()
{
	/* Expected to be called on remote clients only */
	assert(!GetWorld()->IsServer());

	if (PendingContextAction == EContextButton::PlacingGhost)
	{
		CancelGhost();
	}
}

void ARTSPlayerController::Client_OnBuildsInTabPlaceBuildingSuccess_Implementation(ABuilding * ProducingBuilding, EBuildingType BuildingType)
{
	/* Expected to be called on remote clients only */
	assert(!GetWorld()->IsServer());

	/* This will erase any ghost even if we switched to another type while waiting for this
	message to arrive */
	if (PendingContextAction == EContextButton::PlacingGhost)
	{
		CancelGhost();
	}

	//if (!GetWorld()->IsServer())
	if (Statics::IsValid(ProducingBuilding))
	{
		/* Remove it from the array, but also housekeep and remove nulls while we're at it.
		Actually housekeeping it shouldn't be required */
		TArray < ABuilding * > & BITCompleteBuildings = PS->GetBuildsInTabCompleteBuildings();
		int32 ArrayNum = BITCompleteBuildings.Num();
		for (int32 i = ArrayNum - 1; i >= 0; --i)
		{
			ABuilding * Elem = BITCompleteBuildings[i];

			if (!Statics::IsValid(Elem) || Elem == ProducingBuilding)
			{
				BITCompleteBuildings.RemoveAt(i, 1, false);
			}
			if (Elem == ProducingBuilding)
			{
				/* Remove item from front of queue. Assuming using persistent queue */
				Elem->GetPersistentProductionQueue()->OnBuildsInTabBuildingPlaced();
			}
		}
	}

	/* Tell HUD to get rid of 'ready to place' text */
	HUD->OnBuildsInTabBuildingPlaced(BuildingType);
}

void ARTSPlayerController::CancelGhost()
{
	assert(GhostBuilding != nullptr);

	/* Do this. If the player was trying to place the building via a selectable context menu then 
	a button will be highlighted */
	UnhighlightHighlightedButton();

	PendingContextAction = EContextButton::None;
	bNeedToRecordGhostLocOnNextTick = false;
	bIsGhostRotationActive = false;
	
#if GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD
	/* This line isn't required so long as in RotateGhost GhostRotationDirection is set when 
	bIsGhostRotationActive is changed to true */
	//GhostRotationDirection = ERotationDirection::NoDirectionEstablished;
	
	GhostAccumulatedMovementTowardsDirection = 0.f;
#endif
	
	GhostBuilding->OnEnterPool();
	GhostBuilding = nullptr;
}

UInGameWidgetBase * ARTSPlayerController::GetWidget(EMatchWidgetType WidgetType, bool bCallSetupWidget)
{
	if (MatchWidgets.Contains(WidgetType))
	{
		assert(MatchWidgets[WidgetType] != nullptr);

		return MatchWidgets[WidgetType];
	}
	/* Need to construct widget first */
	else
	{
		TSubclassOf <UInGameWidgetBase> WidgetBP = nullptr;

		/* Check if a faction specific blueprint is set in faction info before defaulting to one set
		in game instance */
		if (FactionInfo->GetMatchWidgetBP(WidgetType) != nullptr)
		{
			WidgetBP = FactionInfo->GetMatchWidgetBP(WidgetType);
		}
		else
		{
			WidgetBP = GI->GetMatchWidgetBP(WidgetType);
		}

		UE_CLOG(WidgetBP == nullptr, RTSLOG, Fatal, TEXT("Attempt to construct match widget of type "
			"[%s] but no blueprint for it exists. Need to add blueprint for it in game instance or "
			"faction info"), TO_STRING(EMatchWidgetType, WidgetType));
		
		UInGameWidgetBase * Widget = CreateWidget<UInGameWidgetBase>(this, WidgetBP);

		if (bCallSetupWidget)
		{
			Widget->SetupWidget(GI, this);
		}

		MatchWidgets.Emplace(WidgetType, Widget);

		return Widget;
	}
}

UInGameWidgetBase * ARTSPlayerController::ShowWidget(EMatchWidgetType WidgetType, bool bAddToWidgetHistory,
	ESlateVisibility PreviousWidgetVisibility, bool bCallSetupWidget)
{
	/* Making the previous widget still clickable seems like unintended behavior */
	assert(PreviousWidgetVisibility == ESlateVisibility::Collapsed
		|| PreviousWidgetVisibility == ESlateVisibility::Hidden
		|| PreviousWidgetVisibility == ESlateVisibility::HitTestInvisible);

	if (WidgetHistory.Num() > 0)
	{
		GetWidget(WidgetHistory.Last())->SetVisibility(PreviousWidgetVisibility);
	}

	UInGameWidgetBase * Widget = GetWidget(WidgetType, bCallSetupWidget);

	if (Widget->IsInViewport())
	{
		Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		const int32 ZOrder = GetWidgetZOrder(WidgetType);
		Widget->AddToViewport(ZOrder);
	}
	
	if (bAddToWidgetHistory)
	{
		WidgetHistory.Emplace(WidgetType);
	}

	return Widget;
}

UInGameWidgetBase * ARTSPlayerController::ShowPreviousWidget()
{
	UInGameWidgetBase * Widget = GetWidget(WidgetHistory.Last());

	Widget->RemoveFromViewport();
	WidgetHistory.Pop();

	UInGameWidgetBase * PreviousWidget = nullptr;

	if (WidgetHistory.Num() > 0)
	{
		PreviousWidget = GetWidget(WidgetHistory.Last());

		// Make previous widget visible if it is not already 
		PreviousWidget->SetVisibility(ESlateVisibility::Visible);
	}

	return PreviousWidget;
}

void ARTSPlayerController::HideWidget(EMatchWidgetType WidgetType, bool bTryRemoveFromWidgetHistory)
{
	/* This is badish. We SetVisibility to hide it here, but in the function ShowPreviousWidget 
	we use RemoveFromViewport instead. Not consistent. Because I have a couple of calls to 
	RemoveFromViewport already in code and CBF dealing with bugs that might arise if I change it 
	I will just leave this note saying: change it so either both functions use RemoveFromViewport 
	or SetVisibility(Hidden) TODO */
	GetWidget(WidgetType)->SetVisibility(ESlateVisibility::Hidden);

	if (bTryRemoveFromWidgetHistory)
	{
		/* Remove entry from widget history */
		for (int32 i = WidgetHistory.Num() - 1; i >= 0; --i)
		{
			if (WidgetHistory[i] == WidgetType)
			{
				WidgetHistory.RemoveAt(i, 1, false);
				break;
			}
		}
	}
}

bool ARTSPlayerController::IsWidgetBlueprintSet(EMatchWidgetType WidgetType) const
{
	return FactionInfo->GetMatchWidgetBP(WidgetType) != nullptr
		|| GI->IsMatchWidgetBlueprintSet(WidgetType);
}

int32 ARTSPlayerController::GetWidgetZOrder(EMatchWidgetType WidgetType)
{
	/* Doing this since HUD is shown during loading screen and want to make sure it doesn't
	get drawn on top of it */
	return static_cast<int32>(WidgetType) - (UINT8_MAX + 1);
}

uint8 ARTSPlayerController::GetBuildingIndex(EBuildingType BuildingType) const
{
	return static_cast<uint8>(BuildingType);
}

void ARTSPlayerController::OnRep_PlayerState()
{
	// Does not get called on server from my testing

	Super::OnRep_PlayerState();
}

void ARTSPlayerController::SetupReferences()
{
	AssignGI();
	AssignPS();
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
}

void ARTSPlayerController::AssignGI()
{
	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());
}

void ARTSPlayerController::AssignPS()
{
	PS = CastChecked<ARTSPlayerState>(PlayerState);
}

void ARTSPlayerController::Delay(FTimerHandle & TimerHandle, void(ARTSPlayerController::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

bool ARTSPlayerController::IsContextActionPending() const
{
	/* If this throws you should call IsPlacingGhost before calling this. Assert here for 
	performance */
	assert(PendingContextAction != EContextButton::PlacingGhost);
	
	/* Maybe able to get rid of second != check */
	return PendingContextAction != EContextButton::None
		&& PendingContextAction != EContextButton::RecentlyExecuted;
}

bool ARTSPlayerController::IsGlobalSkillsPanelAbilityPending() const
{
	return (PendingCommanderAbility != nullptr);
}

void ARTSPlayerController::SendMouseLocToMarqueeHUD()
{
	MarqueeHUD->SetMouseLoc(GetMouseCoords());
}

bool ARTSPlayerController::OnLMBReleased_WasUIButtonClicked(UMyButton * ButtonLMBReleasedOn) const
{
	assert(ButtonLMBReleasedOn != nullptr);
	return (ButtonLMBReleasedOn == ButtonPressedOnLMBPressed);
}

bool ARTSPlayerController::OnRMBReleased_WasUIButtonClicked(UMyButton * ButtonRMBReleasedOn) const
{
	assert(ButtonRMBReleasedOn != nullptr);
	return (ButtonRMBReleasedOn == ButtonPressedOnRMBPressed);
}

bool ARTSPlayerController::IsASelectable(AActor * Actor) const
{
	assert(Actor != nullptr);

	return Actor->GetClass()->ImplementsInterface(USelectable::StaticClass());
}

void ARTSPlayerController::SetSpringArmTargetArmLength(float NewLength)
{
	SpringArm->TargetArmLength = NewLength;
	AdjustWorldWidgetLocations();
}

void ARTSPlayerController::AdjustWorldWidgetLocations()
{
	if (GI->GetWorldWidgetViewMode() == EWorldWidgetViewMode::NoChange)
	{
		// If NoChange then nothing needs to be done
		return;
	}
	
	// Assumed some other method name TBD (COH2). 

	const float NewCameraZoomAmount = SpringArm->TargetArmLength;
	const float CameraPitch = -PlayerCameraManager->GetCameraRotation().Pitch;

	if (Type == EPlayerType::Player)
	{
		// Persistent world widgets for buildings and infantry
		for (ARTSPlayerState * Elem : GS->GetPlayerStates())
		{
			// Check quick case 
			if (Elem->GetFI()->GetSelectablePersistentWorldWidget() != nullptr)
			{
				for (ABuilding * Building : Elem->GetBuildings())
				{
					Building->AdjustPersistentWorldWidgetForNewCameraZoomAmount(NewCameraZoomAmount, CameraPitch);
				}

				for (AInfantry * Infantry : Elem->GetUnits())
				{
					Infantry->AdjustPersistentWorldWidgetForNewCameraZoomAmount(NewCameraZoomAmount, CameraPitch);
				}
			}
		}

		// Persistent world widgets for resource spots
		for (AResourceSpot * ResourceSpot : GS->GetAllResourceSpots())
		{
			ResourceSpot->AdjustPersistentWorldWidgetForNewCameraZoomAmount(NewCameraZoomAmount, CameraPitch);
		}
	}
	else // Assumed Observer
	{
		// Persistent world widgets for buildings and infantry
		if (GI->GetObserverSelectablePersistentWorldWidget() != nullptr)
		{
			for (ARTSPlayerState * Elem : GS->GetPlayerStates())
			{
				for (ABuilding * Building : Elem->GetBuildings())
				{
					Building->AdjustPersistentWorldWidgetForNewCameraZoomAmount(NewCameraZoomAmount, CameraPitch);
				}

				for (AInfantry * Infantry : Elem->GetUnits())
				{
					Infantry->AdjustPersistentWorldWidgetForNewCameraZoomAmount(NewCameraZoomAmount, CameraPitch);
				}
			}
		}
		
		// Persistent world widgets for resource spots
		if (GI->GetObserverResourceSpotPersistentWorldWidget() != nullptr)
		{
			for (AResourceSpot * ResourceSpot : GS->GetAllResourceSpots())
			{
				ResourceSpot->AdjustPersistentWorldWidgetForNewCameraZoomAmount(NewCameraZoomAmount, CameraPitch);
			}
		}
	}

	// World widgets for selected
	for (AActor * Selectable : Selected)
	{
		CastChecked<ISelectable>(Selectable)->AdjustSelectedWorldWidgetForNewCameraZoomAmount(NewCameraZoomAmount, CameraPitch);
	}
}

void ARTSPlayerController::SingleSelect(AActor * SelectedActor)
{
	ISelectable * const AsSelectable = CastChecked<ISelectable>(SelectedActor);

	/* Call OnDeselect on all selected except the one just selected */
	RemoveSelection(AsSelectable);

	/* TODO: shouldn't this crash if array is size 0?. Maybe Selected is
	growing in size unexpectedly */
	Selected.Insert(SelectedActor, 0);
	assert(Selected.Num() == 1);

	CurrentSelected = TScriptInterface<ISelectable>(SelectedActor);

	const FSelectableAttributesBase & SelectedInfo = AsSelectable->GetAttributesBase();

	/* Figure out who controls the actor just selected */
	if (IsControlledByThis(SelectedInfo.GetOwnerID()))
	{
		HUD->OnPlayerCurrentSelectedChanged(CurrentSelected);
		AddToSelectedIDs(AsSelectable->GetSelectableID());
	}
	else
	{
		/* Either allied, neutral or hostile */
		HUD->OnPlayerCurrentSelectedChanged(CurrentSelected);
		
		//AddToSelectedIDs(AsSelectable->GetSelectableID());
		//SelectedIDs.Reset(); // Recently added this. Then commented it because I think it's being done earlier
	}

	// Possibly play a sound
	if (ShouldPlaySelectionSound(SelectedInfo))
	{
		PlaySelectablesSelectionSound();
	}

	if (IsControlledByThis(SelectedInfo.GetOwnerID()))
	{
		TimeOfLastSelection_Owned = GetWorld()->GetRealTimeSeconds();
	}

	AsSelectable->OnSingleSelect();
	/* Do this cause in all my OnSingleSelect functions I always set it to false */
	AsSelectable->GetAttributesBase().SetIsPrimarySelected(true);
}

bool ARTSPlayerController::ShouldPlaySelectionSound(const FSelectableAttributesBase & CurrentSelectedInfo) const
{
	// Check current selected is owned by us. Can also add more checks here if desired such as 
	// if a 'unit just built' sound is playing or whatever
	if (IsControlledByThis(CurrentSelectedInfo.GetOwnerID()))
	{
		// If enough time has passed then play sound
		return (GetWorld()->GetRealTimeSeconds() - TimeOfLastSelection_Owned) > 0.8f;
	}

	return false;
}

void ARTSPlayerController::PlaySelectablesSelectionSound()
{
	USoundBase * SoundToPlay = CurrentSelected->GetAttributes()->GetSelectionSound();

	if (SoundToPlay != nullptr)
	{
		GI->PlayPlayerSelectionSound(SoundToPlay);
	}
}

bool ARTSPlayerController::ShouldPlayMoveCommandSound(const TScriptInterface<ISelectable>& SelectableToPlaySoundFor) const
{
	// Return true if not already playing a command sound
	return GI->IsPlayingPlayerCommandSound() == false;
	/* Might actually wanna IsValid test here */
}

void ARTSPlayerController::PlayMoveCommandSound(const TScriptInterface<ISelectable>& SelectableToPlaySoundFor)
{
	// Will crash if not infantry
	USoundBase * SoundToPlay = SelectableToPlaySoundFor->GetInfantryAttributes()->GetMoveCommandSound();

	if (SoundToPlay != nullptr)
	{
		GI->PlayPlayerCommandSound(SoundToPlay);
	}
}

bool ARTSPlayerController::ShouldPlayCommandSound(const TScriptInterface<ISelectable>& SelectableToPlaySoundFor, EContextButton Command)
{
	// Return true if not already playing a command sound
	return GI->IsPlayingPlayerCommandSound() == false;
}

void ARTSPlayerController::PlayCommandSound(const TScriptInterface<ISelectable>& SelectableToPlaySoundFor, EContextButton Command)
{
	USoundBase * SoundToPlay = SelectableToPlaySoundFor->GetAttributes()->GetCommandSound(Command);

	if (SoundToPlay != nullptr)
	{
		GI->PlayPlayerCommandSound(SoundToPlay);
	}
}

bool ARTSPlayerController::ShouldPlayChangeRallyPointSound() const
{
	// Always play change rally point sound
	return true;
}

void ARTSPlayerController::PlayChangeRallyPointSound()
{
	USoundBase * SoundToPlay = FactionInfo->GetChangeRallyPointSound();

	if (SoundToPlay != nullptr)
	{
		GI->PlayEffectSound(SoundToPlay);
	}
}

bool ARTSPlayerController::ShouldPlayChatMessageReceivedSound(EMessageRecipients MessageType) const
{
	// Always play chat message received sound
	return true;
}

void ARTSPlayerController::PlayChatMessageReceivedSound(EMessageRecipients MessageType)
{
	USoundBase * SoundToPlay = GI->GetChatMessageReceivedSound(MessageType);

	if (SoundToPlay != nullptr)
	{
		Statics::PlaySound2D(this, SoundToPlay);
	}
}

bool ARTSPlayerController::ShouldPlayZeroHealthSound(AActor * Selectable, USoundBase * SoundRequested) const
{
	return (GI->IsPlayingZeroHealthSound() == false);
}

void ARTSPlayerController::PlayZeroHealthSound(USoundBase * Sound)
{
	if (Sound != nullptr)
	{
		GI->PlayZeroHealthSound(Sound);
	}
}

void ARTSPlayerController::SetPerformMarqueeNextTick(bool bNewValue)
{
	MarqueeHUD->SetPerformMarqueeASAP(bNewValue);
}

EGameWarning ARTSPlayerController::IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, bool bCheckCharges) const
{
	const FCommanderAbilityInfo * AbilityInfo = AbilityState->GetAbilityInfo();
	
	/* Check if the ability has at least one charge remaining */
	if (bCheckCharges && AbilityInfo->HasUnlimitedUses() == false)
	{
		if (AbilityState->GetNumTimesUsed() >= AbilityInfo->GetNumUses())
		{
			return EGameWarning::AllAbilityChargesUsed;
		}
	}

	/* Check if off cooldown */
	if (AbilityState->GetCooldownRemaining(GetWorldTimerManager()) > 0.f)
	{
		return EGameWarning::ActionOnCooldown;
	}

	return EGameWarning::None;
}

EGameWarning ARTSPlayerController::IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, ARTSPlayerState * AbilityTarget, bool bCheckCharges) const
{
	/* Check cooldown and ability charges */
	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, bCheckCharges);
	if (Warning == EGameWarning::None)
	{
		/* Check if the ability is allowed to target the player */
		return AbilityTarget->CanBeTargetedByAbility(*AbilityState->GetAbilityInfo(), PS);
	}
	else
	{
		return Warning;
	}
}

EGameWarning ARTSPlayerController::IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, 
	AActor * TargetAsActor, ISelectable * AbilityTarget, bool bCheckCharges) const
{
	const FCommanderAbilityInfo * AbilityInfo = AbilityState->GetAbilityInfo();

	/* Check the cooldown and charges checks */
	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, bCheckCharges);
	if (Warning == EGameWarning::None)
	{
		const ETeam TargetsTeam = AbilityTarget->GetAttributesBase().GetTeam();
		
		/* Check if target is the right affiliation */
		if (TargetsTeam != Team)
		{
			if (AbilityInfo->CanTargetEnemies() == false)
			{
				return EGameWarning::AbilityCannotTargetHostiles;
			}
		}
		else 
		{
			if (AbilityInfo->CanTargetFriendlies() == false)
			{
				return EGameWarning::AbilityCannotTargetFriendlies;
			}
		}

		/* Check if target is the type of selectable that can be targeted by the ability */
		if (Statics::CanTypeBeTargeted(TargetAsActor, AbilityInfo->GetAcceptableSelectableTargetFNames()))
		{
			return EGameWarning::None;
		}
		else
		{
			return EGameWarning::InvalidTarget;
		}
	}
	else
	{
		return Warning;
	}	
}

EGameWarning ARTSPlayerController::IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, const FVector & AbilityLocation, bool bCheckCharges) const
{
	const FCommanderAbilityInfo * AbilityInfo = AbilityState->GetAbilityInfo();

	/* Check the cooldown and charges checks */
	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, bCheckCharges);
	if (Warning == EGameWarning::None)
	{
		if (AbilityInfo->CanTargetInsideFog() == false)
		{
			if (Statics::IsLocationOutsideFog(AbilityLocation, Team, FogOfWarManager))
			{
				return EGameWarning::None;
			}
			else
			{
				return EGameWarning::AbilityLocationInsideFog;
			}
		}
		else
		{
			return EGameWarning::None;
		}
	}
	else
	{
		return Warning;
	}
}

void ARTSPlayerController::HighlightButton(UMyButton * ButtonToHighlight)
{
	HighlightedButton = ButtonToHighlight;
	ButtonToHighlight->SetIsHighlighted(true);
}

void ARTSPlayerController::UnhighlightHighlightedButton()
{
	if (HighlightedButton != nullptr)
	{
		HighlightedButton->SetIsHighlighted(false);
		HighlightedButton = nullptr;
	}
}

void ARTSPlayerController::UpdateMouseAppearance(const FContextButtonInfo & ButtonInfo)
{
	const EAbilityMouseAppearance MouseAppearanceOption = ButtonInfo.GetMouseAppearanceOption();

	if (MouseAppearanceOption == EAbilityMouseAppearance::CustomMouseCursor)
	{
		/* Make mouse cursor visible and use mouse cursor. Just using default for now. If there is
		already a selectable under cursor then will have to wait till tick to update it */
		bShowMouseCursor = true;
		SetMouseCursor(ButtonInfo.GetDefaultCursorInfo());
		/* A crash on this line may incidate the path to your hardware cursor is not valid */
	}
	else if (MouseAppearanceOption == EAbilityMouseAppearance::HideAndShowDecal)
	{
		/* Make mouse cursor hidden and draw a decal under it */
		bShowMouseCursor = false;
		SetContextDecal(ButtonInfo);

		UnhoverHoveredUIElement();
	}
	else
	{
		assert(MouseAppearanceOption == EAbilityMouseAppearance::NoChange);
		/* Nothing needed to be done */
	}
}

void ARTSPlayerController::GetMouseCursor_NoAbilitiesPending(AActor * HoveredActor, ISelectable * HoveredSelectable, EAffiliation HoveredAffiliation, const FCursorInfo *& OutCursor) const
{
	assert(HasSelection() && IsSelectionControlledByThis());

	if (HoveredSelectable->GetAttributesBase().GetBuildingType() == EBuildingType::ResourceSpot)
	{
		AResourceSpot * ResourceSpot = CastChecked<AResourceSpot>(HoveredActor);

		OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CannotGatherFromHoveredResourceSpot(ResourceSpot->GetType());

		for (int32 i = 0; i < Selected.Num(); ++i)
		{
			ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
			const FInfantryAttributes * Attributes = Selectable->GetInfantryAttributes();
			if (Attributes != nullptr)
			{
				if (Attributes->ResourceGatheringProperties.CanGatherResource(ResourceSpot->GetType()))
				{
					OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CanGatherFromHoveredResourceSpot(ResourceSpot->GetType());
					break;
				}
			}
		}
	}
	else if (HoveredSelectable->GetAttributesBase().GetSelectableType() == ESelectableType::Unit)
	{
		if (HoveredAffiliation == EAffiliation::Owned || HoveredAffiliation == EAffiliation::Allied)
		{
			OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CannotAttackHoveredFriendlyUnit();

			for (int32 i = 0; i < Selected.Num(); ++i)
			{
				ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
				if (Selectable->CanAquireTarget(HoveredActor, HoveredSelectable))
				{
					if (Selectable->GetSelectedMouseCursor_CanAttackFriendlyUnit(GI, HoveredActor, HoveredSelectable, HoveredAffiliation) != nullptr)
					{
						OutCursor = Selectable->GetSelectedMouseCursor_CanAttackFriendlyUnit(GI, HoveredActor, HoveredSelectable, HoveredAffiliation);
					}
					else if (FactionInfo->GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyUnit().ContainsCustomCursor())
					{
						OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyUnit();
					}
					break;
				}
			}
		}
		else if (HoveredAffiliation == EAffiliation::Hostile)
		{
			OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CannotAttackHoveredHostileUnit();

			for (int32 i = 0; i < Selected.Num(); ++i)
			{
				ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
				if (Selectable->CanAquireTarget(HoveredActor, HoveredSelectable))
				{
					if (Selectable->GetSelectedMouseCursor_CanAttackHostileUnit(GI, HoveredActor, HoveredSelectable) != nullptr)
					{
						OutCursor = Selectable->GetSelectedMouseCursor_CanAttackHostileUnit(GI, HoveredActor, HoveredSelectable);
					}
					else if (FactionInfo->GetDefaultMouseCursorInfo_CanAttackHoveredHostileUnit().ContainsCustomCursor())
					{
						OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CanAttackHoveredHostileUnit();
					}
					break;
				}
			}
		}
		else { } // Neutral and Observed, do we want to do something with these?
	}
	else if (HoveredSelectable->GetAttributesBase().GetSelectableType() == ESelectableType::Building)
	{
		ABuilding * Building = CastChecked<ABuilding>(HoveredActor);
		const EFaction BuildingsFaction = Building->GetFaction();
		const EBuildingType BuildingType = Building->GetType();
		const FCursorInfo * AttackCursor = nullptr;

		if (HoveredAffiliation == EAffiliation::Owned)
		{
			const FBuildingAttributes * BuildingAttributes = Building->GetBuildingAttributes();
			if (BuildingAttributes->GetGarrisonAttributes().IsGarrison())
			{
				/* Check if we have at least 1 unit selected since only units have the possibility 
				of entering a garrison. This probably relies on the fact that buildings and 
				units cannot be selected at the same time cause I just check primary selected */
				if (GetCurrentSelected()->GetAttributesBase().GetSelectableType() == ESelectableType::Unit)
				{
					/* If the building is a garrison then always show the 'can enter' mouse cursor
					for the garrison. Maybe wanna actually check if anything can actually enter the
					garrison in the future though? Maybe not */
					OutCursor = &BuildingAttributes->GetGarrisonAttributes().GetCanEnterMouseCursor(GI);
					return;
				}
			}
			
			OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CannotAttackHoveredFriendlyBuilding();

			for (int32 i = 0; i < Selected.Num(); ++i)
			{
				ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
				/* If we find an ability e.g. engineers or spies in C&C then use the cursor for
				that. Otherwise use an attack cursor */
				const FBuildingTargetingAbilityPerSelectableInfo * SpecialBuildingTargetingAbilityInfo = Selectable->GetSpecialRightClickActionTowardsBuildingInfo(Building, BuildingsFaction, BuildingType, HoveredAffiliation);
				if (SpecialBuildingTargetingAbilityInfo != nullptr)
				{
					OutCursor = &SpecialBuildingTargetingAbilityInfo->GetAcceptableTargetMouseCursor(GI);
					return;
				}
				else if (AttackCursor == nullptr && Selectable->CanAquireTarget(HoveredActor, HoveredSelectable))
				{
					AttackCursor = Selectable->GetSelectedMouseCursor_CanAttackFriendlyBuilding(GI, Building, HoveredAffiliation);
					/* If selectable specific one is null then try the faction's one */
					if (AttackCursor == nullptr)
					{
						AttackCursor = &FactionInfo->GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyBuilding();
					}
				}
			}

			if (AttackCursor != nullptr)
			{
				OutCursor = AttackCursor;
			}
		}
		else if (HoveredAffiliation == EAffiliation::Allied)
		{
			OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CannotAttackHoveredFriendlyBuilding();

			for (int32 i = 0; i < Selected.Num(); ++i)
			{
				ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
				/* If we find an ability e.g. engineers or spies in C&C then use the cursor for
				that. Otherwise use an attack cursor */
				const FBuildingTargetingAbilityPerSelectableInfo * SpecialBuildingTargetingAbilityInfo = Selectable->GetSpecialRightClickActionTowardsBuildingInfo(Building, BuildingsFaction, BuildingType, HoveredAffiliation);
				if (SpecialBuildingTargetingAbilityInfo != nullptr)
				{
					OutCursor = &SpecialBuildingTargetingAbilityInfo->GetAcceptableTargetMouseCursor(GI);
					return;
				}
				else if (AttackCursor == nullptr && Selectable->CanAquireTarget(HoveredActor, HoveredSelectable))
				{
					AttackCursor = Selectable->GetSelectedMouseCursor_CanAttackFriendlyBuilding(GI, Building, HoveredAffiliation);
					/* If selectable specific one is null then try the faction's one */
					if (AttackCursor == nullptr)
					{
						AttackCursor = &FactionInfo->GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyBuilding();
					}
				}
			}

			if (AttackCursor != nullptr)
			{
				OutCursor = AttackCursor;
			}
		}
		else if (HoveredAffiliation == EAffiliation::Hostile)
		{
			OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CannotAttackHoveredHostileBuilding();

			for (int32 i = 0; i < Selected.Num(); ++i)
			{
				ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
				/* If we find an ability e.g. engineers or spies in C&C then use the cursor for
				that. Otherwise use an attack cursor */
				const FBuildingTargetingAbilityPerSelectableInfo * SpecialBuildingTargetingAbilityInfo = Selectable->GetSpecialRightClickActionTowardsBuildingInfo(Building, BuildingsFaction, BuildingType, HoveredAffiliation);
				if (SpecialBuildingTargetingAbilityInfo != nullptr)
				{
					const FCursorInfo & Cursor = SpecialBuildingTargetingAbilityInfo->GetAcceptableTargetMouseCursor(GI);
					if (Cursor.ContainsCustomCursor())
					{
						OutCursor = &Cursor;
					}
					return;
				}
				else if (AttackCursor == nullptr && Selectable->CanAquireTarget(HoveredActor, HoveredSelectable))
				{
					AttackCursor = Selectable->GetSelectedMouseCursor_CanAttackHostileBuilding(GI, Building);
					/* If selectable specific one is null then try the faction's one */
					if (AttackCursor == nullptr)
					{
						AttackCursor = &FactionInfo->GetDefaultMouseCursorInfo_CanAttackHoveredHostileBuilding();
					}
				}
			}

			if (AttackCursor != nullptr)
			{
				OutCursor = AttackCursor;
			}
		}
		else { } // Neutral and Observed, do we want to do something with these?
	}
	else // Assumed it's an inventory item that is being hovered
	{
		assert(HoveredSelectable->GetAttributesBase().GetSelectableType() == ESelectableType::InventoryItem);
		
		OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CannotPickUpHoveredInventoryItem();

		AInventoryItem * InventoryItem = CastChecked<AInventoryItem>(HoveredActor);
		/* Check if at least one selected can pick up the item. If yes we'll use the 'can pick
		up' cursor. Otherwise we leave it as the 'cannot pick up' cursor */
		for (int32 i = 0; i < Selected.Num(); ++i)
		{
			ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
			const FInventory * Inventory = Selectable->GetInventory();
			if (Inventory != nullptr)
			{
				const EGameWarning Warning = Inventory->CanItemEnterInventory(InventoryItem, Selectable);
				if (Warning == EGameWarning::None)
				{
					OutCursor = &FactionInfo->GetDefaultMouseCursorInfo_CanPickUpHoveredInventoryItem();
					break;
				}
			}
		}
	}
}

void ARTSPlayerController::SetMouseCursor(const FCursorInfo & MouseCursorInfo, bool bFirstCall)
{	
	/* Assumed that cursor path is valid */

	/* Only change mouse cursor if different from current cursor or first time calling this
	function */
	if (bFirstCall || CurrentCursor->GetFullPath() != MouseCursorInfo.GetFullPath())
	{
		const bool bResult = UWidgetBlueprintLibrary::SetHardwareCursor(this, EMouseCursor::Default,
			MouseCursorInfo.GetFullPath(), MouseCursorInfo.GetHotSpot());
		
		UE_CLOG(bResult == false, RTSLOG, Fatal, TEXT("Did not successfully set mouse cursor. "
			"Path was: \"%s\". Likely no .ani, .cur or .png file with that name exists there. "
			"Make sure in your game instance to NOT include the .ani, .cur or .png part. "), 
			*MouseCursorInfo.GetFullPath().ToString());

		CurrentCursor = &MouseCursorInfo;
	}
}

void ARTSPlayerController::SetMouseDecal(const FBasicDecalInfo & DecalInfo)
{
	// If thrown then check you have set a decal for your ability
	assert(DecalInfo.GetDecal() != nullptr);
	
	const FVector Bounds = FVector(MouseDecalHeight, DecalInfo.GetRadius(), DecalInfo.GetRadius());

	MouseDecal = UGameplayStatics::SpawnDecalAtLocation(this, DecalInfo.GetDecal(),
		Bounds, HitResult.Location, FRotator(-90.f, 0.f, 0.f));
	assert(MouseDecal != nullptr);
}

void ARTSPlayerController::SetContextDecal(const FContextButtonInfo & ButtonInfo)
{
	const FVector Bounds = FVector(MouseDecalHeight, ButtonInfo.GetAcceptableLocationDecal().GetRadius(),
		ButtonInfo.GetAcceptableLocationDecal().GetRadius());

	MouseDecal = UGameplayStatics::SpawnDecalAtLocation(this, ButtonInfo.GetAcceptableLocationDecal().GetDecal(),
		Bounds, HitResult.Location, FRotator(-90.f, 0.f, 0.f));
	assert(MouseDecal != nullptr);
}

void ARTSPlayerController::SetContextDecalType(const FContextButtonInfo & AbilityInfo, EAbilityDecalType DecalTypeToSet)
{
	if (DecalTypeToSet == EAbilityDecalType::UsableLocation)
	{
		if (MouseDecal->GetDecalMaterial() != AbilityInfo.GetAcceptableLocationDecal().GetDecal())
		{
			MouseDecal->SetDecalMaterial(AbilityInfo.GetAcceptableLocationDecal().GetDecal());
			// Not sure if need a setter or if this line will work, will see
			MouseDecal->DecalSize = FVector(MouseDecalHeight, AbilityInfo.GetAcceptableLocationDecal().GetRadius(), 
				AbilityInfo.GetAcceptableLocationDecal().GetRadius());
		}
	}
	else // Assumed NotUsableLocation
	{
		assert(DecalTypeToSet == EAbilityDecalType::NotUsableLocation);

		/* Only use decal if one is set */
		if (AbilityInfo.GetUnusableLocationDecal().GetDecal() != nullptr
			&& MouseDecal->GetDecalMaterial() != AbilityInfo.GetUnusableLocationDecal().GetDecal())
		{
			MouseDecal->SetDecalMaterial(AbilityInfo.GetUnusableLocationDecal().GetDecal());
			// Not sure if need a setter or if this line will work, will see
			MouseDecal->DecalSize = FVector(MouseDecalHeight, AbilityInfo.GetUnusableLocationDecal().GetRadius(),
				AbilityInfo.GetUnusableLocationDecal().GetRadius());
		}
	}
}

void ARTSPlayerController::SetCommanderAbilityDecalType(const FCommanderAbilityInfo & AbilityInfo, EAbilityDecalType DecalTypeToSet)
{
	if (DecalTypeToSet == EAbilityDecalType::UsableLocation)
	{
		if (MouseDecal->GetDecalMaterial() != AbilityInfo.GetAcceptableLocationDecalInfo().GetDecal())
		{
			MouseDecal->SetDecalMaterial(AbilityInfo.GetAcceptableLocationDecalInfo().GetDecal());
			MouseDecal->DecalSize = FVector(MouseDecalHeight, AbilityInfo.GetAcceptableLocationDecalInfo().GetRadius(),
				AbilityInfo.GetAcceptableLocationDecalInfo().GetRadius());
		}
	}
	else // Assumed NotUsableLocation
	{
		assert(DecalTypeToSet == EAbilityDecalType::NotUsableLocation);

		if (AbilityInfo.GetUnusableLocationDecalInfo().GetDecal() != nullptr
			&& MouseDecal->GetDecalMaterial() != AbilityInfo.GetUnusableLocationDecalInfo().GetDecal())
		{
			MouseDecal->SetDecalMaterial(AbilityInfo.GetUnusableLocationDecalInfo().GetDecal());
			MouseDecal->DecalSize = FVector(MouseDecalHeight, AbilityInfo.GetUnusableLocationDecalInfo().GetRadius(),
				AbilityInfo.GetUnusableLocationDecalInfo().GetRadius());
		}
	}
}

void ARTSPlayerController::ResetMouseAppearance()
{
	SetMouseCursor(*ScreenLocationCurrentCursor);
	bShowMouseCursor = true;
	HideContextDecal();
}

void ARTSPlayerController::HideContextDecal()
{
	if (MouseDecal != nullptr)
	{
		MouseDecal->DestroyComponent();
	}
	// May be needed if getting errors
	//MouseDecal = nullptr;
}

void ARTSPlayerController::UpdateSelectableUnderMouse(float DeltaTime)
{
	/* ObjectUnderMouseOnTick = selectable under mouse on previous tick */

	if (IsMarqueeActive())
	{
		return;
	}

	AActor * const HitActor = HitResult.GetActor();

	TScriptInterface<ISelectable> AsSelectable = TScriptInterface<ISelectable>(HitActor);
	if (AsSelectable != nullptr && Statics::CanBeSelected(HitActor))
	{
		if (AsSelectable != ObjectUnderMouseOnTick)
		{
			/* Hit a selectable that is different than previous one under mouse */

#if !INSTANT_SHOWING_SELECTABLE_TOOLTIPS
			AccumulatedTimeSpentHoveringSelectable = FMath::Min<float>(AccumulatedTimeSpentHoveringSelectable, TooltipOptions::SELECTABLE_HOVER_TIME_REQUIREMENT);
			AccumulatedTimeSpentHoveringSelectable -= TooltipOptions::SELECTABLE_HOVER_TIME_SWITCH_PENALTY;
			AccumulatedTimeSpentHoveringSelectable = FMath::Max<float>(0.f, AccumulatedTimeSpentHoveringSelectable);
#if USING_SELECTABLE_HOVER_TIME_DECAY_DELAY
			TimeSpentNotHoveringSelectable = 0.f;
#endif
#endif // !INSTANT_SHOWING_SELECTABLE_TOOLTIPS

			if (ObjectUnderMouseOnTick != nullptr)
			{
				ObjectUnderMouseOnTick->OnMouseUnhover(this);
			}

#if !INSTANT_SHOWING_SELECTABLE_TOOLTIPS
			bShowingSelectablesTooltip = false;
#endif

			AsSelectable->OnMouseHover(this);
			
			ObjectUnderMouseOnTick = AsSelectable;
		}
	}
	else
	{
		UnhoverPreviouslyHoveredSelectable();
	}
}

void ARTSPlayerController::UnhoverPreviouslyHoveredSelectable()
{
	if (ObjectUnderMouseOnTick != nullptr)
	{
		ObjectUnderMouseOnTick->OnMouseUnhover(this);
		ObjectUnderMouseOnTick = nullptr;

#if !INSTANT_SHOWING_SELECTABLE_TOOLTIPS
		AccumulatedTimeSpentHoveringSelectable = FMath::Min<float>(AccumulatedTimeSpentHoveringSelectable, TooltipOptions::SELECTABLE_HOVER_TIME_REQUIREMENT);
#endif
	}
}

void ARTSPlayerController::UpdateMouseDecalLocation(float DeltaTime)
{
	if (MouseDecal != nullptr)
	{
		MouseDecal->SetWorldLocation(HitResult.Location);
	}
}

void ARTSPlayerController::Server_GiveLayFoundationCommand_Implementation(EBuildingType BuildingType,
	const FVector_NetQuantize10 & Location, const FRotator & Rotation, uint8 BuildersID)
{
	/* Check builder is still valid and if so give order to go lay foundations. No additional
	checks are made - they will be made when selectable calls Server_PlaceBuilding when at
	construction site */
	AActor * Builder = PS->GetSelectableFromID(BuildersID);
	if (Statics::IsValid(Builder) && !Statics::HasZeroHealth(Builder))
	{
		ISelectable * AsSelectable = CastChecked<ISelectable>(Builder);
		AsSelectable->OnLayFoundationCommand(BuildingType, Location, Rotation);
	}
	else
	{
		PS->OnGameWarningHappened(EGameWarning::BuilderDestroyed);
	}
}

void ARTSPlayerController::PrepareContextCommandRPC(EContextButton CommandType, const FVector & ClickLoc, AActor * ClickTarget)
{
	assert(!GetWorld()->IsServer());

	// Should not have called this if this is the case
	assert(Statics::IsValid(ClickTarget));

	/* This is gonna be tough to enforce. Whenever Selected gets updated SelectedIDs
	needs to get updated too */
	assert(Selected.Num() == SelectedIDs.Num());

	/* 2 ways this could be implemented: 
	1. Send IDs of every selected selectable. Means server must check which units have button 
	on their context menu, but this check is fast. This is how I currently do it 
	2. Send only IDs of selectables in SelectedAndSupportPendingContextAction. Server could 
	trust client and not check if button is on context menu but for security would probably 
	still do it anyway. But the amount of array iteration is reduced for server. 
	Disadvantage is that we have to send new IDs more often generally. We send ability 
	IDs, but then do a right click and have to resend all IDs 
	Hmmm interesting one to think about. Also instant context commands can do it a similar 
	way too */

	/* Check if we need to send the selectable IDs because the selectables we are
	issuing orders to are different from the group of the last order */
	bool bSendNewIDs = false;

	if (SelectedIDs.Num() == CommandIDs.Num())
	{
		for (const auto & Elem : CommandIDs)
		{
			if (!SelectedIDs.Contains(Elem))
			{
				bSendNewIDs = true;
				break;
			}
		}
	}
	else
	{
		bSendNewIDs = true;
	}

	if (bSendNewIDs)
	{
		CommandIDs.Reset();

		FContextCommandWithTarget Command = FContextCommandWithTarget(CommandType, false, ClickLoc,
			ClickTarget);

		/* Attach selectables affected by command to it and remember what we sent */
		for (const auto & SelectableID : SelectedIDs)
		{
			Command.AddSelectable(SelectableID);
			CommandIDs.Emplace(SelectableID);
		}

		Server_IssueContextCommand(Command);
	}
	else
	{
		/* Server will use the selectable already in Selected server-side */
		const FContextCommandWithTarget Command = FContextCommandWithTarget(CommandType, true, 
			ClickLoc, ClickTarget);
		Server_IssueContextCommand(Command);
	}
}

void ARTSPlayerController::PrepareContextCommandRPC(EContextButton CommandType, const FVector & ClickLoc)
{
	assert(!GetWorld()->IsServer());
	assert(Selected.Num() == SelectedIDs.Num());

	bool bSendNewIDs = false;

	if (SelectedIDs.Num() == CommandIDs.Num())
	{
		for (const auto & Elem : CommandIDs)
		{
			if (!SelectedIDs.Contains(Elem))
			{
				bSendNewIDs = true;
				break;
			}
		}
	}
	else
	{
		bSendNewIDs = true;
	}

	if (bSendNewIDs)
	{
		CommandIDs.Reset();
		
		FContextCommandWithLocation Command = FContextCommandWithLocation(CommandType, false, ClickLoc);

		/* Attach selectables affected by command to it and remember what we sent */
		for (const auto & SelectableID : SelectedIDs)
		{
			Command.AddSelectable(SelectableID);
			CommandIDs.Emplace(SelectableID);
		}

		Server_IssueLocationTargetingContextCommand(Command);
	}
	else
	{
		/* Server will use the selectable already in Selected server-side */
		Server_IssueLocationTargetingContextCommand(FContextCommandWithLocation(CommandType, true, ClickLoc));
	}
}

void ARTSPlayerController::Server_IssueContextCommand_Implementation(const FContextCommandWithTarget & Command)
{
	/* Does not need to be called by the server player. */
	assert(GetWorld()->GetFirstPlayerController() != this);
	
	/* Just like right-click commands it would probably be faster to not bother changing
	Selected and just look up each ID in PS and call function on that actor but I don't
	want to rewrite that function so for now update Selected */

	/* Construct button */
	const FContextButton Button = FContextButton(Command);

	if (!Command.bSameGroup)
	{
		Selected.Reset();

		for (const auto & SelectableID : Command.AffectedSelectables)
		{
			AActor * Selectable = PS->GetSelectableFromID(SelectableID);
			Selected.Emplace(Selectable);
		}
	}

	const FContextButtonInfo & CommandInfo = GI->GetContextInfo(Button.GetButtonType());

	IssueContextCommand(CommandInfo, Command.ClickLocation, Command.Target);
}

void ARTSPlayerController::Server_IssueLocationTargetingContextCommand_Implementation(const FContextCommandWithLocation & Command)
{
	/* Does not need to be called by the server player. */
	assert(GetWorld()->GetFirstPlayerController() != this);

	/* Construct button */
	const FContextButton Button = FContextButton(Command);

	if (!Command.bSameGroup)
	{
		Selected.Reset();

		for (const auto & SelectableID : Command.AffectedSelectables)
		{
			AActor * Selectable = PS->GetSelectableFromID(SelectableID);
			Selected.Emplace(Selectable);
		}
	}

	const FContextButtonInfo & CommandInfo = GI->GetContextInfo(Button.GetButtonType());

	IssueContextCommand(CommandInfo, Command.GetClickLocation());
}

void ARTSPlayerController::Server_IssueInstantContextCommand_Implementation(const FContextCommand & Command)
{
	/* Does not need to be called by the server player. */
	assert(GetWorld()->GetFirstPlayerController() != this);
	
	/* Just like right-click commands it would probably be faster to not bother changing
	Selected and just look up each ID in PS and call function on that actor but I don't
	want to rewrite that function so for now update Selected */

	/* Construct button */
	const FContextButton Button = FContextButton(Command);

	if (!Command.bSameGroup)
	{
		Selected.Reset();

		for (const auto & SelectableID : Command.AffectedSelectables)
		{
			AActor * Selectable = PS->GetSelectableFromID(SelectableID);
			Selected.Emplace(Selectable);
		}
	}

	IssueInstantContextCommand(Button, false);
}

void ARTSPlayerController::Server_IssueInstantUseInventoryItemCommand_Implementation(uint8 SelectableID,
	uint8 InventorySlotIndex, EInventoryItem ItemType)
{	
	AActor * Selectable = PS->GetSelectableFromID(SelectableID);
	
	/* Because I couldn't get #pragma going to temporarily suppress warnings some of these 
	variables are  default initialized but do not have to. Same deal with the other 2 similar 
	functions below */
	ISelectable * AsSelectable = nullptr;
	const FInventory * Inventory;
	const FInventorySlotState * InvSlot;
	const FInventoryItemInfo * ItemsInfo = nullptr;
	const FContextButtonInfo * AbilityInfo = nullptr;

	bool bCanUseItem = false;
	EGameWarning Warning = EGameWarning::None;
	EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;

	// Do all the checks
	if (Statics::IsValid(Selectable))
	{
		if (!Statics::HasZeroHealth(Selectable))
		{
			AsSelectable = CastChecked<ISelectable>(Selectable);
			Inventory = AsSelectable->GetInventory();
			
			/* If inventory capacities can change throughout the match then it is ok for this to fail.
			Otherwise it is 100% proof of a modified client if it fails */
			if (Inventory->IsSlotIndexValid(InventorySlotIndex))
			{
				InvSlot = &Inventory->GetSlotGivenServerIndex(InventorySlotIndex);

				if (InvSlot->HasItem())
				{
					/* If this check fails we could search the inventory attempting to find an 
					item with ItemType, but we won't because performance */
					if (InvSlot->GetItemType() == ItemType)
					{
						if (InvSlot->IsItemUsable())
						{
							if (InvSlot->GetNumCharges() != 0)
							{
								if (!InvSlot->IsOnCooldown(GetWorldTimerManager()))
								{
									ItemsInfo = GI->GetInventoryItemInfo(InvSlot->GetItemType());
									AbilityInfo = ItemsInfo->GetUseAbilityInfo();

									if (AsSelectable->HasEnoughSelectableResources(*AbilityInfo))
									{
										AAbilityBase * EffectActor = AbilityInfo->GetEffectActor(GS);
										if (EffectActor->IsUsable_SelfChecks(AsSelectable, CustomWarning))
										{
											bCanUseItem = true;
										}
										// No else needed here - CustomWarning is an out param for IsUsable_SelfChecks
									}
									else
									{
										Warning = EGameWarning::NotEnoughSelectableResources;
									}
								}
								else
								{
									Warning = EGameWarning::ItemOnCooldown;
								}
							}
							else
							{
								Warning = EGameWarning::ItemOutOfCharges;
							}
						}
						else
						{
							Warning = EGameWarning::ItemNotUsable;
						}
					}
					else
					{
						Warning = EGameWarning::ItemNoLongerInInventory;
					}
				}
				else
				{
					Warning = EGameWarning::ItemNoLongerInInventory;
				}
			}
			else
			{
				// This warning will do
				Warning = EGameWarning::ItemNoLongerInInventory;
			}
		}
		else
		{
			Warning = EGameWarning::UserNoLongerAlive;
		}
	}
	else
	{
		// This warning will do 
		Warning = EGameWarning::UserNoLongerAlive;
	}

	//----------------------------------------------------------------------
	//	Final stages - issue command or do warning
	//----------------------------------------------------------------------

	if (bCanUseItem)
	{
		AsSelectable->IssueCommand_UseInventoryItem(InventorySlotIndex, ItemType, *AbilityInfo);
	}
	else
	{
		if (Warning != EGameWarning::None)
		{
			PS->OnGameWarningHappened(Warning);
		}
		else // Assumed custom warning has something
		{
			PS->OnGameWarningHappened(CustomWarning);
		}
	}
}

void ARTSPlayerController::Server_IssueLocationTargetingUseInventoryItemCommand_Implementation(uint8 SelectableID,
	uint8 InventorySlotIndex, EInventoryItem ItemType, const FVector_NetQuantize & Location)
{
	AActor * Selectable = PS->GetSelectableFromID(SelectableID);

	ISelectable * AsSelectable = nullptr;
	const FInventory * Inventory;
	const FInventorySlotState * InvSlot;
	const FInventoryItemInfo * ItemsInfo = nullptr;
	const FContextButtonInfo * AbilityInfo = nullptr;

	bool bCanUseItem = false;
	EGameWarning Warning = EGameWarning::None;
	EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;

	// Do all the checks
	if (Statics::IsValid(Selectable))
	{
		if (!Statics::HasZeroHealth(Selectable))
		{
			AsSelectable = CastChecked<ISelectable>(Selectable);
			Inventory = AsSelectable->GetInventory();

			/* If inventory capacities can change throughout the match then it is ok for this to fail.
			Otherwise it is 100% proof of a modified client if it fails */
			if (Inventory->IsSlotIndexValid(InventorySlotIndex))
			{
				InvSlot = &Inventory->GetSlotGivenServerIndex(InventorySlotIndex);

				if (InvSlot->HasItem())
				{
					/* If this check fails we could search the inventory attempting to find an
					item with ItemType, but we won't because performance */
					if (InvSlot->GetItemType() == ItemType)
					{
						if (InvSlot->IsItemUsable())
						{
							if (InvSlot->GetNumCharges() != 0)
							{
								if (!InvSlot->IsOnCooldown(GetWorldTimerManager()))
								{
									ItemsInfo = GI->GetInventoryItemInfo(InvSlot->GetItemType());
									AbilityInfo = ItemsInfo->GetUseAbilityInfo();

									const bool bCheckRange = AbilityInfo->ShouldCheckRangeAtCommandIssueTime();
									const bool bRangeOk = (!bCheckRange) || (bCheckRange && Statics::IsSelectableInRangeForAbility(*AbilityInfo, AsSelectable, Location));
									if (bRangeOk)
									{
										const bool bCheckIfLocationInFog = !AbilityInfo->CanLocationBeInsideFog();
										const bool bLocationOk = (!bCheckIfLocationInFog) || (bCheckIfLocationInFog && FogOfWarManager->IsLocationVisible(Location, PS->GetTeam()));
										if (bLocationOk)
										{
											if (AsSelectable->HasEnoughSelectableResources(*AbilityInfo))
											{
												AAbilityBase * EffectActor = AbilityInfo->GetEffectActor(GS);
												if (EffectActor->IsUsable_SelfChecks(AsSelectable, CustomWarning))
												{
													bCanUseItem = true;
												}
												// No else needed here - CustomWarning is an out param for IsUsable_SelfChecks
											}
											else
											{
												Warning = EGameWarning::NotEnoughSelectableResources;
											}
										}
										else
										{
											Warning = EGameWarning::AbilityLocationInsideFog;
										}
									}
									else
									{
										Warning = EGameWarning::AbilityOutOfRange;
									}
								}
								else
								{
									Warning = EGameWarning::ItemOnCooldown;
								}
							}
							else
							{
								Warning = EGameWarning::ItemOutOfCharges;
							}
						}
						else
						{
							Warning = EGameWarning::ItemNotUsable;
						}
					}
					else
					{
						Warning = EGameWarning::ItemNoLongerInInventory;
					}
				}
				else
				{
					Warning = EGameWarning::ItemNoLongerInInventory;
				}
			}
			else
			{
				// This warning will do
				Warning = EGameWarning::ItemNoLongerInInventory;
			}
		}
		else
		{
			Warning = EGameWarning::UserNoLongerAlive;
		}
	}
	else
	{
		// This warning will do 
		Warning = EGameWarning::UserNoLongerAlive;
	}

	//----------------------------------------------------------------------
	//	Final stages - issue command or do warning
	//----------------------------------------------------------------------

	if (bCanUseItem)
	{
		AsSelectable->IssueCommand_UseInventoryItem(InventorySlotIndex, ItemType, *AbilityInfo,
			Location);
	}
	else
	{
		if (Warning != EGameWarning::None)
		{
			PS->OnGameWarningHappened(Warning);
		}
		else // Assumed custom warning has something
		{
			PS->OnGameWarningHappened(CustomWarning);
		}
	}
}

void ARTSPlayerController::Server_IssueSelectableTargetingUseInventoryItemCommand_Implementation(uint8 SelectableID,
	uint8 InventorySlotIndex, EInventoryItem ItemType, AActor * TargetSelectable)
{
	AActor * ItemUser = PS->GetSelectableFromID(SelectableID);
	AActor * ItemTarget = TargetSelectable;

	ISelectable * UserAsSelectable = nullptr;
	ISelectable * TargetAsSelectable = nullptr;
	const FInventory * Inventory;
	const FInventorySlotState * InvSlot;
	const FInventoryItemInfo * ItemsInfo = nullptr;
	const FContextButtonInfo * AbilityInfo = nullptr;

	bool bCanUseItem = false;
	EGameWarning Warning = EGameWarning::None;
	EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;

	// Do all the checks
	if (Statics::IsValid(ItemUser))
	{
		if (!Statics::HasZeroHealth(ItemUser))
		{
			if (Statics::IsValid(ItemTarget))
			{
				if (!Statics::HasZeroHealth(ItemTarget))
				{
					UserAsSelectable = CastChecked<ISelectable>(ItemUser);
					Inventory = UserAsSelectable->GetInventory();

					if (Inventory->IsSlotIndexValid(InventorySlotIndex))
					{
						InvSlot = &Inventory->GetSlotGivenServerIndex(InventorySlotIndex);
						
						if (InvSlot->HasItem())
						{
							if (InvSlot->GetItemType() == ItemType)
							{
								if (InvSlot->IsItemUsable())
								{
									if (InvSlot->GetNumCharges() != 0)
									{
										if (!InvSlot->IsOnCooldown(GetWorldTimerManager()))
										{
											ItemsInfo = GI->GetInventoryItemInfo(InvSlot->GetItemType());
											AbilityInfo = ItemsInfo->GetUseAbilityInfo();
											
											const bool bCheckForSelf = AbilityInfo->CanTargetSelf() && AbilityInfo->CanTargetFriendlies();
											// Pretty sure if this fails it's evidence of a modified client
											if (!bCheckForSelf || ItemUser != ItemTarget)
											{
												const bool bIsTargetFriendly = Statics::IsFriendly(ItemTarget, PS->GetTeamTag());
												if ((AbilityInfo->CanTargetFriendlies() && bIsTargetFriendly) || (!AbilityInfo->CanTargetEnemies() && !bIsTargetFriendly))
												{
													if (Statics::CanTypeBeTargeted(ItemTarget, AbilityInfo->GetAcceptableTargetFNames()))
													{
														if (UserAsSelectable->HasEnoughSelectableResources(*AbilityInfo))
														{
															/* Check for if target is in fog could go here maybe. 
															Might need some lenience to it though and that would 
															probably be hard to implement */

															TargetAsSelectable = CastChecked<ISelectable>(ItemTarget);
															const bool bCheckRange = AbilityInfo->ShouldCheckRangeAtCommandIssueTime();
															const bool bRangeOk = !bCheckRange || Statics::IsSelectableInRangeForAbility(*AbilityInfo, UserAsSelectable, TargetAsSelectable);
															if (bRangeOk)
															{
																AAbilityBase * EffectActor = AbilityInfo->GetEffectActor(GS);
																if (EffectActor->IsUsable_SelfChecks(UserAsSelectable, CustomWarning))
																{
																	if (EffectActor->IsUsable_TargetChecks(UserAsSelectable, TargetAsSelectable, CustomWarning))
																	{
																		bCanUseItem = true;
																	}
																	// No else needed here
																}
																// No else needed here
															}
															else
															{
																Warning = EGameWarning::AbilityOutOfRange;
															}
														}
														else
														{
															Warning = EGameWarning::NotEnoughSelectableResources;
														}
													}
													else
													{
														Warning = EGameWarning::InvalidTarget;
													}
												}
												else
												{
													Warning = EGameWarning::InvalidTarget;
												}
											}
											else
											{
												Warning = EGameWarning::AbilityCannotTargetSelf;
											}
										}
										else
										{
											Warning = EGameWarning::ItemOnCooldown;
										}
									}
									else
									{
										Warning = EGameWarning::ItemOutOfCharges;
									}
								}
								else
								{
									Warning = EGameWarning::ItemNotUsable;
								}
							}
							else
							{
								Warning = EGameWarning::ItemNoLongerInInventory;
							}
						}
						else
						{
							Warning = EGameWarning::ItemNoLongerInInventory;
						}
					}
					else
					{
						Warning = EGameWarning::ItemNoLongerInInventory;
					}
				}
				else
				{
					Warning = EGameWarning::TargetNoLongerAlive;
				}
			}
			else
			{
				Warning = EGameWarning::TargetNoLongerAlive;
			}
		}
		else
		{
			Warning = EGameWarning::UserNoLongerAlive;
		}
	}
	else
	{
		Warning = EGameWarning::UserNoLongerAlive;
	}

	//----------------------------------------------------------------------
	//	Final stages - issue command or do warning
	//----------------------------------------------------------------------

	if (bCanUseItem)
	{
		UserAsSelectable->IssueCommand_UseInventoryItem(InventorySlotIndex, ItemType, *AbilityInfo,
			TargetAsSelectable);
	}
	else
	{
		if (Warning != EGameWarning::None)
		{
			PS->OnGameWarningHappened(Warning);
		}
		else // Assumed custom warning has something
		{
			PS->OnGameWarningHappened(CustomWarning);
		}
	}
}

void ARTSPlayerController::IssueTargetRequiredContextCommandChecked(const FContextButtonInfo & AbilityInfo,
	const FVector & ClickLoc, AActor * ClickTarget, const TArray< ISelectable * >& ToIssueTo)
{
#if !UE_BUILD_SHIPPING

	assert(GetWorld()->IsServer());
	assert(GetWorld()->GetFirstPlayerController() == this); // Check as only server player should call this
	assert(Statics::IsValid(ClickTarget));
	
	AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);
	const FContextButton Button = FContextButton(AbilityInfo.GetButtonType());

#endif

	ISelectable * TargetAsSelectable = CastChecked<ISelectable>(ClickTarget);
	const FSelectableAttributesBase & TargetInfo = TargetAsSelectable->GetAttributesBase();

	for (const auto & Elem : ToIssueTo)
	{
#if !UE_BUILD_SHIPPING
		
		/* Make sure we are allowed to use this ability */
		
		assert(Elem->GetCooldownRemaining(Button) != -2.f && Elem->GetCooldownRemaining(Button) <= 0.f);
		assert(Elem->HasEnoughSelectableResources(AbilityInfo));
		assert(EffectActor->IsUsable_SelfChecks(Elem));
		assert(EffectActor->IsUsable_TargetChecks(Elem, TargetAsSelectable));
		assert(AbilityInfo.PassesCommandTimeRangeCheck(Elem, TargetAsSelectable));

		if (!AbilityInfo.CanTargetSelf())
		{
			assert(Elem != TargetAsSelectable);
		}

		/* Could check more stuff like whether target's affiliation is ok */

#endif

		Elem->OnContextCommand(AbilityInfo, ClickLoc, TargetAsSelectable, TargetInfo);
	}
}

void ARTSPlayerController::IssueLocationRequiredContextCommandChecked(const FContextButtonInfo & AbilityInfo, 
	const FVector & Location, const TArray<ISelectable*>& ToIssueTo)
{
#if !UE_BUILD_SHIPPING

	assert(GetWorld()->IsServer());
	assert(GetWorld()->GetFirstPlayerController() == this);

	if (AbilityInfo.CanLocationBeInsideFog() == false)
	{
		assert(Statics::IsLocationOutsideFogLocally(Location, FogOfWarManager));
	}

	AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);
	const FContextButton Button = FContextButton(AbilityInfo.GetButtonType());

#endif

	for (const auto & Elem : ToIssueTo)
	{
#if !UE_BUILD_SHIPPING

		/* Make sure we are allowed to use this ability */
		assert(Elem->GetCooldownRemaining(Button) != -2.f && Elem->GetCooldownRemaining(Button) <= 0.f);
		assert(Elem->HasEnoughSelectableResources(AbilityInfo));
		assert(EffectActor->IsUsable_SelfChecks(Elem));

		if (AbilityInfo.ShouldCheckRangeAtCommandIssueTime())
		{
			assert(Statics::IsSelectableInRangeForAbility(AbilityInfo, Elem, Location));
		}

		/* Could check more stuff like whether target's affiliation is ok */

#endif 

		Elem->OnContextCommand(AbilityInfo, Location);
	}
}

void ARTSPlayerController::IssueInstantUseInventoryItemCommandChecked(FInventorySlotState & InventorySlot, 
	uint8 InventorySlotIndex, const FInventoryItemInfo & ItemsInfo, const FContextButtonInfo & AbilityInfo)
{
	SERVER_CHECK;

	/* Written assuming the command only goes to the primary selected */

	assert(IsSelectionControlledByThis() == true);
	assert(InventorySlot.HasItem() == true);
	assert(InventorySlot.IsItemUsable() == true);
	assert(InventorySlot.GetNumCharges() != 0);
	assert(InventorySlot.IsOnCooldown(GetWorldTimerManager()) == false);
	assert(CurrentSelected->HasEnoughSelectableResources(AbilityInfo) == true);
	assert(AbilityInfo.GetEffectActor(GS)->IsUsable_SelfChecks(ToSelectablePtr(CurrentSelected)) == true);
	assert(AbilityInfo.IsInstant() == true);

	CurrentSelected->IssueCommand_UseInventoryItem(InventorySlotIndex, InventorySlot.GetItemType(), AbilityInfo);
}

void ARTSPlayerController::IssueUseInventoryItemCommandChecked(FInventorySlotState & InventorySlot,
	uint8 InventorySlotIndex, const FInventoryItemInfo & ItemsInfo, const FContextButtonInfo & AbilityInfo, 
	const FVector & Location)
{
	SERVER_CHECK;

	/* Written assuming the command only goes to the primary selected */

	assert(IsSelectionControlledByThis() == true);
	assert(InventorySlot.HasItem() == true);
	assert(InventorySlot.IsItemUsable() == true);
	assert(InventorySlot.GetNumCharges() != 0);
	assert(InventorySlot.IsOnCooldown(GetWorldTimerManager()) == false);
	assert(CurrentSelected->HasEnoughSelectableResources(AbilityInfo) == true);
	assert(AbilityInfo.GetEffectActor(GS)->IsUsable_SelfChecks(ToSelectablePtr(CurrentSelected)) == true);
	assert(AbilityInfo.CanLocationBeInsideFog() || Statics::IsLocationOutsideFogLocally(Location, FogOfWarManager));
	assert(AbilityInfo.PassesCommandTimeRangeCheck(ToSelectablePtr(CurrentSelected), Location));
	// Probably more checks that can go here

	CurrentSelected->IssueCommand_UseInventoryItem(InventorySlotIndex, InventorySlot.GetItemType(), AbilityInfo, Location);
}

void ARTSPlayerController::IssueUseInventoryItemCommandChecked(FInventorySlotState & InventorySlot,
	uint8 InventorySlotIndex, const FInventoryItemInfo & ItemsInfo, const FContextButtonInfo & AbilityInfo,
	ISelectable * Target)
{
	SERVER_CHECK;

	/* Written assuming the command only goes to the primary selected */

	assert(IsSelectionControlledByThis() == true);
	assert(InventorySlot.HasItem() == true);
	assert(InventorySlot.IsItemUsable() == true);
	assert(InventorySlot.GetNumCharges() != 0);
	assert(InventorySlot.IsOnCooldown(GetWorldTimerManager()) == false);
	assert(CurrentSelected->HasEnoughSelectableResources(AbilityInfo) == true);
	assert(AbilityInfo.GetEffectActor(GS)->IsUsable_SelfChecks(ToSelectablePtr(CurrentSelected)) == true);
	assert(AbilityInfo.GetEffectActor(GS)->IsUsable_TargetChecks(ToSelectablePtr(CurrentSelected), Target) == true);
	assert(AbilityInfo.PassesCommandTimeRangeCheck(ToSelectablePtr(CurrentSelected), Target));
	// Probably more checks that can go here

	CurrentSelected->IssueCommand_UseInventoryItem(InventorySlotIndex, InventorySlot.GetItemType(), 
		AbilityInfo, Target);
}

bool ARTSPlayerController::IssueContextCommand(const FContextButtonInfo & AbilityInfo, 
	const FVector & ClickLoc, AActor * Target)
{
	assert(GetWorld()->IsServer());
	// Server player should not call this, or it would likely be inefficient to do so
	assert(GetWorld()->GetFirstPlayerController() != this);

	if (!IsSelectionControlledByThis())
	{
		return false;
	}

	// Can happen because networking
	if (Statics::IsValid(Target) == false)
	{
		return false;
	}

	const EContextButton Command = AbilityInfo.GetButtonType();
	const FContextButton Button = FContextButton(Command);
	const bool bCheckIfTargetIsSelf = !AbilityInfo.CanTargetSelf() && AbilityInfo.CanTargetFriendlies();
	const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();
	AAbilityBase * EffectInfoActor = AbilityInfo.GetEffectActor(GS);
	ISelectable * TargetAsSelectable = CastChecked<ISelectable>(Target);
	const FSelectableAttributesBase & TargetInfo = TargetAsSelectable->GetAttributesBase();

	if (AbilityInfo.IsIssuedToAllSelected())
	{
		/* Num of selected selectables that have this button on their context menu. Start at max
		and decrement as we go along.  */
		int32 NumSupported = Selected.Num();
		/* Number that have ability on cooldown */
		int32 NumOnCooldown = 0;
		int32 NumWithoutEnoughSelectableResources = 0;
		int32 NumOutOfRange = 0;
		/* Number that failed the ability's custom checks */
		int32 NumFailedCustomChecks = 0;
		EAbilityRequirement CustomChecksFailReason = EAbilityRequirement::Uninitialized;
		int32 NumCannotTargetBecauseSelf = 0;
		int32 NumThatCanUse = 0;

		for (const auto & Elem : Selected)
		{
			/* Valid check because networking */
			if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
			{
				continue;
			}

			ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

			/* -2.f implies does not support button */
			const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

			if (CooldownRemaining == -2.f)
			{
				NumSupported--;
			}
			else
			{
				if (CooldownRemaining > 0.f)
				{
					NumOnCooldown++;
				}
				else
				{
					if (bCheckIfTargetIsSelf)
					{
						if (AsSelectable == TargetAsSelectable)
						{
							NumCannotTargetBecauseSelf++;
							assert(NumCannotTargetBecauseSelf == 1); // Sanity check

							continue;
						}
					}
					
					if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
					{
						NumWithoutEnoughSelectableResources++;

						continue;
					}

					if (bCheckRange)
					{
						if (Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, TargetAsSelectable) == false)
						{
							NumOutOfRange++;
							
							continue;
						}
					}

					if (EffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false 
						|| EffectInfoActor->IsUsable_TargetChecks(AsSelectable, TargetAsSelectable, CustomChecksFailReason) == false)
					{
						NumFailedCustomChecks++;
					}
					else
					{
						// Actually issue command
						AsSelectable->OnContextCommand(AbilityInfo, ClickLoc, TargetAsSelectable, TargetInfo);

						NumThatCanUse++;
					}
				}
			}
		}

		// Sanity checks
		assert(NumThatCanUse <= NumSupported && NumOnCooldown <= NumSupported
			&& NumOutOfRange <= NumSupported && NumFailedCustomChecks <= NumSupported);

		/* Show warning message if required */
		if (NumThatCanUse == 0)
		{
			/* As long as at least one unit has it on cooldown we just say the error is because
			cooldown */
			if (NumSupported == 0)
			{
				PS->OnGameWarningHappened(EGameWarning::UserNoLongerValid);
			}
			else if (NumOnCooldown > 0)
			{
				PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
			}
			else if (NumWithoutEnoughSelectableResources > 0)
			{
				PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
			}
			else if (NumFailedCustomChecks > 0)
			{
				PS->OnGameWarningHappened(CustomChecksFailReason);
			}
			else if (NumCannotTargetBecauseSelf > 0)
			{
				PS->OnGameWarningHappened(EGameWarning::AbilityCannotTargetSelf);
			}
			else
			{
				assert(NumOutOfRange > 0);
				
				PS->OnGameWarningHappened(EGameWarning::AbilityOutOfRange);
			}

			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		/* Num of selected selectables that have this button on their context menu. Start at max
		and decrement as we go along.  */
		int32 NumSupported = Selected.Num();
		/* Number that have ability on cooldown */
		int32 NumOnCooldown = 0;
		int32 NumWithoutEnoughSelectableResources = 0;
		int32 NumOutOfRange = 0;
		/* Number that failed the ability's custom checks */
		int32 NumFailedCustomChecks = 0;
		EAbilityRequirement CustomChecksFailReason = EAbilityRequirement::Uninitialized;
		int32 NumCannotTargetBecauseSelf = 0;

		if (CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::First)
		{
			for (const auto & Elem : Selected)
			{
				/* Valid check because networking */
				if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
				{
					continue;
				}

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				/* -2.f implies does not support button */
				const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

				if (CooldownRemaining == -2.f)
				{
					NumSupported--;
				}
				else
				{
					if (CooldownRemaining > 0.f)
					{
						NumOnCooldown++;
					}
					else
					{
						if (bCheckIfTargetIsSelf)
						{
							if (AsSelectable == TargetAsSelectable)
							{
								NumCannotTargetBecauseSelf++;
								assert(NumCannotTargetBecauseSelf == 1); // Sanity check

								continue;
							}
						}
						
						if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
						{
							NumWithoutEnoughSelectableResources++;

							continue;
						}

						if (bCheckRange)
						{
							if (Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, TargetAsSelectable) == false)
							{
								NumOutOfRange++;

								continue;
							}
						}

						if (EffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false
							|| EffectInfoActor->IsUsable_TargetChecks(AsSelectable, TargetAsSelectable, CustomChecksFailReason) == false)
						{
							NumFailedCustomChecks++;
						}
						else
						{
							// Actually issue command
							AsSelectable->OnContextCommand(AbilityInfo, ClickLoc, TargetAsSelectable, TargetInfo);

							/* Only needed to issue to one so stop here */
							return true;
						}
					}
				}
			}
		}
		else // Assumed Closest rule
		{
			assert(CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::Closest);

			ISelectable * BestSelectable = nullptr;
			float BestDistanceSqr = FLT_MAX;

			for (const auto & Elem : Selected)
			{
				/* Valid check because networking */
				if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
				{
					continue;
				}

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				/* -2.f implies does not support button */
				const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

				if (CooldownRemaining == -2.f)
				{
					NumSupported--;
				}
				else
				{
					if (CooldownRemaining > 0.f)
					{
						NumOnCooldown++;
					}
					else
					{
						if (bCheckIfTargetIsSelf)
						{
							if (AsSelectable == TargetAsSelectable)
							{
								NumCannotTargetBecauseSelf++;
								assert(NumCannotTargetBecauseSelf == 1); // Sanity check

								continue;
							}
						}
						
						if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
						{
							NumWithoutEnoughSelectableResources++;

							continue;
						}

						if (EffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false
							|| EffectInfoActor->IsUsable_TargetChecks(AsSelectable, TargetAsSelectable, CustomChecksFailReason) == false)
						{
							NumFailedCustomChecks++;
						}
						else
						{
							/* Get range from target */
							
							const float RangeFromTargetSqr = Statics::GetSelectablesDistanceForAbilitySquared(AbilityInfo, AsSelectable, TargetAsSelectable);

							if (RangeFromTargetSqr < BestDistanceSqr)
							{
								/* Don't actually care whether in range or not. At the end we 
								just check if the BestDistance is in range and if so then we 
								issue command */
								BestDistanceSqr = RangeFromTargetSqr;
								BestSelectable = AsSelectable;
							}
						}
					}
				}
			}

			if (bCheckRange)
			{
				if (BestDistanceSqr <= Statics::GetAbilityRangeSquared(AbilityInfo))
				{
					// Actually issue command
					BestSelectable->OnContextCommand(AbilityInfo, ClickLoc, TargetAsSelectable, TargetInfo);

					return true;
				}
				else
				{
					/* This variable isn't strictly correct. We've now made it equal 1 but it could
					be much more. It is only used for HUD warning message purposes and doing it this
					way is still fine */
					NumOutOfRange++;
				}
			}
			else
			{
				// Actually issue command
				BestSelectable->OnContextCommand(AbilityInfo, ClickLoc, TargetAsSelectable, TargetInfo);

				return true;
			}
		}

		// Sanity checks
		assert(NumOnCooldown <= NumSupported && NumOutOfRange <= NumSupported 
			&& NumFailedCustomChecks <= NumSupported);

		/* If here then we did not issue a command */
		
		if (NumSupported == 0)
		{
			PS->OnGameWarningHappened(EGameWarning::UserNoLongerValid);
		}
		else if (NumOutOfRange > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::AbilityOutOfRange);
		}
		else if (NumWithoutEnoughSelectableResources > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
		}
		else if (NumOnCooldown > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
		}
		else if (NumCannotTargetBecauseSelf > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::AbilityCannotTargetSelf);
		}
		else
		{
			assert(NumFailedCustomChecks > 0);

			PS->OnGameWarningHappened(CustomChecksFailReason);
		}

		return false;
	}
}

bool ARTSPlayerController::IssueContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc)
{
	assert(GetWorld()->IsServer());
	// Server player should not call this, or it would likely be inefficient to do so
	assert(GetWorld()->GetFirstPlayerController() != this);

	if (!IsSelectionControlledByThis())
	{
		/* Actually if this happens then it is worrying. This check is a good candidate to be in 
		the _Validate part of the function */
		return false;
	}

	/* Check this fast path first */
	if (AbilityInfo.CanLocationBeInsideFog() == false)
	{
		if (FogOfWarManager->IsLocationVisible(ClickLoc, PS->GetTeam()) == false)
		{
			PS->OnGameWarningHappened(EGameWarning::AbilityLocationInsideFog);

			return false;
		}
	}

	const EContextButton Command = AbilityInfo.GetButtonType();
	const FContextButton Button = FContextButton(Command);
	const bool bCheckRange = AbilityInfo.ShouldCheckRangeAtCommandIssueTime();
	AAbilityBase * EffectInfoActor = AbilityInfo.GetEffectActor(GS);

	if (AbilityInfo.IsIssuedToAllSelected())
	{
		/* Num of selected selectables that have this button on their context menu. Start at max
		and decrement as we go along.  */
		int32 NumSupported = Selected.Num();
		int32 NumWithoutEnoughSelectableResources = 0;
		/* Number that have ability on cooldown */
		int32 NumOnCooldown = 0;
		int32 NumOutOfRange = 0;
		/* Number that failed the ability's custom checks */
		int32 NumFailedCustomChecks = 0;
		EAbilityRequirement CustomChecksFailReason = EAbilityRequirement::Uninitialized;
		int32 NumThatCanUse = 0;

		for (const auto & Elem : Selected)
		{
			/* Valid check because networking */
			if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
			{
				continue;
			}

			ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

			/* -2.f implies does not support button */
			const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

			if (CooldownRemaining == -2.f)
			{
				NumSupported--;
			}
			else
			{
				if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
				{
					NumWithoutEnoughSelectableResources++;

					continue;
				}

				if (bCheckRange)
				{
					if (Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, ClickLoc) == false)
					{
						NumOutOfRange++;
							
						continue;
					}
				}
					
				if (EffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false)
				{
					NumFailedCustomChecks++;
				}
				else
				{
					// Actually issue command
					AsSelectable->OnContextCommand(AbilityInfo, ClickLoc);

					NumThatCanUse++;
				}
			}
		}

		// Sanity checks
		assert(NumThatCanUse <= NumSupported && NumOnCooldown <= NumSupported
			&& NumOutOfRange <= NumSupported && NumFailedCustomChecks <= NumSupported);

		/* Show warning message if required */
		if (NumThatCanUse == 0)
		{
			/* As long as at least one unit has it on cooldown we just say the error is because
			cooldown */
			if (NumOnCooldown > 0)
			{
				PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
			}
			else if (NumWithoutEnoughSelectableResources > 0)
			{
				PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
			}
			else if (NumFailedCustomChecks > 0)
			{
				PS->OnGameWarningHappened(CustomChecksFailReason);
			}
			else
			{
				assert(NumOutOfRange > 0);

				PS->OnGameWarningHappened(EGameWarning::AbilityOutOfRange);
			}

			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		/* Num of selected selectables that have this button on their context menu. Start at max
		and decrement as we go along.  */
		int32 NumSupported = Selected.Num();
		/* Number that have ability on cooldown */
		int32 NumOnCooldown = 0;
		int32 NumWithoutEnoughSelectableResources = 0;
		int32 NumOutOfRange = 0;
		/* Number that failed the ability's custom checks */
		int32 NumFailedCustomChecks = 0;
		EAbilityRequirement CustomChecksFailReason = EAbilityRequirement::Uninitialized;

		if (CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::First)
		{
			for (const auto & Elem : Selected)
			{
				/* Valid check because networking */
				if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
				{
					continue;
				}

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				/* -2.f implies does not support button */
				const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

				if (CooldownRemaining == -2.f)
				{
					NumSupported--;
				}
				else
				{
					if (CooldownRemaining > 0.f)
					{
						NumOnCooldown++;
					}
					else
					{
						if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
						{
							NumWithoutEnoughSelectableResources++;

							continue;
						}

						if (bCheckRange)
						{
							if (Statics::IsSelectableInRangeForAbility(AbilityInfo, AsSelectable, ClickLoc) == false)
							{
								NumOutOfRange++;

								continue;
							}
						}

						if (EffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false)
						{
							NumFailedCustomChecks++;
						}
						else
						{
							// Actually issue command
							AsSelectable->OnContextCommand(AbilityInfo, ClickLoc);

							/* Only needed to issue to one so stop here */
							return true;
						}
					}
				}
			}
		}
		else // Assumed Closest
		{
			assert(CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::Closest);
			
			ISelectable * BestSelectable = nullptr;
			float BestDistanceSqr = FLT_MAX;

			for (const auto & Elem : Selected)
			{
				/* Valid check because networking */
				if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
				{
					continue;
				}

				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				/* -2.f implies does not support button */
				const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

				if (CooldownRemaining == -2.f)
				{
					NumSupported--;
				}
				else
				{
					if (CooldownRemaining > 0.f)
					{
						NumOnCooldown++;
					}
					else
					{
						if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
						{
							NumWithoutEnoughSelectableResources++;

							continue;
						}
						
						if (EffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false)
						{
							NumFailedCustomChecks++;
						}
						else
						{
							/* Get range from target */

							const float RangeFromTargetSqr = Statics::GetSelectablesDistanceForAbilitySquared(AbilityInfo, AsSelectable, ClickLoc);

							if (RangeFromTargetSqr < BestDistanceSqr)
							{
								/* Don't actually care whether in range or not. At the end we
								just check if the BestDistance is in range and if so then we
								issue command */
								BestDistanceSqr = RangeFromTargetSqr;
								BestSelectable = AsSelectable;
							}
						}
					}
				}
			}

			if (bCheckRange)
			{
				if (BestDistanceSqr <= Statics::GetAbilityRangeSquared(AbilityInfo))
				{
					// Actually issue command
					BestSelectable->OnContextCommand(AbilityInfo, ClickLoc);

					return true;
				}
				else
				{
					/* This variable isn't strictly correct. We've now made it equal 1 but it could
					be much more. It is only used for HUD warning message purposes and doing it this
					way is still fine */
					NumOutOfRange++;
				}
			}
			else
			{
				// Actually issue command
				BestSelectable->OnContextCommand(AbilityInfo, ClickLoc);

				return true;
			}
		}

		// Sanity checks
		assert(NumOnCooldown <= NumSupported && NumOutOfRange <= NumSupported
			&& NumFailedCustomChecks <= NumSupported);

		/* If here then we did not issue a command */

		if (NumSupported == 0)
		{
			PS->OnGameWarningHappened(EGameWarning::UserNoLongerValid);
		}
		else if (NumOutOfRange > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::AbilityOutOfRange);
		}
		else if (NumOnCooldown > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
		}
		else if (NumWithoutEnoughSelectableResources > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
		}
		else
		{
			assert(NumFailedCustomChecks > 0);

			PS->OnGameWarningHappened(CustomChecksFailReason);
		}

		return false;
	}
}

bool ARTSPlayerController::IssueInstantContextCommand(const FContextButton & Button, bool bFromServerPlayer)
{
	SERVER_CHECK;

	if (!IsSelectionControlledByThis())
	{
		return false;
	}

	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(Button.GetButtonType());
	AAbilityBase * AbilityEffectInfoActor = AbilityInfo.GetEffectActor(GS);

	if (AbilityInfo.IsIssuedToAllSelected())
	{
		/* Num of selected selectables that have this button on their context menu. Start at max 
		and decrement as we go along.  */
		int32 NumSupported = Selected.Num();
		/* Number that have ability on cooldown */
		int32 NumOnCooldown = 0;
		int32 NumWithoutEnoughSelectableResources = 0;
		/* Number that failed the ability's custom checks */
		int32 NumFailedCustomChecks = 0;
		EAbilityRequirement CustomChecksFailReason = EAbilityRequirement::Uninitialized;
		int32 NumThatCanUse = 0;

		for (const auto & Elem : Selected)
		{
			/* Valid check because networking */
			if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
			{
				continue;
			}

			ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

			/* -2.f implies does not support button */
			const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

			if (CooldownRemaining == -2.f)
			{
				NumSupported--;
			}
			else
			{
				if (CooldownRemaining > 0.f)
				{
					NumOnCooldown++;
				}
				else
				{
					if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
					{
						NumWithoutEnoughSelectableResources++;

						continue;
					}
					
					if (AbilityEffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false)
					{
						NumFailedCustomChecks++;
					}
					else
					{
						// Actually issue command
						AsSelectable->OnContextCommand(Button);

						NumThatCanUse++;
					}
				}
			}
		}

#if !UE_BUILD_SHIPPING
		if (GetWorld()->GetFirstPlayerController() == this)
		{
			/* If this throws then server player called this but they don't have any selected
			selectables that support it so they should not have been able to click button in
			the first place */
			assert(NumSupported > 0);
		}
#endif

		// Sanity checks
		assert(NumThatCanUse <= NumSupported && NumOnCooldown <= NumSupported 
			&& NumFailedCustomChecks <= NumSupported);

		/* Show warning message if required */
		if (NumThatCanUse == 0)
		{
			/* As long as at least one unit has it on cooldown we just say the error is because 
			cooldown */
			if (NumOnCooldown > 0)
			{
				PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
			}
			else if (NumWithoutEnoughSelectableResources > 0)
			{
				PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
			}
			else
			{
				assert(NumFailedCustomChecks > 0);
				
				PS->OnGameWarningHappened(CustomChecksFailReason);
			}

			return false;
		}
		else
		{
			if (ControlOptions::bInstantActionsCancelPendingOnes && bFromServerPlayer)
			{
				/* Cancel any pending actions. We've delayed this right until we know the ability 
				is usable cause it seems like a good thing to do feel wise */
				if (IsPlacingGhost())
				{
					CancelGhost();
				}
				else if (IsContextActionPending())
				{
					CancelPendingContextCommand();
				}
				else if (IsGlobalSkillsPanelAbilityPending())
				{
					CancelPendingGlobalSkillsPanelAbility();
				}
			}
			
			return true;
		}
	}
	else
	{
		/* Give out command to first selectable that supports it. For warning message reasons 
		we will also have to record how many have it on cooldown and failed custom checks etc */
		
		/* Num of selected selectables that have this button on their context menu. Start at max
		and decrement as we go along.  */
		int32 NumSupported = Selected.Num();
		/* Number that have ability on cooldown */
		int32 NumOnCooldown = 0;
		int32 NumWithoutEnoughSelectableResources = 0;
		/* Number that failed the ability's custom checks */
		int32 NumFailedCustomChecks = 0;
		EAbilityRequirement CustomChecksFailReason = EAbilityRequirement::Uninitialized;

		for (const auto & Elem : Selected)
		{
			/* Valid check because networking */
			if (!Statics::IsValid(Elem) || Statics::HasZeroHealth(Elem))
			{
				continue;
			}

			ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

			/* -2.f implies does not support button */
			const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

			if (CooldownRemaining == -2.f)
			{
				NumSupported--;
			}
			else
			{
				if (CooldownRemaining > 0.f)
				{
					NumOnCooldown++;
				}
				else
				{
					if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
					{
						NumWithoutEnoughSelectableResources++;

						continue;
					}
					
					if (AbilityEffectInfoActor->IsUsable_SelfChecks(AsSelectable, CustomChecksFailReason) == false)
					{
						NumFailedCustomChecks++;
					}
					else
					{
						if (ControlOptions::bInstantActionsCancelPendingOnes && bFromServerPlayer)
						{
							/* Cancel any pending actions. We've delayed this right until we know the ability
							is usable cause it seems like a good thing to do feel wise */
							if (IsPlacingGhost())
							{
								CancelGhost();
							}
							else if (IsContextActionPending())
							{
								CancelPendingContextCommand();
							}
							else if (IsGlobalSkillsPanelAbilityPending())
							{
								CancelPendingGlobalSkillsPanelAbility();
							}
						}
						
						// Actually issue command
						AsSelectable->OnContextCommand(Button);

						/* Not a command type that gets issued to all, so can stop here */
						return true;
					}
				}
			}
		}

#if !UE_BUILD_SHIPPING
		if (GetWorld()->GetFirstPlayerController() == this)
		{
			/* If this throws then server player called this but they don't have any selected
			selectables that support it so they should not have been able to click button in
			the first place */
			assert(NumSupported > 0);
		}
#endif

		/* If here then we did not issue a command */

		if (NumSupported == 0)
		{
			/* This can happen because networking. Although sometimes server player calls this 
			func so really we should assert this for them because the button should not have 
			been clickable in the first place. */

			/* Rare but can happen. Try send client a warning message */
			PS->OnGameWarningHappened(EGameWarning::UserNoLongerValid);
		}
		else if (NumOnCooldown > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
		}
		else if (NumWithoutEnoughSelectableResources > 0)
		{
			PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
		}
		else
		{
			assert(NumFailedCustomChecks > 0);

			PS->OnGameWarningHappened(CustomChecksFailReason);
		}

		return false;
	}

	/* Crashes that show this line possibly indicates that what was selected does not support 
	the action */
}

void ARTSPlayerController::ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, 
	const FCommanderAbilityInfo & AbilityInfo)
{
	SERVER_CHECK;
	
	// Assert ability is usable. Should have checked all this by now 
	assert(IsGlobalSkillsPanelAbilityUsable(AbilityState, true) == EGameWarning::None);

	GS->Server_CreateAbilityEffect(AbilityInfo, PS, Team, FVector::ZeroVector, nullptr);
}

void ARTSPlayerController::ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState,
	const FCommanderAbilityInfo & AbilityInfo, ARTSPlayerState * AbilityTarget)
{
	SERVER_CHECK;
	
	// Assert ability is usable. Should have checked all this by now 
	assert(IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityTarget, true) == EGameWarning::None);

	GS->Server_CreateAbilityEffect(AbilityInfo, PS, Team, FVector::ZeroVector, AbilityTarget);
}

void ARTSPlayerController::ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, 
	const FCommanderAbilityInfo & AbilityInfo, AActor * AbilityTarget, ISelectable * TargetAsSelectable)
{
	SERVER_CHECK;

	// Assert ability is usable. Should have checked all this by now 
	assert(IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityTarget, TargetAsSelectable, true) == EGameWarning::None);

	GS->Server_CreateAbilityEffect(AbilityInfo, PS, Team, AbilityTarget->GetActorLocation(), AbilityTarget);
}

void ARTSPlayerController::ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, 
	const FCommanderAbilityInfo & AbilityInfo, const FVector & AbilityLocation)
{
	SERVER_CHECK;

	// Assert ability is usable. Should have checked all this by now
	assert(IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityLocation, true) == EGameWarning::None);

	GS->Server_CreateAbilityEffect(AbilityInfo, PS, Team, AbilityLocation, nullptr);
}

void ARTSPlayerController::ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, 
	const FCommanderAbilityInfo & AbilityInfo, const FVector & AbilityLocation, AActor * AbilityTarget, 
	ISelectable * TargetAsSelectable)
{
	SERVER_CHECK;

	/* If AbilityTarget is null the ability is targeting a world location. Otherwise it is 
	targeting a selectable */
	if (AbilityTarget != nullptr)
	{
		// Assert ability is usable. Should have checked all this by now
		assert(IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityTarget, TargetAsSelectable, true) == EGameWarning::None);

		GS->Server_CreateAbilityEffect(AbilityInfo, PS, Team, FVector(FLT_MAX), AbilityTarget);
	}
	else
	{
		// Assert ability is usable. Should have checked all this by now
		assert(IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityLocation, true) == EGameWarning::None);

		GS->Server_CreateAbilityEffect(AbilityInfo, PS, Team, AbilityLocation, nullptr);
	}
}

void ARTSPlayerController::Server_RequestExecuteCommanderAbility_Implementation(ECommanderAbility AbilityType)
{
	/* Check if ability is usable. Then execute it if it is */
	
	/* Kind of a long access chain here. 
	ECommanderAbility --> FCommanderAbilityInfo* --> ECommanderSkillTreeNodeType --> FAquiredCommanderAbilityState* */
	const FCommanderAbilityInfo & AbilityInfo = GI->GetCommanderAbilityInfo(AbilityType);
	FAquiredCommanderAbilityState * AbilityState = PS->GetCommanderAbilityState(AbilityInfo);
	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, true);
	if (Warning == EGameWarning::None)
	{
		ExecuteCommanderAbility(AbilityState, AbilityInfo);
	}
	else
	{
		PS->OnGameWarningHappened(Warning);
	}
}

void ARTSPlayerController::Server_RequestExecuteCommanderAbility_PlayerTargeting_Implementation(ECommanderAbility AbilityType, uint8 TargetsPlayerID)
{
	/* Check if ability is usable. Then execute it if it is */

	const FCommanderAbilityInfo & AbilityInfo = GI->GetCommanderAbilityInfo(AbilityType);
	FAquiredCommanderAbilityState * AbilityState = PS->GetCommanderAbilityState(AbilityInfo);
	ARTSPlayerState * AbilityTarget = GS->GetPlayerFromID(TargetsPlayerID);

	if (Statics::IsValid(AbilityTarget))
	{
		const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityTarget, true);
		if (Warning == EGameWarning::None)
		{
			ExecuteCommanderAbility(AbilityState, AbilityInfo, AbilityTarget);
		}
		else
		{
			PS->OnGameWarningHappened(Warning);
		}
	}
	else
	{
		PS->OnGameWarningHappened(EGameWarning::TargetPlayerHasBeenDefeated);
	}

}

void ARTSPlayerController::Server_RequestExecuteCommanderAbility_SelectableTargeting_Implementation(ECommanderAbility AbilityType, 
	AActor * SelectableTarget)
{
	const FCommanderAbilityInfo & AbilityInfo = GI->GetCommanderAbilityInfo(AbilityType);
	FAquiredCommanderAbilityState * AbilityState = PS->GetCommanderAbilityState(AbilityInfo);

	AActor * AbilityTarget = SelectableTarget;

	if (Statics::IsValid(AbilityTarget))
	{
		if (IsASelectable(AbilityTarget))
		{
			ISelectable * TargetAsSelectable = CastChecked<ISelectable>(AbilityTarget);
			const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityTarget, TargetAsSelectable, true);
			if (Warning == EGameWarning::None)
			{
				ExecuteCommanderAbility(AbilityState, AbilityInfo, AbilityTarget, TargetAsSelectable);
			}
			else
			{
				PS->OnGameWarningHappened(Warning);
			}
		}
		else // Evidence of a modified client
		{
			
		}
	}
	else
	{
		PS->OnGameWarningHappened(EGameWarning::TargetNoLongerAlive);
	}
}

void ARTSPlayerController::Server_RequestExecuteCommanderAbility_LocationTargeting_Implementation(ECommanderAbility AbilityType, 
	const FVector_NetQuantize & AbilityLocation)
{
	const FCommanderAbilityInfo & AbilityInfo = GI->GetCommanderAbilityInfo(AbilityType);
	FAquiredCommanderAbilityState * AbilityState = PS->GetCommanderAbilityState(AbilityInfo);

	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityLocation, true);
	if (Warning == EGameWarning::None)
	{
		ExecuteCommanderAbility(AbilityState, AbilityInfo, AbilityLocation);
	}
	else
	{
		PS->OnGameWarningHappened(Warning);
	}
}

void ARTSPlayerController::Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingSelectable_Implementation(ECommanderAbility AbilityType, AActor * SelectableTarget)
{	
	const FCommanderAbilityInfo & AbilityInfo = GI->GetCommanderAbilityInfo(AbilityType);
	FAquiredCommanderAbilityState * AbilityState = PS->GetCommanderAbilityState(AbilityInfo);

	if (Statics::IsValid(SelectableTarget))
	{
		if (IsASelectable(SelectableTarget))
		{
			ISelectable * TargetAsSelectable = CastChecked<ISelectable>(SelectableTarget);
			const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, SelectableTarget, TargetAsSelectable, true);
			if (Warning == EGameWarning::None)
			{
				ExecuteCommanderAbility(AbilityState, AbilityInfo, FVector::ZeroVector, SelectableTarget, TargetAsSelectable);
			}
			else
			{
				PS->OnGameWarningHappened(Warning);
			}
		}
		else // Evidence of a modified client
		{

		}
	}
	else
	{
		PS->OnGameWarningHappened(EGameWarning::TargetNoLongerAlive);
	}
}

void ARTSPlayerController::Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingLocation_Implementation(ECommanderAbility AbilityType, const FVector_NetQuantize & AbilityLocation)
{
	const FCommanderAbilityInfo & AbilityInfo = GI->GetCommanderAbilityInfo(AbilityType);
	FAquiredCommanderAbilityState * AbilityState = PS->GetCommanderAbilityState(AbilityInfo);

	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, AbilityLocation, true);
	if (Warning == EGameWarning::None)
	{
		ExecuteCommanderAbility(AbilityState, AbilityInfo, AbilityLocation, nullptr, nullptr);
	}
	else
	{
		PS->OnGameWarningHappened(Warning);
	}
}

void ARTSPlayerController::CancelPendingContextCommand()
{
	/* Reset mouse cursor to default */
	ResetMouseAppearance();

	/* Only unhighlight button if it's a selectable's context action that's pending NOT a
	commander ability */
	if (PendingContextAction != EContextButton::None)
	{
		UnhighlightHighlightedButton();
	}

	PendingContextAction = EContextButton::None;
}

void ARTSPlayerController::CancelPendingGlobalSkillsPanelAbility()
{
	ResetMouseAppearance();
	HUD->HidePlayerTargetingPanel();
	/* Only unhighlight highlighted button if it's a global skills panel ability that
	is pending NOT a selectable's ability */
	if (PendingCommanderAbility != nullptr)
	{
		UnhighlightHighlightedButton();
	}
	PendingCommanderAbility = nullptr;
}

void ARTSPlayerController::RemoveSelection()
{
	/* Remove all entries. Iterating in the normal direction may
	shrink the array each time RemoveAt is called */
	const int32 Size = Selected.Num();
	for (int32 i = Size - 1; i >= 0; --i)
	{
		AActor * const Elem = Selected[i];

		/* Check because networking */
		if (!Statics::IsValid(Elem))
		{
			Selected.RemoveAt(i, 1, false);
			continue;
		}

		ISelectable * const AsSelectable = CastChecked<ISelectable>(Selected[i]);

		AsSelectable->OnDeselect();
		Selected.RemoveAt(i);
	}

	SelectedIDs.Reset();

	CurrentSelected = nullptr;
}

void ARTSPlayerController::RemoveSelection(const ISelectable * Exclude)
{
	/* If Exclude == nullptr you want to call the function with no params */
	assert(Exclude != nullptr);

	/* Iterating in the normal direction may shrink the
	array each time RemoveAt is called */
	const int32 Size = Selected.Num();
	for (int32 i = Size - 1; i >= 0; --i)
	{
		AActor * const Elem = Selected[i];

		/* Check because networking */
		if (!Statics::IsValid(Elem))
		{
			Selected.RemoveAt(i, 1, false);
			continue;
		}

		ISelectable * const AsSelectable = CastChecked<ISelectable>(Selected[i]);

		if (AsSelectable != Exclude)
		{
			AsSelectable->OnDeselect();
		}

		Selected.RemoveAt(i);
	}

	SelectedIDs.Reset();

	/* If we removed everything make sure this is set to null */
	if (Selected.Num() == 0)
	{
		CurrentSelected = nullptr;
	}
}

bool ARTSPlayerController::HasSelection() const
{
	return Selected.Num() > 0;
}

bool ARTSPlayerController::HasMultipleSelected() const
{
	return Selected.Num() > 1;
}

bool ARTSPlayerController::IsControlledByThis(const FName OtherPlayerID) const
{
	return (OtherPlayerID == this->PlayerID);
}

bool ARTSPlayerController::IsControlledByThis(const AActor * Actor) const
{
	return (Actor->Tags[0] == PS->GetPlayerID());	// TODO: try use this->PlayerID instead
}

bool ARTSPlayerController::IsSelectionControlledByThis() const
{
	if (HasSelection() && Statics::IsValid(Selected[0]))
	{
		return IsControlledByThis(Selected[0]);
	}

	return false;
}

ISelectable * ARTSPlayerController::ToSelectablePtr(const TScriptInterface<ISelectable> & Obj) const
{
	// Hoping this works
	return static_cast<ISelectable*>(Obj.GetInterface());
}

bool ARTSPlayerController::IsNeutral(FName OtherPlayerID)
{
	return (OtherPlayerID == Statics::NeutralID);
}

void ARTSPlayerController::OnRightClickCommand(const FVector & ClickLoc, AActor * Target)
{
	/* TODO: make sure to not update Selected server-side from clients */

	/* These have to be true in order to get this far */
	assert(HasSelection() && IsSelectionControlledByThis());

	/* Save this. Basically we need to pass it into PlayRightClickCommandParticlesAndSound 
	later on. It is called after the command and it is possible that during issuing the 
	command we lose our selection e.g. unit enters a garrison so we need to save it here. */
	TScriptInterface<ISelectable> SavedPrimarySelected = GetCurrentSelected();

	// Issue the order to selection
	if (GetWorld()->IsServer())
	{
		/* Just use the Selected array - don't bother with ID lookups. No need to try
		and save bandwidth since we are the server */
		const EGameWarning Result = IssueRightClickCommand(ClickLoc, Target);
		if (Result != EGameWarning::None)
		{
			/* Stop here and do not do any click particles or anything like that. 
			IssueRightClickCommand should of handled the warning message */
			return;
		}
	}
	else
	{
		//=======================================================================================
		//	Figuring out if we need to send an RPC
		//=======================================================================================
		//	For example: if we right clicked on an inventory item in the world and we do not have 
		//	anything selected that can pick it up, then in this case we shouldn't even bother 
		//	sending an RPC (dependent on command issuing options though).
		//---------------------------------------------------------------------------------------

		if (Target != nullptr)
		{
			EGameWarning bCanSendRPC = EGameWarning::None;
			
			/* Dunno what's faster: doing Cast<AInventoryItem> and checking if null, or doing it 
			like this. Probably Cast<AInventoryItem> is faster */
			ISelectable * const TargetAsSelectable = CastChecked<ISelectable>(Target);
			if (TargetAsSelectable->GetAttributesBase().GetSelectableType() == ESelectableType::InventoryItem)
			{
				AInventoryItem * TargetAsInventoryItem = CastChecked<AInventoryItem>(Target);
				
				bCanSendRPC = EGameWarning::NothingSelectedCanPickUpItem;
				
				if (CommandIssuingOptions::PickUpInventoryItemRule == EPickUpInventoryItemCommandIssuingRule::Single_QuickHasInventoryCheck)
				{
					for (const auto & Elem : Selected)
					{
						ISelectable * Selectable = CastChecked<ISelectable>(Elem);
						if (Selectable->GetInventory() != nullptr
							&& Selectable->GetInventory()->GetCapacity() > 0)
						{
							bCanSendRPC = EGameWarning::None;
							break;
						}
					}
				}
				else if (CommandIssuingOptions::PickUpInventoryItemRule == EPickUpInventoryItemCommandIssuingRule::Single_CheckIfCanPickUp)
				{
					const EInventoryItem ItemType = TargetAsInventoryItem->GetType();
					const int32 ItemQuantity = TargetAsInventoryItem->GetItemQuantity();
					const FInventoryItemInfo * ItemsInfo = TargetAsInventoryItem->GetItemInfo();

					for (const auto & Elem : Selected)
					{
						ISelectable * Selectable = CastChecked<ISelectable>(Elem);
						if (Selectable->GetInventory() != nullptr)
						{
							bCanSendRPC = Selectable->GetInventory()->CanItemEnterInventory(ItemType,
								ItemQuantity, *ItemsInfo, Selectable);
							if (bCanSendRPC == EGameWarning::None)
							{
								break;
							}
						}
					}
				}
				
				if (bCanSendRPC == EGameWarning::None)
				{
					FRightClickCommandOnInventoryItem Command(ClickLoc, TargetAsInventoryItem);

					FillCommandWithIDs(Command);

					Server_IssueRightClickCommandOnInventoryItem(Command);

					PlayRightClickCommandParticlesAndSound(SavedPrimarySelected, ClickLoc, Target);
				}
				else
				{
					PS->OnGameWarningHappened(bCanSendRPC);
				}
				
				/* We stop here and will not send RPC, but we also do not create click particles
				or do any other visual/audio feedback. Is that what we want? Dunno. Anyway if
				I want to change this make sure to change the server's case above near the
				start of this func */
				return;
			}
		}

		FRightClickCommandWithTarget Command(ClickLoc, Target);

		FillCommandWithIDs(Command);

		// RPC to issue command
		Server_IssueRightClickCommand(Command);
	}

	PlayRightClickCommandParticlesAndSound(SavedPrimarySelected, ClickLoc, Target);
}

void ARTSPlayerController::FillCommandWithIDs(FRightClickCommandBase & Command)
{
	CLIENT_CHECK;
	assert(Selected.Num() == SelectedIDs.Num());

	/* Check if we need to send the selectable IDs because the selectables we are
	issuing orders to are different from the group of the last order */
	bool bSendNewIDs = false;

	if (SelectedIDs.Num() == CommandIDs.Num())
	{
		for (const auto & Elem : CommandIDs)
		{
			if (!SelectedIDs.Contains(Elem))
			{
				bSendNewIDs = true;
				break;
			}
		}
	}
	else
	{
		bSendNewIDs = true;
	}

	if (bSendNewIDs)
	{
		CommandIDs.Reset(SelectedIDs.Num());

		Command.SetUseSameGroup(false);

		/* Attach selectables affected by command to it and remember what we will send */
		Command.ReserveAffectedSelectables(SelectedIDs.Num());
		for (const auto & SelectableID : SelectedIDs)
		{
			Command.AddSelectable(SelectableID);
			CommandIDs.Emplace(SelectableID);
		}
	}
	else
	{
		Command.SetUseSameGroup(true);
	}
}

void ARTSPlayerController::PlayRightClickCommandParticlesAndSound(const TScriptInterface<ISelectable> InPrimarySelected, 
	const FVector & ClickLoc, AActor * Target)
{
	/* Possibly play a sound */
	if (ShouldPlayMoveCommandSound(InPrimarySelected))
	{
		PlayMoveCommandSound(InPrimarySelected);
	}

	/* The rest of this code is for showing visuals at the click location - specifically a particle
	system and decal */

	ISelectable * TargetAsSelectable = nullptr;

	/* Get target type */
	ECommandTargetType TargetType = ECommandTargetType::NoTarget;
	if (Target != nullptr) // By now if target is non-null then it is a selectable
	{
		TargetAsSelectable = CastChecked<ISelectable>(Target);
		const EAffiliation Affiliation = TargetAsSelectable->GetAttributesBase().GetAffiliation();
		TargetType = Statics::AffiliationToCommandTargetType(Affiliation);
	}

	/* Hide previous particles if showing */
	if (Statics::IsValid(PreviousRightClickParticle))
	{
		/* As far as I'm aware the behavior will vary depending on how the particle system was
		setup in Cascade. If KillOnDeactivate it true then this will hide system straight away.
		Otherwise if KillOnComplete is true then it will stay visible but keep emitting until
		complete. And if both are false... is that even possible? I don't know */
		PreviousRightClickParticle->DeactivateSystem();
		//PreviousRightClickParticle->DestroyComponent();
	}

	/* Show new particles at click location */
	const FParticleInfo & ParticleInfo = FactionInfo->GetRightClickCommandStaticParticles(TargetType);
	FTransform SpawnTransform = ParticleInfo.GetTransform();
	SpawnTransform.SetLocation(SpawnTransform.GetLocation() + ClickLoc);
	PreviousRightClickParticle = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
		ParticleInfo.GetTemplate(), SpawnTransform);

	/* Only showing a decal if clicked on a selectable */
	if (TargetType != ECommandTargetType::NoTarget)
	{
		/* Let clicked selectable display their decal */
		TargetAsSelectable->OnRightClick();
	}
}

void ARTSPlayerController::Server_IssueRightClickCommand_Implementation(const FRightClickCommandWithTarget & Command)
{
	/* It would be faster to integrate this functionality into IssueRightClickCommand
	but I cannot be bothered copying and pasting that massive function here and having
	to change both when I want to change right click functionality so this way will
	work for now. TODO: condsider changing this */

	if (!Command.UseSameGroup())
	{
		Selected.Reset();

		for (const uint8 & ID : Command.GetAffectedSelectables())
		{
			AActor * const Selectable = PS->GetSelectableFromID(ID);
			Selected.Emplace(Selectable);
		}
	}

	IssueRightClickCommand(Command.GetClickLocation(), Command.GetTarget());
}

void ARTSPlayerController::Server_IssueRightClickCommandOnInventoryItem_Implementation(const FRightClickCommandOnInventoryItem & Command)
{
	if (!Command.UseSameGroup())
	{
		Selected.Reset(Command.GetAffectedSelectables().Num());

		for (const uint8 & ID : Command.GetAffectedSelectables())
		{
			AActor * const Selectable = PS->GetSelectableFromID(ID);
			Selected.Emplace(Selectable);
		}
	}

	IssueRightClickCommand(Command.GetClickLocation(), Command.GetClickedInventoryItem(GS));
}

EGameWarning ARTSPlayerController::IssueRightClickCommand(const FVector & Location, AActor * ClickedActor)
{
	SERVER_CHECK;

	// Whether to call PS::OnGameWarningHappened
	const bool bHandleWarnings = true;

	if (!IsSelectionControlledByThis())
	{
		return EGameWarning::SelectionNotUnderYourControl;
	}

	/* Handle case where right click was on another selectable revealed by fog */
	if (Statics::IsValid(ClickedActor))
	{
		// Only targets that are selectables should be passed into this func
		assert(IsASelectable(ClickedActor));

		ISelectable * const AsSelectable = CastChecked<ISelectable>(ClickedActor);

		/* Check if target is in fog. If they aren't then issue order but if they are then
		do nothing. This is because of possible modified client revealing enemies in fog.
		Could change this in future if it denies too many right-click commands. The reason
		not even a regular right click command is given is because the client thinks they
		clicked on a selectable and if that selectable is now in fog then they COULD want a
		right-click command on the ground instead but I think this is the better option */
		if (AsSelectable->Security_CanBeClickedOn(Team, TeamTag, GS))
		{
			const FSelectableAttributesBase & SInfo = AsSelectable->GetAttributesBase();
			
			/* This if is the first case of possibly many where we decide in advance what
			the specific command issuing func we should call on the selected selectable
			instead of just blindly calling the same one func on the unit and letting that
			func decide what to do */
			if (SInfo.GetSelectableType() == ESelectableType::InventoryItem)
			{
				/* TODO currently for the 2 AndMoveEveryoneElse methods we move everyone to the 
				same spot (the param Location)... which is not really what we want. We want them spread out */
				
				AInventoryItem * TargetAsInventoryItem = CastChecked<AInventoryItem>(ClickedActor);
				
				// This check probably only needs to happen if a remote client instigated the right click command
				if (TargetAsInventoryItem->IsInObjectPool())
				{
					if (bHandleWarnings)
					{
						PS->OnGameWarningHappened(EGameWarning::ItemNoLongerInWorld);
					}
					
					return EGameWarning::ItemNoLongerInWorld;
				}

				assert(TargetAsInventoryItem->IsInObjectPool() == false);

				const EInventoryItem ItemType = TargetAsInventoryItem->GetType();
				const FInventoryItemInfo * ItemsInfo = TargetAsInventoryItem->GetItemInfo();
				const int32 ItemQuantity = TargetAsInventoryItem->GetItemQuantity();

				switch (CommandIssuingOptions::PickUpInventoryItemRule)
				{
				case EPickUpInventoryItemCommandIssuingRule::Single_QuickHasInventoryCheck:
				{
					if (CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::First)
					{
						for (const auto & Elem : Selected)
						{
							/* If there's significant casting overhead it may be beneficial to 
							add an actor tag to query for whether selectable has an inventory 
							or not */
							ISelectable * Selectable = CastChecked<ISelectable>(Elem);
							const bool bCanTryAquire = Selectable->GetInventory() != nullptr && Selectable->GetInventory()->GetCapacity() > 0 && ItemsInfo->IsSelectablesTypeAcceptable(Selectable) == EGameWarning::None;
							if (bCanTryAquire)
							{
								Selectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);
								return EGameWarning::None;
							}
						}

						if (bHandleWarnings)
						{
							PS->OnGameWarningHappened(EGameWarning::NothingSelectedCanPickUpItem);
						}

						return EGameWarning::NothingSelectedCanPickUpItem;
					}
					else // Assumed Closest
					{
						float BestDistanceSqr = FLT_MAX;
						ISelectable * BestSelectable = nullptr;

						for (const auto & Elem : Selected)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Elem);
							const bool bCanTryAquire = Selectable->GetInventory() != nullptr && Selectable->GetInventory()->GetCapacity() > 0 && ItemsInfo->IsSelectablesTypeAcceptable(Selectable) == EGameWarning::None;
							if (bCanTryAquire)
							{
								const float DistanceSqr = Statics::GetSelectablesDistanceSquaredFromInventoryItemActor(Selectable, ClickedActor);
								if (DistanceSqr < BestDistanceSqr)
								{
									BestDistanceSqr = DistanceSqr;
									BestSelectable = Selectable;
								}
							}
						}

						if (BestSelectable != nullptr)
						{
							BestSelectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);
							return EGameWarning::None;
						}
						else
						{
							if (bHandleWarnings)
							{
								PS->OnGameWarningHappened(EGameWarning::NothingSelectedCanPickUpItem);
							}

							return EGameWarning::NothingSelectedCanPickUpItem;
						}
					}

					// This break like every other in this switch statement never gets reached
					break;
				}
				case EPickUpInventoryItemCommandIssuingRule::Single_CheckIfCanPickUp:
				{
					if (CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::First)
					{
						for (const auto & Elem : Selected)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Elem);
							if (Selectable->GetInventory() != nullptr)
							{
								const EGameWarning Result = Selectable->GetInventory()->CanItemEnterInventory(ItemType,
									ItemQuantity, *ItemsInfo, Selectable);
								if (Result == EGameWarning::None)
								{
									Selectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);
									return EGameWarning::None;
								}
							}
						}

						if (bHandleWarnings)
						{
							PS->OnGameWarningHappened(EGameWarning::NothingSelectedCanPickUpItem);
						}

						return EGameWarning::NothingSelectedCanPickUpItem;
					}
					else // Assumed Closest
					{
						float BestDistanceSqr = FLT_MAX;
						ISelectable * BestSelectable = nullptr;

						for (const auto & Elem : Selected)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Elem);
							if (Selectable->GetInventory() != nullptr)
							{
								/* Going to do the distance check first since CanItemEnterInventory 
								is probably more costly */
								const float DistanceSqr = Statics::GetSelectablesDistanceSquaredFromInventoryItemActor(Selectable, ClickedActor);
								if (DistanceSqr < BestDistanceSqr)
								{
									const EGameWarning Result = Selectable->GetInventory()->CanItemEnterInventory(ItemType,
										ItemQuantity, *ItemsInfo, Selectable);
									if (Result == EGameWarning::None)
									{
										BestDistanceSqr = DistanceSqr;
										BestSelectable = Selectable;
									}
								}
							}
						}

						// Issue command
						if (BestSelectable != nullptr)
						{
							BestSelectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);
							return EGameWarning::None;
						}
						else
						{
							if (bHandleWarnings)
							{
								PS->OnGameWarningHappened(EGameWarning::NothingSelectedCanPickUpItem);
							}

							return EGameWarning::NothingSelectedCanPickUpItem;
						}
					}
	
					break;
				}
				case EPickUpInventoryItemCommandIssuingRule::Single_QuickHasInventoryCheck_AndMoveEveryoneElse:
				{
					if (CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::First)
					{
						bool bIssuedPickUpItemCommand = false;
						
						for (const auto & Elem : Selected)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Elem);
							if (!bIssuedPickUpItemCommand && Selectable->GetInventory() != nullptr
								&& Selectable->GetInventory()->GetCapacity() > 0 
								&& ItemsInfo->IsSelectablesTypeAcceptable(Selectable) == EGameWarning::None)
							{
								Selectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);

								bIssuedPickUpItemCommand = true;
							}
							else
							{
								Selectable->IssueCommand_MoveTo(Location);
							}
						}

						return EGameWarning::None;
					}
					else // Assumed Closest
					{
						float BestDistanceSqr = FLT_MAX;
						ISelectable * BestSelectable = nullptr;
						
						// First loop to find the closest selectable that can pick up item
						for (int32 i = 0; i < Selected.Num(); ++i)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
							if (Selectable->GetInventory() != nullptr
								&& Selectable->GetInventory()->GetCapacity() > 0 
								&& ItemsInfo->IsSelectablesTypeAcceptable(Selectable) == EGameWarning::None)
							{
								const float DistanceSqr = Statics::GetSelectablesDistanceSquaredFromInventoryItemActor(Selectable, ClickedActor);
								if (DistanceSqr < BestDistanceSqr)
								{
									BestDistanceSqr = DistanceSqr;
									BestSelectable = Selectable;
								}
							}
						}

						/* Second loop to issue the commands. Iterating backwards because higher
						entries are more likely to still be in CPU cache from last loop */
						for (int32 i = Selected.Num() - 1; i >= 0; --i)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
							if (Selectable == BestSelectable)
							{
								Selectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);
							}
							else
							{
								Selectable->IssueCommand_MoveTo(Location);
							}
						}
						
						return EGameWarning::None;
					}
					
					break;
				}
				case EPickUpInventoryItemCommandIssuingRule::Single_CheckIfCanPickUp_AndMoveEveryoneElse:
				{
					if (CommandIssuingOptions::SingleIssueSelectionRule == ESingleCommandIssuingRule::First)
					{
						bool bIssuedPickUpItemCommand = false;
						
						for (const auto & Elem : Selected)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Elem);
							if (!bIssuedPickUpItemCommand && Selectable->GetInventory() != nullptr)
							{
								const EGameWarning Result = Selectable->GetInventory()->CanItemEnterInventory(ItemType, 
									ItemQuantity, *ItemsInfo, Selectable);
								if (Result == EGameWarning::None)
								{
									Selectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);
									
									bIssuedPickUpItemCommand = true;
								}
								else
								{
									Selectable->IssueCommand_MoveTo(Location);
								}
							}
							else
							{
								Selectable->IssueCommand_MoveTo(Location);
							}
						}

						return EGameWarning::None;
					}
					else // Assumed Closest
					{
						float BestDistanceSqr = FLT_MAX;
						ISelectable * BestSelectable = nullptr;

						// First loop to find the closest selectable that can pick up the item
						for (int32 i = 0; i < Selected.Num(); ++i)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
							if (Selectable->GetInventory() != nullptr)
							{
								const float DistanceSqr = Statics::GetSelectablesDistanceSquaredFromInventoryItemActor(Selectable, ClickedActor);
								if (DistanceSqr < BestDistanceSqr)
								{
									const EGameWarning Result = Selectable->GetInventory()->CanItemEnterInventory(ItemType, 
										ItemQuantity, *ItemsInfo, Selectable);
									if (Result == EGameWarning::None)
									{
										BestDistanceSqr = DistanceSqr;
										BestSelectable = Selectable;
									}
								}
							}
						}

						// Second loop to issue commands
						for (int32 i = Selected.Num() - 1; i >= 0; --i)
						{
							ISelectable * Selectable = CastChecked<ISelectable>(Selected[i]);
							if (Selectable == BestSelectable)
							{
								Selectable->IssueCommand_PickUpItem(Location, TargetAsInventoryItem);
							}
							else
							{
								Selectable->IssueCommand_MoveTo(Location);
							}
						}

						return EGameWarning::None;
					}
					
					break;
				}
				default:
				{
					UE_LOG(RTSLOG, Fatal, TEXT("Unimplemented rule: \"%s\" "), 
						TO_STRING(EPickUpInventoryItemCommandIssuingRule, CommandIssuingOptions::PickUpInventoryItemRule));
					break;
				}
				}
			}
			else if (SInfo.GetBuildingType() == EBuildingType::ResourceSpot)
			{
				AResourceSpot * ResourceSpot = CastChecked<AResourceSpot>(ClickedActor);
				
				for (const auto & Elem : Selected)
				{
					/* Because networking */
					if (!Statics::IsValid(Elem))
					{
						continue;
					}
					
					ISelectable * const OwnedSelectable = CastChecked<ISelectable>(Elem);
					OwnedSelectable->IssueCommand_RightClickOnResourceSpot(ResourceSpot);
				}

				return EGameWarning::None;
			}
			else if (SInfo.GetSelectableType() == ESelectableType::Building)
			{
				ABuilding * ClickedBuilding = CastChecked<ABuilding>(ClickedActor);
				ARTSPlayerState * ClickedBuildingsOwner = ClickedBuilding->Selectable_GetPS();
				const EFaction BuildingsFaction = ClickedBuilding->GetFaction();
				const EBuildingType BuildingsType = SInfo.GetBuildingType();
				const EAffiliation BuildingsAffiliation = (ClickedBuildingsOwner == PS)
					? EAffiliation::Owned 
					: ClickedBuildingsOwner->IsAllied(PS)
					? EAffiliation::Allied 
					: EAffiliation::Hostile;

				const FBuildingGarrisonAttributes & GarrisonAttributes = ClickedBuilding->GetBuildingAttributes()->GetGarrisonAttributes();
				if (GarrisonAttributes.IsGarrison())
				{
					/* Behavior I have here is just try and send everything into the garrison. */

					for (int32 i = 0; i < Selected.Num(); ++i)
					{
						AActor * const Selectable = Selected[i];

						/* Because networking */
						if (!Statics::IsValid(Selectable))
						{
							continue;
						}

						ISelectable * const OwnedSelectable = CastChecked<ISelectable>(Selectable);
						OwnedSelectable->IssueCommand_EnterGarrison(ClickedBuilding, GarrisonAttributes);
					}
				}
				else
				{
					/* Some of selected may have special abilities that target buildings e.g. engineers
					in C&C capture the building, so make sure we issue those if they have them */
					for (int32 i = 0; i < Selected.Num(); ++i)
					{
						AActor * const Selectable = Selected[i];

						/* Because networking */
						if (!Statics::IsValid(Selectable))
						{
							continue;
						}

						ISelectable * const OwnedSelectable = CastChecked<ISelectable>(Selectable);
						const FBuildingTargetingAbilityPerSelectableInfo * AbilityInfo = OwnedSelectable->GetSpecialRightClickActionTowardsBuildingInfo(ClickedBuilding, BuildingsFaction, BuildingsType, BuildingsAffiliation);
						if (AbilityInfo != nullptr)
						{
							OwnedSelectable->IssueCommand_SpecialBuildingTargetingAbility(ClickedBuilding, AbilityInfo);
						}
						else
						{
							OwnedSelectable->OnRightClickCommand(AsSelectable, SInfo);
						}
					}
				}
				
				return EGameWarning::None;
			}
			else // Default case. Probably eventually going to remove this
			{
				for (int32 i = 0; i < Selected.Num(); ++i)
				{
					AActor * const Selectable = Selected[i];

					/* Because networking */
					if (!Statics::IsValid(Selectable))
					{
						continue;
					}

					ISelectable * const OwnedSelectable = CastChecked<ISelectable>(Selectable);
					OwnedSelectable->OnRightClickCommand(AsSelectable, SInfo);
				}

				return EGameWarning::None;
			}
		}
		else
		{
			/* Basically the target is in fog or can't be selected. I guess return something 
			along those lines */

			if (bHandleWarnings)
			{
				PS->OnGameWarningHappened(EGameWarning::TargetNotVisible);
			}

			return EGameWarning::TargetNotVisible;
		}
	}

	/* If here then something like the ground was clicked on */

	/* Because we want the units to each move to different locations
	we give a right click command most of this function just shifts
	the locations for each unit */

	/* Take middle unit in Selected and use its location to
	work out angle for final move location. A perhaps better
	way would be to figure out the angles for all the units
	selected and average the angle. */
	const int32 UnitIndex = Selected.Num() / 2;
	assert(Selected.IsValidIndex(UnitIndex));
	AActor * const Middle = Selected[UnitIndex];
	assert(Middle != nullptr);
	/* How much to rotate spots around clock location. For some
	reason I need to add 90 degrees to this to get the rotation
	I thought it would calculate */
	const FRotator Rotation = UKismetMathLibrary::FindLookAtRotation(Middle->GetActorLocation(), Location) + FRotator(0.f, 90.f, 0.f);

	/* Calculate distance squared from unit picked above to
	click location */
	const float DistanceSqr = (Middle->GetActorLocation() - Location).SizeSquared();

	/* Move commands that make the unit picked above move
	less than this distance will not change the rotation
	of the selected units. When doing clicks that are close
	and near the selected units the ones in the front have
	to try and pass through the ones at the back and lots
	of colliding takes place. This is a quick fix kinda. The
	ideal solution would be that rotation always happens but
	each unit takes the best spot which makes the least
	collisions. */
	const float RotationDistanceThresholdSqr = 500.f * 500.f;

	/* TODO: Rotation resets back to the same formation when the
	move is considered not far enough. The last rotation needs to
	be saved and then that rotation is used again if the move is
	considered too short. But it would have to be stored for each
	unit for it to have a chance of working */

	/* Distance between each units move location */
	const float Spacing = 200.f;

	/* Number of units allowed per rank or line */
	const int32 RankSize = 4;

	const int32 NumSelected = Selected.Num();

	/* Basically do integer division but take the roof instead
	of floor */
	const int32 NumRanks = (NumSelected % RankSize) ? NumSelected / RankSize + 1
		: NumSelected / RankSize;

	/* Because we want the click location to be in the center of
	all the units when they have finished moving instead of being
	in a corner we will figure that out now and adjust the value
	of Location */

	/* If true then there will be a rank that goes right through the
	click location */
	const FVector NewCorner = NumRanks % 2 ? Location - FVector(0.f, Spacing * (NumRanks / 2), 0.f)
		: Location - FVector(0.f, Spacing * (NumRanks / 2 - 1) + Spacing / 2, 0.f);

	float RankOffset = RankSize % 2 ? RankSize / 2 : RankSize / 2 - 0.5f;

	for (int32 i = 0; i < NumRanks; ++i)
	{
		if (i == NumRanks - 1)
		{
			/* We are on the last rank which may not be a full rank.
			Make sure to center them */

			/* Ternary operator there in case last rank is full */
			const int32 NumInLastRank = NumSelected % RankSize ? NumSelected % RankSize : RankSize;
			const int32 NumMissing = RankSize - NumInLastRank;

			/* RankOffset is dependent on whether RankSize is odd or even. */
			if (RankSize % 2 == 1)
			{
				RankOffset -= NumInLastRank % 2 ? (NumMissing / 2) : (NumMissing / 2.f);
			}
			else
			{
				RankOffset -= NumInLastRank % 2 ? (NumMissing / 2.f) : (NumMissing / 2);
			}

			for (int32 j = 0; j < NumInLastRank; ++j)
			{
				/* Because networking */
				if (!Statics::IsValid(Selected[i * RankSize + j]))
				{
					continue;
				}

				assert(Selected.IsValidIndex(i * RankSize + j));
				ISelectable * const AsSelectable = CastChecked<ISelectable>(Selected[i * RankSize + j]);

				FVector MoveLocation = NewCorner + FVector((j - RankOffset) * Spacing, i * Spacing, 0.f);

				/* Figure out correct rotation by rotating about original
				click location if move command is at least a certain distance */
				if (DistanceSqr > RotationDistanceThresholdSqr)
				{
					MoveLocation -= Location;
					MoveLocation = Rotation.RotateVector(MoveLocation);
					MoveLocation += Location;
				}

				/* TODO: check if MoveLocation is valid. If not do something */

				/* Issue move command to unit */
				AsSelectable->OnRightClickCommand(MoveLocation);
			}
		}
		else
		{
			for (int32 j = 0; j < RankSize; ++j)
			{
				/* Because networking */
				if (!Statics::IsValid(Selected[i * RankSize + j]))
				{
					continue;
				}

				assert(Selected.IsValidIndex(i * RankSize + j));
				ISelectable * const AsSelectable = CastChecked<ISelectable>(Selected[i * RankSize + j]);

				FVector MoveLocation = NewCorner + FVector((j - RankOffset) * Spacing, i * Spacing, 0.f);

				/* Figure out correct rotation by rotating about original
				click location if move command is at least a certain distance */
				if (DistanceSqr > RotationDistanceThresholdSqr)
				{
					MoveLocation -= Location;
					MoveLocation = Rotation.RotateVector(MoveLocation);
					MoveLocation += Location;
				}

				/* TODO: check if MoveLocation is valid. If not do something */

				/* Issue move command to unit */
				AsSelectable->OnRightClickCommand(MoveLocation);
			}
		}
	}

	// Return "None" which means success
	return EGameWarning::None;
}

void ARTSPlayerController::SetupFogOfWarManager(ARTSLevelVolume * MapFogVolume, uint8 InNumTeams, ETeam InTeam)
{
	if (!IsObserver())
	{
#if GAME_THREAD_FOG_OF_WAR

		FogOfWarManager = GetWorld()->SpawnActor<AFogOfWarManager>(Statics::INFO_ACTOR_SPAWN_LOCATION,
			FRotator::ZeroRotator);
		FogOfWarManager->Initialize(MapFogVolume, InNumTeams, InTeam, GI->GetFogOfWarMaterial());

		GS->SetFogManager(FogOfWarManager);

#elif MULTITHREADED_FOG_OF_WAR

		MultithreadedFogOfWarManager::Get().Setup(GetWorld(), MapFogVolume, InNumTeams, 
			Statics::TeamToArrayIndex(InTeam), GI->GetFogOfWarMaterial());

#endif
	}
}

void ARTSPlayerController::SetupGhostPool()
{
	assert(GhostPool.Num() == 0);

	for (const auto & Elem : FactionInfo->GetAllBuildingTypes())
	{
		const FBuildingInfo & BuildInfo = Elem.Value;
		
		AGhostBuilding * Ghost = GetWorld()->SpawnActor<AGhostBuilding>(BuildInfo.GetGhostBuildingBP(),
			Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);
		assert(Statics::IsValid(Ghost));

		// Because of this line it is important this func is called after SetStartingSpotInMatch
		Ghost->SetInitialValues(BuildInfo, DefaultCameraRotation.Yaw);

		Ghost->OnEnterPool();

		assert(!GhostPool.Contains(BuildInfo.GetBuildingType()));
		GhostPool.Emplace(BuildInfo.GetBuildingType(), Ghost);
	}
}

void ARTSPlayerController::SetupExternalReferences()
{
	PS->SetPC(this);
	assert(GI != nullptr);
	PS->SetGI(GI);
}

void ARTSPlayerController::SetupPlayerID(const FName & InPlayerID)
{
	PlayerID = InPlayerID;
}

void ARTSPlayerController::InitHUD(EFaction InFaction, EStartingResourceAmount InStartingResources,
	ARTSLevelVolume * FogVolume)
{
	/* Setup and show HUD widget */
	HUD = CastChecked<URTSHUD>(GetWidget(EMatchWidgetType::HUD, false));
#if MINIMAP_ENABLED_GAME
	HUD->SetupMinimap(FogVolume);
#endif

	// Z ordering should put this below loading screen
	ShowWidget(EMatchWidgetType::HUD, false);

	// Stuff that needs to happen after AddToViewport is called
	HUD->SetupWidget(GI, this);

	/* Set the initial value so a resource update animation won't play. It is possible that the
	OnRep for resources in player state will happen after and cause an animation to play anyway */
	const TArray < int32 > & StartingResourceArray = GI->GetStartingResourceConfig(InStartingResources).GetResourceArraySlow();
	PS->Client_SetInitialResourceAmounts(StartingResourceArray, HUD);

	/* Setup the marquee box drawing HUD */
	MarqueeHUD = CastChecked<AMarqueeHUD>(GetHUD());
	MarqueeHUD->SetMarqueeSettingsValues(GI->GetMarqueeSelectionBoxStyle(),
		GI->GetMarqueeBoxRectangleFillColor(), GI->GetMarqueeBoxBorderColor(),
		GI->GetMarqueeBoxBorderLineThickness());

#if WITH_EDITOR
	if (GI->EditorPlaySession_IsCheatWidgetBPSet())
	{
		/* Create the optional development cheat widget */ 

		DevelopmentCheatWidget = CreateWidget<UInMatchDeveloperWidget>(this, GI->EditorPlaySession_GetCheatWidgetBP());

		// This is where RebuildWidget() is called. Also it appears the vis it gets is SelfHitTestInvisible
		DevelopmentCheatWidget->AddToViewport(GetWidgetZOrder(EMatchWidgetType::DevelopmentCheats));

		DevelopmentCheatWidget->SetupWidget(GI, this);

		if (GI->EditorPlaySession_ShouldInitiallyShowCheatWidget() == false)
		{
			DevelopmentCheatWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
#endif // WITH_EDITOR
}

void ARTSPlayerController::SetupMouseCursorProperties()
{
	const FCursorInfo * Cursor = nullptr;

	if (GI->IsInMainMenuMap())
	{
		/* For PIE/standalone and when bSkipMainMenu is true I'm taking this branch TODO */
		
		if (GI->GetMenuCursorInfo().ContainsCustomCursor())
		{
			Cursor = &GI->GetMenuCursorInfo();
		}
	}
	else
	{
		if (FactionInfo->GetMouseCursorInfo().ContainsCustomCursor())
		{
			Cursor = &FactionInfo->GetMouseCursorInfo();
		}
	}

	/* Only set cursor if one is valid. If abilities use a custom cursor for their targeting and
	cursor was null here then there will be problems because they will need to revert to the
	default cursor afterwards but we haven't specified one */
	if (Cursor != nullptr)
	{
		SetMouseCursor(*Cursor, true);
		DefaultCursor = Cursor;
		//ScreenLocationCurrentCursor = DefaultCursor; May need this line if crashes
	}
}

void ARTSPlayerController::SetStartingSpotInMatch(int16 InStartingSpotID, FMapID MapID)
{
	const FMapInfo & MapInfo = GI->GetMapInfo(MapID);

	// Figure out start location
	FVector InitialLocation;
	FRotator InitialRotation;
	MapInfo.GetPlayerStartTransform(InStartingSpotID, InitialLocation, InitialRotation);

	/* Setting this may even be irrelevant since it doesn't move the pawn (camera) and that 
	is really what needs to be moved */
	SetInitialLocationAndRotation(InitialLocation, InitialRotation);

	// Above line does not move pawn (camera) so need to do this 
	GetPawn()->SetActorLocationAndRotation(InitialLocation, InitialRotation);

	// Remember yaw so we know what yaw to return to when pressing 'reset camera yaw' button
	SetDefaultCameraYaw(InitialRotation.Yaw);
}

void ARTSPlayerController::Server_SetupForMatch(uint8 InPlayerID, ETeam InTeam, EFaction InFaction,
	EStartingResourceAmount StartingResources, uint8 NumTeams, FMapID InMapID, int16 InStartingSpotID)
{
	SetupReferences();
	PoolingManager = GI->GetPoolingManager();
	SetupExternalReferences();

	GS->SetupForMatch(PoolingManager, NumTeams);

	Team = InTeam;
	TeamTag = GS->GetTeamTag(InTeam);

	// Set this on server, maybe needed when a player leaves
	Type = (InTeam == ETeam::Observer) ? EPlayerType::Observer : EPlayerType::Player;

	GS->AddController(this);

	SetupPlayerID(*FString::FromInt(InPlayerID));

	FactionInfo = GI->GetFactionInfo(InFaction);

	TArray <int32> StartingResourceArray = GI->GetStartingResourceConfig(StartingResources).GetResourceArraySlow();
	PS->Server_SetInitialValues(InPlayerID, PlayerID, InTeam, InFaction, InStartingSpotID, StartingResourceArray);

	Client_SetupForMatch(InPlayerID, InTeam, InFaction, NumTeams, StartingResources, InMapID, 
		InStartingSpotID);
}

// Important: if changing params make sure to change timer delegate params below too
void ARTSPlayerController::Client_SetupForMatch_Implementation(uint8 InPlayerID, ETeam InTeam,
	EFaction InFaction, uint8 NumTeams, EStartingResourceAmount StartingResources,
	FMapID InMapID, int16 InStartingSpotID)
{
	/* TODO: remove some things from this */

#if WITH_EDITOR
	// TODO check a client RPC called on client doesn't generate bandwidth, cause doing a lot of 
	// that here. Then again we are in PIE so shouldn't affect anything

	/* Also are all these RPCs getting put in a queue and then sent multiple times? Like
	if this function in call multiple times before being sent to server will the function
	be called multiple times? If so don't want that, need a safegaurd for it. Also some GM
	code that uses polling might need to change like this too */

	/* For testing in PIE player state, pawn and game state are at least 2 things that have had
	crashes because they haven't repped by the time this function runs. Using delays shows this.
	Going to poll that they have repped before carrying out rest of function */
	if (PlayerState == nullptr || GetPawn() == nullptr || GetWorld()->GetGameState() == nullptr)
	{	
		/* Possibly add another param to if that checks that all player states have repped. Would
		need another param in function which would also mean TimerDel.BindUFunction right below
		needs to be updated to include that new param */

		static const FName FuncName = GET_FUNCTION_NAME_CHECKED(ARTSPlayerController, Client_SetupForMatch);
		FTimerDelegate TimerDel;
		FTimerHandle DummyTimerHandle;
		TimerDel.BindUFunction(this, FuncName, InPlayerID, InTeam, InFaction, NumTeams,
			StartingResources, InMapID, InStartingSpotID);
		GetWorldTimerManager().SetTimer(DummyTimerHandle, TimerDel, 0.01f, false);
		return;
	}
#endif // WITH_EDITOR

	Team = InTeam;
	// Currently only used by SetupInputComponent
	Type = (InTeam == ETeam::Observer) ? EPlayerType::Observer : EPlayerType::Player;

	/* No biggie to check, just stops repeating stuff already done in Server_PostLogin */
	if (!GetWorld()->IsServer())
	{
		SetupReferences();
		SetupExternalReferences();

		/* Initialize game instance if it hasn't been already */
		GI->Initialize();

		PoolingManager = GI->GetPoolingManager();

		/* Usually set in BeginPlay but is not set on clients for next group of functions
		apparently */
		GS->SetGI(GI);
	}

	/* If we came from lobby as opposed to PIE + skip main menu then destroy all menu related
	widgets except the match loading screen */
#if WITH_EDITOR
	if (GI->EditorPlaySession_ShouldSkipMainMenu())
	{
		/* This is so if player leaves during the match their game mode will think they just
		left game and will show main menu */
		GI->SetIsColdBooting(false);
	}
	else
	{
		GI->DestroyMenuWidgetsBeforeMatch();
	}
#else
	GI->DestroyMenuWidgetsBeforeMatch();
#endif

	// Do this if it hasn't been done already
	GS->SetupForMatch(PoolingManager, NumTeams);
	if (GetWorld()->GetFirstPlayerController() == this)
	{
		GS->SetLocalPlayersTeam(InTeam);
	}

	TeamTag = GS->GetTeamTag(InTeam);

	/* This will set our own PS's ID but we need every other PS's ID set. In match initializing
	from lobby PS::Multicast_SetInitialValues does that. When loading from PIE this may need
	to be done too */
	SetupPlayerID(*FString::FromInt(InPlayerID));

	/* These rely on replication of varaibles happening beforehand, but not anymore because
	of param */
	FactionInfo = GI->GetFactionInfo(InFaction);
	PS->SetFactionInfo(FactionInfo);
	PS->SetupProductionCapableBuildingsMap();
	PS->SetTeamTag(TeamTag);

	ARTSLevelVolume * FogVolume = nullptr;

#if GAME_THREAD_FOG_OF_WAR || MULTITHREADED_FOG_OF_WAR || MINIMAP_ENABLED_GAME

	// Get reference to the maps level volume
	for (TActorIterator<ARTSLevelVolume> Iter(GetWorld()); Iter; ++Iter)
	{
		FogVolume = *Iter;
		break;
	}

	UE_CLOG(FogVolume == nullptr, RTSLOG, Fatal, TEXT("Fog of war and/or minimap is enabled for "
		"project but no fog volume was found on map. Add a fog volume to the map"));
	// TODO crash here because no fog volume, but alternative where use can still play just with 
	// no fog and/or minimap would be nice 

#endif // GAME_THREAD_FOG_OF_WAR || MULTITHREADED_FOG_OF_WAR || MINIMAP_ENABLED_GAME

	SetupFogOfWarManager(FogVolume, NumTeams, InTeam);

	InitHUD(InFaction, StartingResources, FogVolume);

	if (FogVolume != nullptr)
	{
		//const bool bResult = FogVolume->Destroy(); 

		/* Incoming verbose comment: 
		This has been commented. We will now leave the RTS level volume on the map because it 
		contains the 4 map walls. 
		But side note: Destroy here only works on server, and not on remote clients. But the level 
		volume isn't replicated, so this is unexpected */
	}

	SetupCameraReferences();

	SetupMouseCursorProperties();

	AssignViewportValues();

	/* Apply control settings */
	UGameUserSettings * UserSettings = UGameUserSettings::GetGameUserSettings();
	URTSGameUserSettings * RTSSettings = CastChecked<URTSGameUserSettings>(UserSettings);
	RTSSettings->ApplyControlSettings(this);

	// Teleport to start location and set default camera yaw
	SetStartingSpotInMatch(InStartingSpotID, InMapID);

	// Recently moved from below PS->SetTeamTag
	SetupGhostPool();

	SetActorTickEnabled(true);

	/* When this was called by the engine source all it called was super, but now player type is
	known we should call it */
	SetupInputComponent();

	/* Set attenuation listener to the invisible sphere of the camera so audio levels stay the 
	same no matter how far zoomed in or out we are */
	SetAudioListenerAttenuationOverride(GetPawn()->GetRootComponent(), FVector::ZeroVector);

	// Tell game state we have completed our setup process.
	Server_AckSetupForMatchComplete();

	/* Camera is spawned using default pawn class in GM */
}

void ARTSPlayerController::Server_AckSetupForMatchComplete_Implementation()
{
	GS->Server_AckPCSetupComplete(this);
}

void ARTSPlayerController::Server_BeginPlay()
{
	assert(GetWorld()->IsServer());

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());
	/* To set the menu mouse cursor */
	SetupMouseCursorProperties();
}

void ARTSPlayerController::Client_OnPIEStarted_Implementation()
{
	/* Enable input */
	FInputModeGameAndUI InputMode;
	/* Fixes flashing during clicks but then implies mouse is getting captured on clicks */
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	
	/* Position mouse in middle of screen. Otherwise it starts at the edge and therefore 
	edge scrolling happens straight away. This is hardcoded though */
	//SetMouseLocation(2560, 1440); // Doesn't work
	FEventReply Dummy;
	UWidgetBlueprintLibrary::SetMousePosition(Dummy, FVector2D(-1280.f, 700.f)); // Puts mouse at top of screen. Better, but still doesn't do what I want

	if (GetWorld()->IsServer())
	{
		/* Play music just for the faction that server player is on */
		GI->PlayMusic(FactionInfo->GetMatchMusic());
	}
}

void ARTSPlayerController::OnMatchStarted()
{
	/* Play 3 second camera fade */
	PlayerCameraManager->StartCameraFade(1.f, 0.f, 3.f, FLinearColor::Black, true, true);

	/* Hide loading screen. It's actually possible to have no loading screen widget if the
	widget exit anim is slow enough to let map load before loading screen even needs to be
	shown */
	GI->HideAllMenuWidgets();

	/* Enable input */
	FInputModeGameAndUI InputMode;
	/* Fixes flashing during clicks but then implies mouse if getting captured on clicks */
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	/* Play faction music */
	GI->PlayMusic(FactionInfo->GetMatchMusic());
}

void ARTSPlayerController::NotifyOfBuildingReachingZeroHealth(ABuilding * Building, 
	USoundBase * ZeroHealthSound, bool bWasSelected)
{
	/* Try play a sound */
	if (ShouldPlayZeroHealthSound(Building, ZeroHealthSound))
	{
		PlayZeroHealthSound(ZeroHealthSound);
	}

	if (bWasSelected)
	{
		RemoveFromSelected<ABuilding>(Building);
	}
}

void ARTSPlayerController::NotifyOfInfantryReachingZeroHealth(AInfantry * Infantry,
	USoundBase * ZeroHealthSound, bool bWasSelected)
{
	/* Try play a sound */
	if (ShouldPlayZeroHealthSound(Infantry, ZeroHealthSound))
	{
		PlayZeroHealthSound(ZeroHealthSound);
	}

	if (bWasSelected)
	{
		RemoveFromSelected<AInfantry>(Infantry);
	}
}

void ARTSPlayerController::NotifyOfInfantryEnteringGarrison(AInfantry * Infantry, bool bWasSelected)
{
	if (bWasSelected)
	{
		RemoveFromSelected<AInfantry>(Infantry);
		Infantry->OnDeselect();
	}
}

void ARTSPlayerController::NotifyOfInventoryItemBeingPickedUp(AInventoryItem * InventoryItem, bool bWasSelected)
{
	if (bWasSelected)
	{
		RemoveFromSelected<AInventoryItem>(InventoryItem);
	}
}

void ARTSPlayerController::ServerUnloadSingleUnitFromGarrison(ABuilding * BuildingGarrison, AActor * SelectableToUnload)
{
	BuildingGarrison->GetBuildingAttributesModifiable()->GetGarrisonAttributes().ServerUnloadSingleUnit(GS, BuildingGarrison, SelectableToUnload, HUD);
}

void ARTSPlayerController::ServerUnloadGarrison(ABuilding * BuildingGarrison)
{
	BuildingGarrison->GetBuildingAttributesModifiable()->GetGarrisonAttributes().ServerUnloadAll(GS, BuildingGarrison, HUD);
}

void ARTSPlayerController::Server_RequestUnloadSingleUnitFromGarrison_Implementation(ABuilding * BuildingGarrison, AActor * SelectableToUnload)
{
	if (Statics::IsValid(BuildingGarrison) && !Statics::HasZeroHealth(BuildingGarrison) 
		&& Statics::IsValid(SelectableToUnload) && !Statics::HasZeroHealth(SelectableToUnload))
	{
		ServerUnloadSingleUnitFromGarrison(BuildingGarrison, SelectableToUnload);
	}
}

void ARTSPlayerController::Server_RequestUnloadGarrison_Implementation(ABuilding * BuildingGarrison)
{
	if (Statics::IsValid(BuildingGarrison) && !Statics::HasZeroHealth(BuildingGarrison))
	{
		ServerUnloadGarrison(BuildingGarrison);
	}
}

template <typename TSelectable>
void ARTSPlayerController::RemoveFromSelected(TSelectable * Selectable)
{
	assert(Selectable != nullptr);
	
	/* Maybe book keep while doing this and remove non-valids. Not sure I do it anywhere else
	so Selected can potentially just keep growing. It's quite rare though: jist of how it
	happens is that the zero health health rep never happens (actually dunno if UE4 allows
	that to happen.
	How it would happen: basically Health which is a replicated variable changes on server but
	the change is never repped to clients). That's really a whole nother issue though and
	should be dealt with in each selectable class, not here in PC */

	// Commenting this, doesn't make sense to check since we assert Selectable != nullptr
	/* If param is null then it is no longer being replicated */
	//if (!HasSelection())
	//{
	//	return;
	//}

	/* First remove ID from SelectedIDs */
	const uint8 SelectableID = Selectable->GetSelectableID();
	SelectedIDs.Remove(SelectableID);

	/* Check if the param is in index 0 (and is therefore the selectable
	having their context menu shown) */
	if (Selectable == Selected[0])
	{
		/* The selectable to remove is the one whose context menu is showing.
		Got to find the most suitable unit to replace it */
		EUnitType TypeToBeCurrent = EUnitType::None;
		int32 IndexToSwap = 0;
		for (int32 i = 1; i < Selected.Num(); ++i)
		{
			AActor * Elem = Selected[i];

			ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);
			const EUnitType UnitType = AsSelectable->GetAttributesBase().GetUnitType();
			if (UnitType > TypeToBeCurrent)
			{
				TypeToBeCurrent = UnitType;
				IndexToSwap = i;
			}
		}

		/* Swap new context menu unit into index 0 and remove param */
		Selected.SwapMemory(0, IndexToSwap);
		Selected.SwapMemory(IndexToSwap, Selected.Num() - 1);
		Selected.RemoveAt(Selected.Num() - 1, 1, false);

		if (Selected.IsValidIndex(0))
		{
			CurrentSelected = TScriptInterface<ISelectable>(Selected[0]);
			CurrentSelected->GetAttributesBase().SetIsPrimarySelected(true);
			HUD->OnPlayerCurrentSelectedChanged(CurrentSelected);
		}
		else
		{
			/* Param was the only selectable selected */
			CurrentSelected = nullptr;
			HUD->OnPlayerNoSelection();

			/* Cancel pending context action if there is one */
			if (PendingContextAction != EContextButton::None)
			{
				CancelPendingContextCommand();
			}
		}

		bHasPrimarySelectedChanged = true;
	}
	else
	{
		Selected.RemoveSingleSwap(Selectable, false);
	}
}

void ARTSPlayerController::OnInventoryIndicesOfPrimarySelectedSwapped(uint8 ServerIndex_1, uint8 ServerIndex_2)
{
	/* Oh, I just realized that the pointer points to an entry in FInventory::SlotsArray, and 
	that when a swap happens we never reorder it, so this can be a NOOP */
	
	//if (!IsPlacingGhost() && IsContextActionPending())
	//{
	//	if (PendingContextActionUseMethod == EAbilityUsageType::SelectablesInventory)
	//	{
	//		/* Update inv slot pointer to point to new slot if it was one of the 2 that was swapped.
	//		Do not need to update the server slot index (which I store in PendingContextActionMoreAuxilleryData) */
	//		if (PendingContextActionAuxilleryDataPtr == SlotPtr_1)
	//		{
	//			PendingContextActionAuxilleryDataPtr = SlotPtr_2;
	//		}
	//		else if (PendingContextActionAuxilleryDataPtr == SlotPtr_2)
	//		{
	//			PendingContextActionAuxilleryDataPtr = SlotPtr_1;
	//		}
	//	}
	//}
}

void ARTSPlayerController::OnMarqueeSelect(bool bDidSelectionChange)
{
	if (bDidSelectionChange)
	{
		/* Something was selected from marquee selection */

		assert(Selected[0] != nullptr);

		CurrentSelected = TScriptInterface<ISelectable>(Selected[0]);
		CurrentSelected->GetAttributesBase().SetIsPrimarySelected(true);

		HUD->OnPlayerCurrentSelectedChanged(CurrentSelected);
	}
	else
	{
		/* Same selection as before marquee select */
	}

	// If selected something then try play CurrentSelected's 'selection' sound
	if (HasSelection())
	{
		if (ShouldPlaySelectionSound(CurrentSelected->GetAttributesBase()))
		{
			PlaySelectablesSelectionSound();
		}

		TimeOfLastSelection_Owned = GetWorld()->GetRealTimeSeconds();
	}
}

void ARTSPlayerController::OnPersistentTabButtonLeftClicked(UHUDPersistentTabButton * ButtonWidget)
{
	/* Moved this whole func from UHUDPersistentTabButton::UIBinding_OnClicked() */

	const FContextButton * Button = ButtonWidget->GetButtonType();
	FActiveButtonStateData * StateInfo = ButtonWidget->GetStateInfo();
	assert(Button != nullptr);

	/* For now just handling left mouse button clicks. Cancelling production in progress is not
	allowed currently */

	const bool bShowHUDWarnings = true;

	/* This method does not make buttons HitTestInvisible. Instead it allows users to still
	click the button. We must show a warning message and return if the button is not "active" */
	if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables)
	{
		if (PS->ArePrerequisitesMet(*Button, bShowHUDWarnings) == false
			|| PS->HasQueueSupport(*Button, bShowHUDWarnings) == false)
		{
			return;
		}
	}
	else
	{
		/* Button should not be clickable in the first place if prereqs are not met */
		assert(PS->ArePrerequisitesMet(*Button, false) == true);
		assert(PS->HasQueueSupport(*Button, false) == true);
	}

	/* Currently one reason for UnClickableReason to be be None is that the button is for
	producing a building but we are already producing one with a persistent queue and we
	only allow one building to be produced at a time */
	if (StateInfo->GetCannotFunctionReason() != EGameWarning::None)
	{
		if (bShowHUDWarnings)
		{
			PS->OnGameWarningHappened(StateInfo->GetCannotFunctionReason());
		}

		return;
	}

	/* To be passed by reference */
	ABuilding * BuildingToBuildFrom = nullptr;
	EProductionQueueType QueueType; // Queue type that will produce item. Actually irrelevant 

	if (Button->GetButtonType() == EContextButton::BuildBuilding)
	{
		/* Just allowing one persistent queue to build right now. When production starts what
		we actually do is make all the OTHER BuildBuilding buttons set the CannotFunctionReason
		to BuildingInProgress. However we don't actually set it on the button we clicked so
		we check for that here and show a warning if so */
		if (ButtonWidget->IsProducingSomething())
		{
			PS->OnGameWarningHappened(EGameWarning::BuildingInProgress);
			return;
		}

		const EBuildingType BuildingType = Button->GetBuildingType();

		if (IsPlacingGhost() && BuildingType == GetGhostType())
		{
			/* Ghost building is already being placed and the button clicked on is for the same
			building so do nothing */
			return;
		}
		else
		{
			const FBuildingInfo * BuildInfo = FactionInfo->GetBuildingInfo(BuildingType);
			const EBuildingBuildMethod BuildMethod = BuildInfo->GetBuildingBuildMethod();

			/* It is assumed if the producing building/PlayerState was destroyed then it will have
			notified the HUD */
			if (ButtonWidget->HasProductionCompleted())
			{
				assert(BuildMethod == EBuildingBuildMethod::BuildsInTab);
				SpawnGhostBuilding(BuildingType);
			}
			else
			{
				if (BuildMethod == EBuildingBuildMethod::BuildsInTab)
				{
					if (PS->IsAtSelectableCap(true, bShowHUDWarnings) == false
						&& PS->IsAtUniqueBuildingCap(BuildingType, true, bShowHUDWarnings) == false
						&& PS->HasEnoughResources(BuildingType, bShowHUDWarnings) == true)
					{
						/* This func will handle any warnings if all queues are full */
						PS->GetActorToBuildFrom(*Button, bShowHUDWarnings, BuildingToBuildFrom, QueueType);

						// Check that the queue wasn't full
						if (BuildingToBuildFrom != nullptr)
						{
							assert(QueueType != EProductionQueueType::None);

							if (GetWorld()->IsServer())
							{
								// This func skips performing all the checks we just did
								BuildingToBuildFrom->Server_OnProductionRequest(*Button, QueueType,
									false, nullptr);
							}
							else
							{
								// Send RPC
								BuildingToBuildFrom->Server_OnProductionRequestBuilding(BuildingType, QueueType);
							}
						}
					}
				}
				/* The other 4 build methods are allowed. Their functionality is implemented
				in PC::Server_PlaceBuilding */
				else
				{
					/* This *may* be able to be relaxed, I haven't tried it.
					Should probably check the build types during the whole setup process though
					and just exclude them from the tab if unusable, and show warning log
					message of course */
					UE_CLOG(BuildMethod != EBuildingBuildMethod::BuildsItself, RTSLOG, Fatal,
						TEXT("Building %s for faction %s is on a HUD persistent tab but is its "
							"build method is not one that allows this. Change its build method or "
							"change its persistent tab category to \"None\""),
						TO_STRING(EBuildingType, BuildingType),
						TO_STRING(EFaction, FactionInfo->GetFaction()));

					/* These checks used to be manditory but now are optional based on a project
					setting */
					if (GhostOptions::bPerformChecksBeforeGhostSpawn_SelectableCap == true)
					{
						if (PS->IsAtSelectableCap(true, bShowHUDWarnings))
						{
							return;
						}
					}
					if (GhostOptions::bPerformChecksBeforeGhostSpawn_Uniqueness == true)
					{
						if (PS->IsAtUniqueBuildingCap(BuildingType, true, bShowHUDWarnings) == true)
						{
							return;
						}
					}
					if (GhostOptions::bPerformChecksBeforeGhostSpawn_Resources == true)
					{
						if (PS->HasEnoughResources(BuildingType, bShowHUDWarnings) == false)
						{
							return;
						}
					}
					if (GhostOptions::bPerformChecksBeforeGhostSpawn_UnfullQueue == true)
					{
						/* This func will handle warnings if all queues are full. I guess when we
						call this func all we're really wanting to know is if all queues are full or
						not, because the building we get back doesn't actually get sent to the server
						and we do nothing with it. So for that reason it is wrapped inside this
						if statement */
						PS->GetActorToBuildFrom(*Button, bShowHUDWarnings, BuildingToBuildFrom, QueueType);

						if (BuildingToBuildFrom == nullptr) // If all queues are full
						{
							return;
						}
					}

					/* Do not need to pass on who the ActorToBuildFrom is because we have to
					check again when the ghost is placed so there's no real point passing that
					info on */
					SpawnGhostBuilding(BuildingType);
				}
			}
		}
	}
	else if (Button->GetButtonType() == EContextButton::Train)
	{
		const EUnitType UnitType = Button->GetUnitType();

		if (PS->IsAtSelectableCap(true, bShowHUDWarnings) == false
			&& PS->IsAtUniqueUnitCap(UnitType, true, bShowHUDWarnings) == false
			&& PS->HasEnoughResources(UnitType, bShowHUDWarnings) == true
			&& PS->HasEnoughHousingResources(UnitType, bShowHUDWarnings) == true)
		{
			// This func will handle warnings if all queues are full
			PS->GetActorToBuildFrom(*Button, bShowHUDWarnings, BuildingToBuildFrom, QueueType);

			if (BuildingToBuildFrom != nullptr)
			{
				assert(QueueType != EProductionQueueType::None);

				if (GetWorld()->IsServer())
				{
					// This func skips all the checks we just did
					BuildingToBuildFrom->Server_OnProductionRequest(*Button, QueueType, false, nullptr);
				}
				else
				{
					// Send RPC
					BuildingToBuildFrom->Server_OnProductionRequestUnit(UnitType, QueueType);
				}
			}
		}

		// And another TODO: make sure to look into FContextButton ctor with that other struct 
		// and that that other struct gets constructed correctly from the FContextButton
	}
	else // Assuming upgrade, are upgrade buttons even allowed on persistent panel though?
	{
		assert(Button->GetButtonType() == EContextButton::Upgrade);

		const EUpgradeType UpgradeType = Button->GetUpgradeType();
		const FUpgradeInfo & UpgradeInfo = FactionInfo->GetUpgradeInfoChecked(UpgradeType);

		/* Similar to unit, but need to check with upgrade manager is upgrade is already being
		researched or has already been researched */
		if (PS->GetUpgradeManager()->HasBeenFullyResearched(UpgradeType, bShowHUDWarnings) == false)
		{
			if (PS->GetUpgradeManager()->IsUpgradeQueued(UpgradeType, bShowHUDWarnings) == false)
			{
				if (PS->HasEnoughResources(UpgradeType, bShowHUDWarnings) == true)
				{
					// This func will handle warnings if all queues are full
					PS->GetActorToBuildFrom(*Button, bShowHUDWarnings, BuildingToBuildFrom, QueueType);

					if (BuildingToBuildFrom != nullptr)
					{
						/* Currently upgrades only happen in context queues so check this */
						assert(QueueType == EProductionQueueType::Context);

						if (GetWorld()->IsServer())
						{
							// This func skips all the checks we just did
							BuildingToBuildFrom->Server_OnProductionRequest(*Button, QueueType, false, nullptr);
						}
						else
						{
							// Send RPC
							BuildingToBuildFrom->Server_OnProductionRequestUpgrade(UpgradeType, QueueType);
						}
					}
				}
			}
		}
	}
}

void ARTSPlayerController::OnPersistentTabButtonRightClicked(UHUDPersistentTabButton * Button)
{
	// Behavior for a right click:
	// If the button is producing something then cancel it. Otherwise do nothing
	// TODO

	if (GetWorld()->IsServer())
	{
		if (Button->IsProducingSomething())
		{
			
		}
		else if (Button->HasProductionCompleted())
		{

		}
	}
	else
	{
		if (Button->IsProducingSomething())
		{
			
		}
		else if (Button->HasProductionCompleted())
		{

		}
	}
}

void ARTSPlayerController::OnContextButtonClick(const FContextButton & Button)
{
	if (!IsSelectionControlledByThis())
	{
		return;
	}

	assert(CurrentSelected != nullptr);

	// Check prereqs if required
	if (Button.GetButtonType() == EContextButton::BuildBuilding
		|| Button.GetButtonType() == EContextButton::Train
		|| Button.GetButtonType() == EContextButton::Upgrade)
	{
		const bool bPrereqsMet = PS->ArePrerequisitesMet(Button, true);
		if (!bPrereqsMet)
		{
			return;
		}
	}

	/* Handle the special case where button is to build building */
	if (Button.GetButtonType() == EContextButton::BuildBuilding)
	{	
		const EBuildingType BuildingType = Button.GetBuildingType();
		const FBuildingInfo * BuildingInfo = FactionInfo->GetBuildingInfo(BuildingType);
		
		/* If the buttons ever choose to start updating their clickability/visibility due to 
		some of these things like resources, selectable cap etc then these checks can be changed 
		to asserts since we shouldn't have been able to click the button in the first place */
		if (GhostOptions::bPerformChecksBeforeGhostSpawn_SelectableCap == true)
		{
			if (PS->IsAtSelectableCap(true, true) == true)
			{
				return;
			}
		}
		if (GhostOptions::bPerformChecksBeforeGhostSpawn_Uniqueness == true)
		{
			if (PS->IsAtUniqueBuildingCap(BuildingType, true, true) == true)
			{
				return;
			}
		}
		if (GhostOptions::bPerformChecksBeforeGhostSpawn_Resources == true)
		{
			if (PS->HasEnoughResources(BuildingType, true) == false)
			{
				return;
			}
		}
		
		const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

		/* For now do not expect this behavior */
		assert(BuildMethod != EBuildingBuildMethod::BuildsInTab);

		/* Assuming that the hovered button was the one that was clicked. Probably better 
		performance wise and in general to just pass it as a parameter */
		HighlightButton(HoveredUIElement);

		if (BuildMethod == EBuildingBuildMethod::BuildsItself)
		{
			/* 2nd param 0 to signal 'build from persistent queue' and not from the selectable
			whose context menu this is for */
			SpawnGhostBuilding(BuildingType, 0);
		}
		else
		{
			/* 2nd param is so we will get ordered to work on building if it is placed successfully */
			SpawnGhostBuilding(BuildingType, CurrentSelected->GetSelectableID());
		}

		return;
	}

	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(Button.GetButtonType());

	/* If the ability is the same as the one we have pending then we take it the player 
	wants to cancel it */
	if (Button.GetButtonType() == PendingContextAction 
		&& PendingContextActionUseMethod == EAbilityUsageType::SelectablesActionBar)
	{
		CancelPendingContextCommand();
		return;
	}

	/* TODO I need to make sure that instant abilities will assign PendingContextAction to none 
	otherwise if the player spams instant abilities there 2nd/3rd/4th etc ones would get caught 
	in the if statement above which is not what we want */

	// TODO play sound for command, do not need to worry about BuildBuilding. Remember 
	// FInfantryAttributes has TMap ContextCommandSounds that can be queried for sound to use
	// Doesn't really cover buildings though

	if (AbilityInfo.IsInstant())
	{
		if (GetWorld()->IsServer())
		{
			IssueInstantContextCommand(Button, true);
		}
		else
		{
			/* Whether to send RPC to issue command */
			bool bSendRPC = false;

			AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);

			int32 NumSupported = Selected.Num(); // Start at max and decrement during loop
			int32 NumOnCooldown = 0;
			int32 NumWithoutEnoughSelectableResources = 0;
			EAbilityRequirement FailedCustomChecksType = EAbilityRequirement::Uninitialized; // For showing HUD message

			for (const auto & Elem : Selected)
			{
				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				/* -2.f implies does not support button */
				const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);

				if (CooldownRemaining == -2.f)
				{
					NumSupported--;
				}
				else
				{
					if (CooldownRemaining > 0.f) 
					{
						NumOnCooldown++;
					}
					else
					{
						if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
						{
							NumWithoutEnoughSelectableResources++;

							continue;
						}
						
						if (EffectActor->IsUsable_SelfChecks(AsSelectable, FailedCustomChecksType))
						{
							bSendRPC = true;
							
							/* Knowing at least one unit can do ability is enough info to send RPC */
							break;
						}
					}
				}
			}
			
			if (bSendRPC)
			{
				if (ControlOptions::bInstantActionsCancelPendingOnes)
				{
					/* If there is a pending action then cancel it. We waited until now because
					if the ability is considered unusable then I think it's good we don't cancel
					what is pending. */
					if (IsPlacingGhost())
					{
						CancelGhost();
					}
					else if (IsContextActionPending())
					{
						CancelPendingContextCommand();
					}
					else if (IsGlobalSkillsPanelAbilityPending())
					{
						CancelPendingGlobalSkillsPanelAbility();
					}
				}
				
				assert(Selected.Num() == SelectedIDs.Num());
				
				/* Check if we need to send the selectable IDs because the selectables we are
				issuing orders to may be from the group of the last order */
				bool bSendNewIDs = false;

				if (SelectedIDs.Num() == CommandIDs.Num())
				{
					for (const auto & SelectableID : CommandIDs)
					{
						if (!SelectedIDs.Contains(SelectableID))
						{
							bSendNewIDs = true;
							break;
						}
					}
				}
				else
				{
					bSendNewIDs = true;
				}

				if (bSendNewIDs)
				{
					CommandIDs.Reset();

					FContextCommand Command = FContextCommand(false, Button);

					/* Attach selectables affected by command to it and remember what we sent */
					for (const auto & SelectableID : SelectedIDs)
					{
						Command.AddSelectable(SelectableID);
						CommandIDs.Emplace(SelectableID);
					}

					Server_IssueInstantContextCommand(Command);
				}
				else
				{
					/* Server will use the selectables stored in CommandIDs */
					const FContextCommand Command = FContextCommand(true, Button);
					Server_IssueInstantContextCommand(Command);
				}
			}
			else
			{
				/* If 0 then button should not have been clickable in the first place */
				assert(NumSupported > 0);
				
				/* Did not issue order. Try show reason. */

				if (NumOnCooldown > 0)
				{
					PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
				}
				else if (NumWithoutEnoughSelectableResources > 0)
				{
					PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
				}
				else // Assumed custom checks failed
				{
					PS->OnGameWarningHappened(FailedCustomChecksType);
				}
			}
		}
	}
	else
	{
		/* Context action requires another button press. */

		if (AbilityInfo.IsIssuedToAllSelected())
		{
			/* Loop through all selected. Figure out which units support the action, add them 
			to SelectedAndSupportPendingContextCommand, and also figure out how many have 
			ability off cooldown and also check any custom checks the ability requires now */
			int32 NumSupported = Selected.Num(); // Start at max and decrement during loop
			int32 NumOnCooldown = 0;
			int32 NumWithoutEnoughSelectableResources = 0;
			EAbilityRequirement FailedCustomChecksType = EAbilityRequirement::Uninitialized; // For showing HUD message
			int32 NumThatCanUse = 0; // Number that have passed all the checks for ability
			SelectedAndSupportPendingContextAction.Reset();
			AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);
			for (const auto & Elem : Selected)
			{
				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);
				/* -2.f implies does not support button type */
				if (CooldownRemaining == -2.f)
				{
					NumSupported--;
				}
				else // Supports ability
				{
					SelectedAndSupportPendingContextAction.Emplace(Elem);

					/* If we know at least one selected selectable can use the ability then we
					don't need to keep checking if anymore can. It is purely for HUD message
					purposes */
					if (NumThatCanUse == 0)
					{
						// Check if on cooldown
						if (CooldownRemaining > 0.f)
						{
							NumOnCooldown++;
						}
						else
						{
							if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
							{
								NumWithoutEnoughSelectableResources++;

								continue;
							}
							
							// Check if selectable passes the ability's custom requirements
							if (EffectActor->IsUsable_SelfChecks(AsSelectable, FailedCustomChecksType))
							{
								NumThatCanUse++;
							}
						}
					}
				}
			}

			// Button should not have been clickable in first place if nothing selected supports it
			assert(NumSupported > 0);

			// Sanity check
			assert(NumThatCanUse <= NumSupported);

			if (NumThatCanUse == 0)
			{
				/* Have chosen to use the cooldown message if at least one selectable had it
				on cooldown */
				if (NumOnCooldown > 0)
				{
					PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
				}
				else if (NumWithoutEnoughSelectableResources > 0)
				{
					PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
				}
				else // Assumed custom checks failed 
				{
					/* Technically I think Uninitialized could be passed in here if NumOnCooldown
					overflows */
					PS->OnGameWarningHappened(FailedCustomChecksType);
				}
				
				return;
			}
			else
			{
				/* If there is a pending action then cancel it. We waited until now because
				if the ability is considered unusable then I think it's good we don't cancel
				what is pending */
				if (IsPlacingGhost())
				{
					CancelGhost();
				}
				else if (IsContextActionPending())
				{
					CancelPendingContextCommand();
				}
				else if (IsGlobalSkillsPanelAbilityPending())
				{
					CancelPendingGlobalSkillsPanelAbility();
				}
				
				HighlightButton(HoveredUIElement);

				UpdateMouseAppearance(AbilityInfo);

				/* Let mouse presses know that a context action is
				pending and their behaviour should change */
				PendingContextAction = Button.GetButtonType();
				PendingContextActionUseMethod = EAbilityUsageType::SelectablesActionBar;
				bHasPrimarySelectedChanged = false;
			}
		}
		else
		{
			/* As far as I can tell this code is exactly the same as the IsIssuedToAllSelected 
			case */

			/* Loop through all selected. Figure out which units support the action, add them
			to SelectedAndSupportPendingContextCommand, and also figure out how many have
			ability off cooldown and also check any custom checks the ability requires now */
			int32 NumSupported = Selected.Num(); // Start at max and decrement during loop
			int32 NumOnCooldown = 0;
			int32 NumWithoutEnoughSelectableResources = 0;
			EAbilityRequirement FailedCustomChecksType = EAbilityRequirement::Uninitialized; // For showing HUD message 
			int32 NumThatCanUse = 0; // Number that have passed all the checks for ability
			SelectedAndSupportPendingContextAction.Reset();
			AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);
			for (const auto & Elem : Selected)
			{
				ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);

				const float CooldownRemaining = AsSelectable->GetCooldownRemaining(Button);
				/* -2.f implies does not support button type */
				if (CooldownRemaining == -2.f)
				{
					NumSupported--;
				}
				else // Supports ability
				{
					SelectedAndSupportPendingContextAction.Emplace(Elem);

					/* If we know at least one selected selectable can use the ability then we 
					don't need to keep checking if anymore can. It is purely for HUD message 
					purposes */
					if (NumThatCanUse == 0)
					{
						// Check if on cooldown
						if (CooldownRemaining > 0.f)
						{
							NumOnCooldown++;
						}
						else
						{
							if (AsSelectable->HasEnoughSelectableResources(AbilityInfo) == false)
							{
								NumWithoutEnoughSelectableResources++;

								continue;
							}
							
							// Check if selectable passes the ability's custom requirements
							if (EffectActor->IsUsable_SelfChecks(AsSelectable, FailedCustomChecksType))
							{
								NumThatCanUse++;
							}
						}
					}
				}
			}

			// Button should not have been clickable in first place if nothing selected supports it
			assert(NumSupported > 0);

			// Sanity check
			assert(NumThatCanUse <= NumSupported);

			if (NumThatCanUse == 0)
			{
				/* Have chosen to use the cooldown message if at least one selectable had it 
				on cooldown */
				if (NumOnCooldown > 0)
				{
					PS->OnGameWarningHappened(EGameWarning::ActionOnCooldown);
				}
				else if (NumWithoutEnoughSelectableResources > 0)
				{
					PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
				}
				else // Assumed custom checks failed
				{
					/* Technically I think Uninitialized could be passed in here if NumOnCooldown 
					overflows */
					PS->OnGameWarningHappened(FailedCustomChecksType);
				}
				
				return;
			}
			else
			{
				/* If there is a pending action then cancel it. We waited until now because
				if the ability is considered unusable then I think it's good we don't cancel
				what is pending */
				if (IsPlacingGhost())
				{
					CancelGhost();
				}
				else if (IsContextActionPending())
				{
					CancelPendingContextCommand();
				}
				else if (IsGlobalSkillsPanelAbilityPending())
				{
					CancelPendingGlobalSkillsPanelAbility();
				}
				
				HighlightButton(HoveredUIElement);

				UpdateMouseAppearance(AbilityInfo);

				/* Let mouse presses know that a context action is
				pending and their behaviour should change */
				PendingContextAction = Button.GetButtonType();
				PendingContextActionUseMethod = EAbilityUsageType::SelectablesActionBar;
				bHasPrimarySelectedChanged = false;
			}
		}
	}
}

void ARTSPlayerController::OnGlobalSkillsPanelButtonLeftClicked(UGlobalSkillsPanelButton * ClickedButtonsUserWidget)
{
	/* If the button is for the same ability that is already pending then cancel it */
	if (ClickedButtonsUserWidget->GetAbilityState() == PendingCommanderAbility)
	{
		CancelPendingGlobalSkillsPanelAbility();
		return;
	}
	
	FAquiredCommanderAbilityState * AbilityState = const_cast<FAquiredCommanderAbilityState*>(ClickedButtonsUserWidget->GetAbilityState());
	const FCommanderAbilityInfo * AbilityInfo = AbilityState->GetAbilityInfo();
	
	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(AbilityState, true);
	if (Warning == EGameWarning::None)
	{
		/* If there is a ghost placement, context action or another commander ability already 
		pending then cancel it */
		if (IsPlacingGhost())
		{
			CancelGhost();
		}
		else if (IsContextActionPending())
		{
			CancelPendingContextCommand();
		}
		else if (IsGlobalSkillsPanelAbilityPending())
		{
			UnhighlightHighlightedButton();
			ResetMouseAppearance();
			
			/* It may be better performance to check this first instead of always hiding the 
			player targeting panel. Could actually possibly do something similar with the 
			ResetMouseAppearance call above */
			if (PendingCommanderAbility->GetAbilityInfo()->GetTargetingMethod() == EAbilityTargetingMethod::RequiresPlayer
				&& AbilityInfo->GetTargetingMethod() != EAbilityTargetingMethod::RequiresPlayer)
			{
				HUD->HidePlayerTargetingPanel();
			}
		}

		if (AbilityInfo->GetTargetingMethod() == EAbilityTargetingMethod::DoesNotRequireAnyTarget)
		{
			if (GetWorld()->IsServer())
			{
				/* Execute ability now */
				ExecuteCommanderAbility(AbilityState, *AbilityInfo);
			}
			else
			{
				/* RPC to server requesting using the ability */
				Server_RequestExecuteCommanderAbility(AbilityInfo->GetType());
			}
		}
		else if (AbilityInfo->GetTargetingMethod() == EAbilityTargetingMethod::RequiresPlayer)
		{
			PendingCommanderAbility = AbilityState;
			if (AbilityInfo->GetMouseAppearance() == EAbilityMouseAppearance::CustomMouseCursor)
			{
				SetMouseCursor(AbilityInfo->GetDefaultMouseCursorInfo());
			}
			HighlightButton(HoveredUIElement);

			HUD->ShowPlayerTargetingPanel(AbilityInfo);
		}
		// From here on out assumed one of RequiresSelectable, RequiresWorldLocation or RequiresSelectableOrWorldLocation
		else if (AbilityInfo->GetMouseAppearance() == EAbilityMouseAppearance::CustomMouseCursor)
		{
			PendingCommanderAbility = AbilityState;
			HighlightButton(HoveredUIElement);
			SetMouseCursor(AbilityInfo->GetDefaultMouseCursorInfo());
		}
		else if (AbilityInfo->GetMouseAppearance() == EAbilityMouseAppearance::HideAndShowDecal)
		{
			PendingCommanderAbility = AbilityState;
			HighlightButton(HoveredUIElement);
			SetMouseDecal(AbilityInfo->GetAcceptableLocationDecalInfo());
		}
		else // Assumed EAbilityMouseAppearance::NoChange
		{
			assert(AbilityInfo->GetMouseAppearance() == EAbilityMouseAppearance::NoChange);
			PendingCommanderAbility = AbilityState;
			HighlightButton(HoveredUIElement);
		}
	}
	else
	{
		PS->OnGameWarningHappened(Warning);
	}
}

void ARTSPlayerController::OnPlayerTargetingPanelButtonLeftClicked(ARTSPlayerState * AbilityTarget)
{
	/* Check if the ability is still usable. Currently this checks cooldown and whether
	num uses has been used. Neither of those would have changed since we pressed the global
	skills panel button unless there are abilities that manipulate either. I guess we'll
	check in case there are */
	const EGameWarning Warning = IsGlobalSkillsPanelAbilityUsable(PendingCommanderAbility, AbilityTarget, true);
	if (Warning == EGameWarning::None)
	{
		if (GetWorld()->IsServer())
		{
			/* Hide the player targeting panel */
			HUD->HidePlayerTargetingPanel();

			/* Reset mouse cursor back to normal */
			SetMouseCursor(*ScreenLocationCurrentCursor);
			
			/* This function will probably want to hide the player targeting panel */
			ExecuteCommanderAbility(PendingCommanderAbility, *PendingCommanderAbility->GetAbilityInfo(), AbilityTarget);

			PendingCommanderAbility = nullptr;
		}
		else
		{
			/* Hide the player targeting panel */
			HUD->HidePlayerTargetingPanel();

			/* Reset mouse cursor back to normal */
			SetMouseCursor(*ScreenLocationCurrentCursor);

			/* RPC to server to request use ability */
			Server_RequestExecuteCommanderAbility_PlayerTargeting(PendingCommanderAbility->GetAbilityInfo()->GetType(), AbilityTarget->GetPlayerIDAsInt());

			PendingCommanderAbility = nullptr;
		}
	}
	else
	{
		PS->OnGameWarningHappened(Warning);
	}
}

void ARTSPlayerController::OnInventorySlotButtonLeftClicked(FInventorySlotState & InventorySlot, 
	uint8 SlotsServerIndex)
{
	/* Function written to ignore FContextButtonInfo::bIssuedToAllSelected i.e. item use is 
	always only carried out by the primary selected */
	
	/* Left click = use the ability */

	/* Not sure if this button will be made hit test invisible or not when it cannot be used
	e.g. ran out of item charges. For now we'll assume it is not and therefore have to check
	a lot of things */

	// Shouldn't be possible to use items in selectable's inventories that are not controlled by us
	if (CurrentSelected->GetAttributesBase().GetAffiliation() != EAffiliation::Owned)
	{
		/* Won't show a warning here by choice */
		return;
	}

	const EInventoryItem ItemType = InventorySlot.GetItemType();
	const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(ItemType);
	const EContextButton ItemUseType = ItemsInfo->GetUseAbilityType();

	/* Check if it's the same item that we have pending. If so then cancel it */
	if (PendingContextAction == ItemUseType && PendingContextActionMoreAuxilleryData == SlotsServerIndex)
	{
		CancelPendingContextCommand();
		return;
	}

	if (InventorySlot.HasItem() == false)
	{
		/* Won't show a warning here by choice */
		return;
	}

	if (InventorySlot.IsItemUsable() == false)
	{
		/* Won't show a warning here by choice */
		return;
	}

	if (InventorySlot.GetNumCharges() == 0)
	{
		PS->OnGameWarningHappened(EGameWarning::ItemOutOfCharges);
		return;
	}

	const FContextButtonInfo * AbilityInfo = ItemsInfo->GetUseAbilityInfo();

	/* Check if primary selected can pay selectable resource cost */
	if (CurrentSelected->HasEnoughSelectableResources(*AbilityInfo) == false)
	{
		PS->OnGameWarningHappened(EGameWarning::NotEnoughSelectableResources);
		return;
	}
	
	/* Check if item is on cooldown */
	if (InventorySlot.IsOnCooldown(GetWorldTimerManager()) == true)
	{
		PS->OnGameWarningHappened(EGameWarning::ItemOnCooldown);
		return;
	}

	AAbilityBase * AbilityEffectActor = AbilityInfo->GetEffectActor(GS);
	
	/* Check custom ability checks */
	EAbilityRequirement CannotUseReason;
	if (AbilityEffectActor->IsUsable_SelfChecks(ToSelectablePtr(CurrentSelected), CannotUseReason) == false)
	{
		PS->OnGameWarningHappened(CannotUseReason);
		return;
	}

	/* At this point we can issue the command to the primary selected */
	
	if (AbilityInfo->IsInstant())
	{
		if (ControlOptions::bInstantActionsCancelPendingOnes)
		{
			/* Cancel pending action if there is one */
			if (IsPlacingGhost())
			{
				CancelGhost();
			}
			else if (IsContextActionPending())
			{
				CancelPendingContextCommand();
			}
			else if (IsGlobalSkillsPanelAbilityPending())
			{
				CancelPendingGlobalSkillsPanelAbility();
			}
		}
		
		if (GetWorld()->IsServer())
		{
			IssueInstantUseInventoryItemCommandChecked(InventorySlot, SlotsServerIndex, *ItemsInfo, *AbilityInfo);
		}
		else
		{
			// RPC to server
			Server_IssueInstantUseInventoryItemCommand(CurrentSelected->GetSelectableID(), SlotsServerIndex, ItemType);
		}
	}
	else // Ability requires another mouse click
	{
		/* Cancel pending action if there is one */
		if (IsPlacingGhost())
		{
			CancelGhost();
		}
		else if (IsContextActionPending())
		{
			CancelPendingContextCommand();
		}
		else if (IsGlobalSkillsPanelAbilityPending())
		{
			CancelPendingGlobalSkillsPanelAbility();
		}
		
		SelectedAndSupportPendingContextAction.Reset();
		SelectedAndSupportPendingContextAction.Emplace(Selected[0]);
		
		HighlightButton(HoveredUIElement);

		UpdateMouseAppearance(*AbilityInfo);

		/* Let mouse presses know that a context action is pending and their behaviour should 
		change */
		bHasPrimarySelectedChanged = false;
		PendingContextAction = ItemUseType;
		PendingContextActionUseMethod = EAbilityUsageType::SelectablesInventory;
		PendingContextActionAuxilleryData = static_cast<uint8>(ItemType);
		PendingContextActionMoreAuxilleryData = SlotsServerIndex;
		PendingContextActionAuxilleryDataPtr = &InventorySlot;
	}
}

void ARTSPlayerController::OnInventorySlotButtonRightClicked(UInventoryItemButton * ButtonWidget)
{
	// Behavior I choose for right-click: try sell item to shop if in range

	// Probably want to check if primary selected has not changed during the time between when 
	// we did the RMB press and RMB release TODO

	const FInventoryItemInfo * ItemsInfo = ButtonWidget->GetItemInfo();

	// Check if there's an item in the slot
	if (ItemsInfo != nullptr)
	{
		if (ItemsInfo->CanBeSold())
		{
			UWorld * World = GetWorld();
			if (World->IsServer())
			{
				// Query params will be for our own team + neutrals since neutral shops are expected.
				// We'll make it a rule that you can only sell items to non-hostile shops
				FCollisionObjectQueryParams QueryParams;
				QueryParams.AddObjectTypesToQuery(GS->GetTeamCollisionChannel(PS->GetTeam()));
				QueryParams.AddObjectTypesToQuery(GS->GetNeutralTeamCollisionChannel());

				TArray <FHitResult> NearbySelectables;
				Statics::CapsuleSweep(World, NearbySelectables, Selected[0]->GetActorLocation(),
					QueryParams, InventoryItemOptions::SELL_TO_SHOP_DISTANCE);

				for (const auto & Elem : NearbySelectables)
				{
					// Maybe could incorperate the inventory tag here
					if (Statics::IsAShopThatAcceptsRefunds(Elem.GetActor()))
					{
						/* Behavior I choose here is to sell just one of the item in the stack.
						Could easily change this to sell all the items in the stack instead */
						const int16 NumToSell = 1; // = ButtonWidget->GetInventorySlot()->GetNumInStack();

						// Refund some/all the cost of the item
						PS->GainResources(ItemsInfo->GetDefaultCost(), InventoryItemOptions::REFUND_RATE * NumToSell);

						FInventory * Inventory = CurrentSelected->GetInventoryModifiable();
						// Remove the item from inventory
						Inventory->OnItemSold(Statics::ToSelectablePtr(CurrentSelected),
							ButtonWidget->GetServerSlotIndex(), NumToSell, *ItemsInfo, HUD);

						return;
					}
				}

				// If here we assume no shop was close enough to sell to. Show warning.
				PS->OnGameWarningHappened(EGameWarning::NotInRangeToSellToShop);
			}
			else
			{
				// Check if at least 1 shop is in range to sell to
				if (CurrentSelected->IsAShopInRange())
				{
					// Send RPC
					Server_SellInventoryItem(CurrentSelected->GetSelectableID(),
						ButtonWidget->GetServerSlotIndex(), ItemsInfo->GetItemType());
				}
				else
				{
					PS->OnGameWarningHappened(EGameWarning::NotInRangeToSellToShop);
				}
			}
		}
		else
		{
			// I guess we'll show a warning here
			PS->OnGameWarningHappened(EGameWarning::ItemCannotBeSold);
		}
	}
}

void ARTSPlayerController::OnShopSlotButtonLeftClicked(UItemOnDisplayInShopButton * ButtonWidget)
{
	const FItemOnDisplayInShopSlot * SlotStateInfo = ButtonWidget->GetSlotStateInfo();
	const FInventoryItemInfo * ItemInfo = ButtonWidget->GetItemInfo();
	const uint8 ShopSlotIndex = ButtonWidget->GetShopSlotIndex();

	if (SlotStateInfo->IsForSale() == false)
	{
		PS->OnGameWarningHappened(EGameWarning::ItemNotForSale);
		return;
	}

	if (SlotStateInfo->GetPurchasesRemaining() == 0)
	{
		PS->OnGameWarningHappened(EGameWarning::ItemSoldOut);
		return;
	}

	/* Now begins checking if we have an inventory user in range and that they can accept
	the item into their inventory */

	/* We do not check if our affiliation is ok to purchase from here. That is because we
	make this widget hidden in the first place if that is the case. */

	const TArray<int32> & CostArray = ItemInfo->GetDefaultCost();
	const EResourceType MissingResource = PS->HasEnoughResourcesSpecific(CostArray);
	const bool bHasEnoughResources = (MissingResource == EResourceType::None);

	const TScriptInterface<ISelectable> & CurrSelected = GetCurrentSelected();

	URTSHUD * HUDWidget = GetHUDWidget();

	bool bIsOwnedInventoryOwnerInRange = false;

	EGameWarning CannotObtainReason = EGameWarning::None;

	const FName & LocalPlayersID = PS->GetPlayerID();
	const ETeam LocalPlayersTeam = PS->GetTeam();

	UWorld * World = GetWorld();

	TArray < FHitResult > InRangeOfShop;
	CurrSelected->GetShopAttributes()->GetSelectablesInRangeOfShopOnTeam(World,
		GS, InRangeOfShop,
		FShopInfo::GetShopLocation(Selected[0]),
		LocalPlayersTeam);

	if (World->IsServer())
	{
		for (const auto & Elem : InRangeOfShop)
		{
			AActor * Actor = Elem.GetActor();

			// Possibly need null and/or validity check here.
			if (Statics::HasInventory(Actor) && Statics::IsOwned(Actor, LocalPlayersID)) // HasInventory... will likely need to add tag for this
			{
				bIsOwnedInventoryOwnerInRange = true;

				if (bHasEnoughResources)
				{
					ISelectable * AsSelectable = CastChecked<ISelectable>(Actor);

					CannotObtainReason = GS->Server_TryPutItemInInventory(AsSelectable, false, false,
						PS, *SlotStateInfo, ShopSlotIndex, *ItemInfo, HUDWidget,
						Statics::ToSelectablePtr(CurrSelected));

					if (CannotObtainReason == EGameWarning::None)
					{
						PS->SpendResources(CostArray);

						// 3rd param false means don't update the HUD, but still need to decrement 
						// from the slot state if limited quantity
						CurrSelected->OnItemPurchasedFromHere(ShopSlotIndex, AsSelectable, false);

						/* Update the shown quantity if applicable. The RPC sent
						from Server_TryPutItemInInventory should handle updating it client-side */
						ButtonWidget->OnPurchaseFromHere();

						return;
					}
				}
				else // Not enough resources 
				{
					PS->OnGameWarningHappened(MissingResource);
					return;
				}
			}
		}

		if (bIsOwnedInventoryOwnerInRange)
		{
			/* This will just be the most recent warning, whatever that may be.

			Without doing CannotObtainReason = None at declaration I get potentially uninitialized
			warning here. 99% sure what I wrote is ok though */
			PS->OnGameWarningHappened(CannotObtainReason);
		}
		else
		{
			PS->OnGameWarningHappened(EGameWarning::NotInRangeToPurchaseFromShop);
		}
	}
	else
	{
		/* We don't actually send the selectable that we want to give the item to. We could,
		and it would save the server some work. It would actually save quite a bit since the
		server would possibly be able to remove some channels for who it checks overlaps for.
		So when we request a selectable to get an item the server would just use a quick
		distance check to see if they are in range. */

		for (const auto & Elem : InRangeOfShop)
		{
			AActor * Actor = Elem.GetActor();

			// Possibly need null and/or validity check here
			if (Statics::HasInventory(Actor) && Statics::IsOwned(Actor, LocalPlayersID))
			{
				bIsOwnedInventoryOwnerInRange = true;

				if (bHasEnoughResources)
				{
					ISelectable * AsSelectable = CastChecked<ISelectable>(Actor);
					const FInventory * Inventory = AsSelectable->GetInventory();

					CannotObtainReason = Inventory->CanItemEnterInventory(SlotStateInfo->GetItemType(),
						SlotStateInfo->GetQuantityBoughtPerPurchase(), *ItemInfo, AsSelectable);

					// If selectable can accept item into their inventory...
					if (CannotObtainReason == EGameWarning::None)
					{
						AActor * ShopAsActor = Selected[0];

						/* This RPC just sends the building we want to buy from and the slot number
						of the item we want to buy. The server will decide what the item is we are
						buying and who gets it */
						Server_RequestBuyItemFromShop(ShopAsActor, ShopSlotIndex);

						return;
					}
				}
				else // Not enough resources
				{
					PS->OnGameWarningHappened(MissingResource);
					return;
				}
			}
		}

		if (bIsOwnedInventoryOwnerInRange)
		{
			/* This will just be the most recent warning, whatever that may be */
			PS->OnGameWarningHappened(CannotObtainReason);
		}
		else
		{
			PS->OnGameWarningHappened(EGameWarning::NotInRangeToPurchaseFromShop);
		}
	}
}

void ARTSPlayerController::UnhoverHoveredUIElement()
{
	if (HoveredUIElement != nullptr)
	{
		HoveredUIElement->ForceUnhover();
	}
}

bool ARTSPlayerController::ShouldIgnoreMouseHoverEvents() const
{
	/* If the mouse cursor isn't showing then ignore button up/down events */
	return !bShowMouseCursor;
}

bool ARTSPlayerController::ShouldIgnoreButtonUpOrDownEvents() const
{
	/* If the mouse cursor isn't showing then ignore button up/down events */
	return !bShowMouseCursor;
}

bool ARTSPlayerController::OnButtonHovered(UMyButton * HoveredWidget, const FGeometry & HoveredWidgetGeometry, 
	const FSlateBrush * HoveredBrush, bool bUsingUnifiedHoverImage, UInGameWidgetBase * UserWidget)
{	
	if (ShouldIgnoreMouseHoverEvents())
	{
		return false;
	}

	HoveredUIElement = HoveredWidget;

	if (bUsingUnifiedHoverImage) 
	{
		ShowUIButtonHoveredImage(HoveredWidget, HoveredWidgetGeometry, HoveredBrush);
	}

#if INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS

	// Show tooltip widget
	if (UserWidget == nullptr)
	{
		HUD->ShowTooltipWidget(HoveredWidget, HoveredWidget->GetPurpose());
	}
	else
	{
		HUD->ShowTooltipWidget(UserWidget, HoveredWidget->GetPurpose());
	}

#else 
	
	HoveredUserWidget = UserWidget;

	// Apply the penalty to accumulated hover time for switching hovered element
	AccumulatedTimeSpentHoveringUIElement -= TooltipOptions::UI_ELEMENT_HOVER_TIME_SWITCH_PENALTY;
	
	AccumulatedTimeSpentHoveringUIElement = FMath::Max<float>(0.f, AccumulatedTimeSpentHoveringUIElement);

#endif

	return true;
}

void ARTSPlayerController::OnButtonUnhovered(UMyButton * UnhoveredWidget, bool bUsingUnifiedHoverImage, 
	UInGameWidgetBase * UserWidget)
{
	HoveredUIElement = nullptr;
	
	if (bUsingUnifiedHoverImage)
	{
		/* Might want to check here that the mouse isn't already hovering another widget. I don't
		know exactly how the slate system works but I assume that when in a single frame the mouse
		unhovers one widget and now hovers another that the unhover is called first then the hovered
		and not the other way round. I should confirm this by checking source but for now I will
		just assume that it's always unhovered followed by hovered (which could lead to bad things
		happening - check source) */
		HUD->GetMouseFocusWidget()->SetVisibility(ESlateVisibility::Hidden);
	}

	// Hide tooltip widget
	if (UserWidget == nullptr)
	{
		HUD->HideTooltipWidget(UnhoveredWidget, UnhoveredWidget->GetPurpose());
	}
	else
	{
		HUD->HideTooltipWidget(UserWidget, UnhoveredWidget->GetPurpose());
	}

#if !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS

	bShowingUIElementTooltip = false;
	AccumulatedTimeSpentHoveringUIElement = FMath::Min<float>(AccumulatedTimeSpentHoveringUIElement, TooltipOptions::UI_ELEMENT_HOVER_TIME_REQUIREMENT);

#if USING_UI_ELEMENT_HOVER_TIME_DECAY_DELAY

	TimeSpentNotHoveringUIElement = 0.f;

#endif

#endif // !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS
}

void ARTSPlayerController::ShowUIButtonHoveredImage(UMyButton * HoveredWidget, 
	const FGeometry & HoveredWidgetGeometry, const FSlateBrush * HoveredBrush)
{
	/* From testing it appears that only buttons that have already been pressed can be have
	their OnHovered called. So if this is called and we're already showing the 'on pressed'
	image then we should not do anything and we know it must be the already pressed button */
	if (!bShowingUIButtonPressedImage)
	{
		UCanvasPanelSlot * MouseFocusSlot = HUD->GetMouseFocusWidgetSlot();

		/* Going to teleport a UImage widget overtop of the hovered widget to simulate the
		'on hovered' image. TODO get this working */
		//MouseFocusSlot->SetAnchors(FAnchors(0.f, 0.f)); // Need to use right values for these 3
		//MouseFocusSlot->SetPosition();
		//MouseFocusSlot->SetSize();
		// Maybe might need to set ZOrder here

		UImage * MouseFocusWidget = HUD->GetMouseFocusWidget();

		/* Set the image to the hovered image */
		MouseFocusWidget->SetBrush(*HoveredBrush);

		MouseFocusWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void ARTSPlayerController::ShowUIButtonPressedImage_Mouse(SMyButton * PressedButtonWidget,
	const FSlateBrush * PressedBrush)
{
	/* Going to make an assumption here that it is impossible to press a button with the mouse 
	without it first being hovered. If this assumption proves to be false then we first need to 
	teleport the widget to the pressed widget's location just like in ShowUIButtonHoveredImage 
	and also probably make it unhidden */
	
	HUD->GetMouseFocusWidget()->SetBrush(*PressedBrush);

	bShowingUIButtonPressedImage = true;
}

void ARTSPlayerController::HideUIButtonPressedImage_Mouse(UMyButton * ButtonWidget,
	const FGeometry & HoveredWidgetGeometry, const FSlateBrush * HoveredBrush, bool bUsingUnifiedHoverImage)
{
	bShowingUIButtonPressedImage = false;
	
	if (ButtonWidget->IsHovered())
	{
		// Show the hovered image but only if the widget uses a unified hover image
		if (bUsingUnifiedHoverImage)
		{
			ShowUIButtonHoveredImage(ButtonWidget, HoveredWidgetGeometry, HoveredBrush);
		}
	}
	else
	{
		/* Mouse is now hovering over another widget/nothing. But the widget's OnHovered won't 
		be called until this release happens. So here we just hide the widget. If there is 
		another widget hovered by the mouse then it will call its on hovered logic right 
		after this function and that will make the hover effect for it */
		HUD->GetMouseFocusWidget()->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ARTSPlayerController::OnMenuWarningHappened(EGameWarning WarningType)
{
	// Possibly show a message on screen and play sound
	if (ShouldShowMenuWarningMessage(WarningType))
	{
		/* Branch is to check whether in match or not */
		if (HUD != nullptr)
		{
			HUD->OnMenuWarningHappened(WarningType);
		}
		else
		{
			// TODO assumed in main menu. Tell something about warning so it can display it
		}
	}
}

bool ARTSPlayerController::ShouldShowMenuWarningMessage(EGameWarning WarningType)
{
	// Always show, but a better implementation may be to throttle them
	return true;
}

void ARTSPlayerController::PlayCameraShake_Implementation(TSubclassOf<UCameraShake> CameraShakeBP, const FVector & Epicenter, float OuterRadius, float Falloff)
{
	/* Ensure we didn't actually send an RPC. Remove this when the need to
	send camera shake RPCs arrises */
	assert(!GetWorld()->IsServer() || (GetWorld()->IsServer() && this == GetWorld()->GetFirstPlayerController()));
	assert(CameraShakeBP != nullptr);

	/* This and only using 2D coords is the only thing different from the default
	implementation */
	const FVector2D Epicenter2D = FVector2D(Epicenter);
	const FVector2D LocCameraPointedAt = FVector2D(GetPawn()->GetRootComponent()->GetComponentLocation());

	float DistPct = ((Epicenter2D - LocCameraPointedAt).Size()) / OuterRadius;
	DistPct = 1.f - FMath::Clamp(DistPct, 0.f, 1.f);
	const float RadialShakeScale = FMath::Pow(DistPct, Falloff);

	PlayerCameraManager->PlayCameraShake(CameraShakeBP, RadialShakeScale);
}

void ARTSPlayerController::Server_RequestBuyItemFromShop_Implementation(AActor * Shop, uint8 ShopSlotIndex)
{
	if (Statics::IsValid(Shop))
	{
		ISelectable * ShopAsSelectable = CastChecked<ISelectable>(Shop);
		const FShopInfo * ShopInfo = ShopAsSelectable->GetShopAttributes();

		/* Do some checks because modified clients */

		// This check here could definently be in the _Validate version
		if (ShopInfo == nullptr)
		{
			return;
		}
		
		// Check player's affiliation is ok to shop here. Another thing that could go in _Validate
		const EAffiliation ShopAffiliation = (ShopAsSelectable->Selectable_GetPS() == PS)
			? EAffiliation::Owned : ShopAsSelectable->Selectable_GetPS()->IsAllied(PS)
			? EAffiliation::Allied : EAffiliation::Hostile;
		if (ShopInfo->CanShopHere(ShopAffiliation) == false)
		{
			return;
		}

		const FItemOnDisplayInShopSlot & ShopSlot = ShopInfo->GetSlotInfo(ShopSlotIndex);

		// Verify item is for sale
		if (ShopSlot.IsForSale() == false)
		{
			return;
		}

		// Verify item has purchases remaining
		if (ShopSlot.GetPurchasesRemaining() == 0)
		{
			PS->OnGameWarningHappened(EGameWarning::ItemSoldOut);
			return;
		}

		const FInventoryItemInfo * ItemInfo = GI->GetInventoryItemInfo(ShopSlot.GetItemType());

		const TArray < int32 > & CostArray = ItemInfo->GetDefaultCost();
		const EResourceType MissingResource = PS->HasEnoughResourcesSpecific(CostArray);
		const bool bHasEnoughResources = (MissingResource == EResourceType::None);

		bool bIsOwnedInventoryOwnerInRange = false;
		EGameWarning CannotObtainReason = EGameWarning::None;

		/* Pick the selectable to give the item to. This logic should follow the same routine
		as UItemOnDisplayInShopButton::UIBinding_OnButtonClicked server branch to ensure both
		server and clients have the exact same experience */
		TArray < FHitResult > InRangeOfShop;
		ShopInfo->GetSelectablesInRangeOfShopOnTeam(GetWorld(), GS, InRangeOfShop, 
			FShopInfo::GetShopLocation(Shop), PS->GetTeam());

		for (const auto & Elem : InRangeOfShop)
		{
			AActor * Actor = Elem.GetActor();

			// Possibly need null and/or validity check here.
			if (Statics::HasInventory(Actor) && IsControlledByThis(Actor))
			{
				bIsOwnedInventoryOwnerInRange = true;

				if (bHasEnoughResources)
				{
					ISelectable * AsSelectable = CastChecked<ISelectable>(Actor);

					CannotObtainReason = GS->Server_TryPutItemInInventory(AsSelectable, AsSelectable->GetAttributesBase().IsSelected(),
						AsSelectable->GetAttributesBase().IsPrimarySelected(), PS, ShopSlot, ShopSlotIndex,
						*ItemInfo, HUD, ShopAsSelectable);

					if (CannotObtainReason == EGameWarning::None)
					{
						/* Successfully gave item to selectable */

						PS->SpendResources(CostArray);

						/* 3rd param as true means update the HUD */
						ShopAsSelectable->OnItemPurchasedFromHere(ShopSlotIndex, AsSelectable, true);

						return;
					}
				}
				else
				{
					PS->OnGameWarningHappened(MissingResource);
					return;
				}
			}
		}

		// Did not successfully purchase item, give out warning
		if (bIsOwnedInventoryOwnerInRange)
		{
			PS->OnGameWarningHappened(CannotObtainReason);
		}
		else
		{
			PS->OnGameWarningHappened(EGameWarning::NotInRangeToPurchaseFromShop);
		}
	}
	else
	{
		PS->OnGameWarningHappened(EGameWarning::NotValid_Shop);
	}
}

void ARTSPlayerController::Server_SellInventoryItem_Implementation(uint8 SellerID, 
	uint8 InventorySlotIndex, EInventoryItem ItemInSlot)
{
	// Lots of modified client checks should be done here too

	AActor * Selectable = PS->GetSelectableFromID(SellerID);

	if (Statics::IsValid(Selectable))
	{
		if (!Statics::HasZeroHealth(Selectable))
		{
			ISelectable * TypedSelectable = CastChecked<ISelectable>(Selectable);
			FInventory * Inventory = TypedSelectable->GetInventoryModifiable();
			const FInventorySlotState & InvSlot = Inventory->GetSlotGivenServerIndex(InventorySlotIndex);

			// Check that the item type is what the client thought it was
			if (InvSlot.GetItemType() == ItemInSlot)
			{
				// Perform sweep to check if any shops are within range
				
				FCollisionObjectQueryParams QueryParams;
				QueryParams.AddObjectTypesToQuery(GS->GetTeamCollisionChannel(PS->GetTeam()));
				QueryParams.AddObjectTypesToQuery(GS->GetNeutralTeamCollisionChannel());

				TArray <FHitResult> NearbySelectables;
				Statics::CapsuleSweep(GetWorld(), NearbySelectables, Selectable->GetActorLocation(),
					QueryParams, InventoryItemOptions::SELL_TO_SHOP_DISTANCE);

				for (const auto & Elem : NearbySelectables)
				{
					if (Statics::IsAShopThatAcceptsRefunds(Elem.GetActor()))
					{
						const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(ItemInSlot);
						
						const int16 NumToSell = 1;

						// Refund some/all the cost of the item
						PS->GainResources(ItemsInfo->GetDefaultCost(), InventoryItemOptions::REFUND_RATE * NumToSell);

						// Remove the item from inventory
						Inventory->OnItemSold(TypedSelectable, InventorySlotIndex, NumToSell, 
							*ItemsInfo, HUD);

						// Broadcast event to each client
						GS->Multicast_OnInventoryItemSold(FSelectableIdentifier(TypedSelectable),
							InventorySlotIndex);

						return;
					}
				}
				
				// If here then we no shop was in range to sell to
				PS->OnGameWarningHappened(EGameWarning::NotInRangeToSellToShop);
			}
			else
			{
				PS->OnGameWarningHappened(EGameWarning::ItemNoLongerInInventory);
			}
		}
		else
		{
			PS->OnGameWarningHappened(EGameWarning::UserNoLongerAlive);
		}
	}
	else
	{
		PS->OnGameWarningHappened(EGameWarning::UserNoLongerAlive);
	}
}

void ARTSPlayerController::PauseGameAndShowPauseMenu()
{
	/* Could currently be the edge scrolling cursor or an ability could be pending so set 
	the mouse cursor to default one */
	SetMouseCursor(*DefaultCursor);
	
	HUD->ShowPauseMenu();
}

void ARTSPlayerController::ResumePlay()
{
	HUD->HidePauseMenu();

	/* If a development action is pending then the mouse cursor then we should restore the 
	cursor here */
}

bool ARTSPlayerController::SetGamePaused(const UObject * WorldContextObject, bool bPaused)
{
	return UGameplayStatics::SetGamePaused(WorldContextObject, bPaused);
}

void ARTSPlayerController::OnLevelUp_LastForEvent(uint8 OldRank, uint8 NewRank, int32 NumUnspentSkillPoints, 
	float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank)
{
	/* Update the HUD */
	HUD->OnCommanderRankGained_LastForEvent(OldRank, NewRank, NumUnspentSkillPoints, 
		ExperienceTowardsNextRank, ExperienceRequiredForNextRank);
}

void ARTSPlayerController::Server_RequestAquireCommanderSkill_Implementation(uint8 AllNodesArrayIndex)
{
	// Version of function that takes an array index
	const EGameWarning Warning = PS->CanAquireCommanderAbility(AllNodesArrayIndex, true);
	if (Warning == EGameWarning::None)
	{
		PS->AquireNextRankForCommanderAbility(AllNodesArrayIndex, false);
	}
}

bool ARTSPlayerController::IsPlacingGhost() const
{
	return PendingContextAction == EContextButton::PlacingGhost;
}

bool ARTSPlayerController::IsMarqueeActive() const
{
	return bIsMarqueeActive;
}

void ARTSPlayerController::SetupCameraReferences()
{
	MatchCamera = CastChecked<APlayerCamera>(GetPawn());

	SpringArm = MatchCamera->GetSpringArm();
	assert(SpringArm != nullptr);
}

EBuildingType ARTSPlayerController::GetGhostType() const
{
	return GhostBuilding->GetType();
}

ETeam ARTSPlayerController::GetTeam() const
{
	return Team;
}

ARTSPlayerState * ARTSPlayerController::GetPS() const
{
	return PS;
}

void ARTSPlayerController::SetPS(ARTSPlayerState * NewPlayerState)
{
	assert(NewPlayerState != nullptr);
	PS = NewPlayerState;
}

ARTSGameState * ARTSPlayerController::GetGS() const
{
	assert(GS != nullptr);
	return GS;
}

void ARTSPlayerController::SetGS(ARTSGameState * InGameState)
{
	GS = InGameState;
}

AFactionInfo * ARTSPlayerController::GetFactionInfo() const
{
	return FactionInfo;
}

URTSGameInstance * ARTSPlayerController::GetGI() const
{
	return GI;
}

TArray <AActor *> & ARTSPlayerController::GetSelected()
{
	return Selected;
}

TScriptInterface<ISelectable> ARTSPlayerController::GetCurrentSelected() const
{
	/* TODO: 
	I call this function a lot. FSelectableAttributesBase::IsCurrentSelected() may be able 
	to replace many of the times where I call this */
	
	return CurrentSelected;
}

AObjectPoolingManager * ARTSPlayerController::GetPoolingManager() const
{
	return PoolingManager;
}

AFogOfWarManager * ARTSPlayerController::GetFogOfWarManager() const
{
	return FogOfWarManager;
}

URTSHUD * ARTSPlayerController::GetHUDWidget() const
{
	return HUD;
}

bool ARTSPlayerController::IsSelected(uint8 SelectableID) const
{
	return SelectedIDs.Contains(SelectableID);
}

void ARTSPlayerController::AddToSelectedIDs(uint8 NewID)
{
	assert(NewID != 0);

	SelectedIDs.Emplace(NewID);
}

void ARTSPlayerController::RemoveFromSelectedIDs(uint8 IDToRemove)
{
	/* Have relaxed this assert because when selecting hostile selectables we do not add their 
	ID and in marquee HUD logic this will fire. */
	//assert((IDToRemove != 0 && SelectedIDs.Contains(IDToRemove)) 
	//	|| (IDToRemove == 0 && !SelectedIDs.Contains(IDToRemove)))
	
	// Legacy asserts that were here before AInventoryItem actors were implemented
	//assert(IDToRemove != 0);
	//assert(SelectedIDs.Contains(IDToRemove)); // Recently added

	SelectedIDs.Remove(IDToRemove);
}

bool ARTSPlayerController::IsObserver() const
{
	return Type == EPlayerType::Observer;
}

void ARTSPlayerController::FailedToSpawnPawn()
{
	Super::FailedToSpawnPawn();

	UE_LOG(RTSLOG, Warning, TEXT("Failed to spawn pawn"));
}


//-------------------------------------------------
//	End of Match
//-------------------------------------------------

void ARTSPlayerController::DisableHUD()
{
	GetWidget(EMatchWidgetType::HUD)->SetVisibility(ESlateVisibility::HitTestInvisible);
}


//-------------------------------------------------
//	In Match Messaging
//-------------------------------------------------

bool ARTSPlayerController::IsChatInputOpen() const
{
	return HUD->IsChatBoxShowing();
}

void ARTSPlayerController::OpenChatInput(EMessageRecipients MessageRecipients)
{
	HUD->OpenChatBox(MessageRecipients);
}

void ARTSPlayerController::CloseChatInput()
{
	HUD->CloseChatBox();
}

void ARTSPlayerController::Server_SendInMatchChatMessageToEveryone_Implementation(const FString & Message)
{
	GS->SendInMatchChatMessageToEveryone(PS, Message);
}

void ARTSPlayerController::Server_SendInMatchChatMessageToTeam_Implementation(const FString & Message)
{
	GS->SendInMatchChatMessageToTeam(PS, Message);
}

/* Maybe want to add some strange unallowed characters on to this so users cannot spoof being
unknown */
const FString ARTSPlayerController::UNKNOWN_PLAYER_NAME = "Unknown";

void ARTSPlayerController::Client_OnAllChatInMatchChatMessageReceived_Implementation(ARTSPlayerState * Sender,
	const FString & Message)
{
	FString SendersName;

	if (Statics::IsValid(Sender))
	{
		SendersName = Sender->GetPlayerName();
	}
	else
	{

		SendersName = UNKNOWN_PLAYER_NAME;
	}

	/* Let HUD know so it can display message */
	HUD->OnChatMessageReceived(SendersName, Message, EMessageRecipients::Everyone);

	/* Possibly play a sound */
	if (ShouldPlayChatMessageReceivedSound(EMessageRecipients::Everyone))
	{
		PlayChatMessageReceivedSound(EMessageRecipients::Everyone);
	}
}

void ARTSPlayerController::Client_OnTeamChatInMatchChatMessageReceived_Implementation(ARTSPlayerState * Sender, const FString & Message)
{
	FString SendersName;

	if (Statics::IsValid(Sender))
	{
		SendersName = Sender->GetPlayerName();
	}
	else
	{
		SendersName = UNKNOWN_PLAYER_NAME;
	}

	/* Let HUD know so it can display message */
	HUD->OnChatMessageReceived(SendersName, Message, EMessageRecipients::TeamOnly);

	/* Possibly play a sound */
	if (ShouldPlayChatMessageReceivedSound(EMessageRecipients::TeamOnly))
	{
		PlayChatMessageReceivedSound(EMessageRecipients::TeamOnly);
	}
}


//===============================================================================================
//	Lobby
//===============================================================================================

uint8 ARTSPlayerController::GetLobbySlotIndex() const
{
	return LobbySlotIndex;
}

void ARTSPlayerController::SetLobbySlotIndexForHost()
{
	LobbySlotIndex = 0;
}

void ARTSPlayerController::Client_OnTryJoinButLobbyFull_Implementation()
{
	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());
	
	UGameMessageWidget * ErrorWidget = GI->GetWidget<UGameMessageWidget>(EWidgetType::GameMessage);
	ErrorWidget->SetMessage(FText::FromString("Lobby is full"));

	GI->ShowWidget(EWidgetType::GameMessage, ESlateVisibility::HitTestInvisible);
}

void ARTSPlayerController::Client_ShowLobbyWidget_Implementation(uint8 OurLobbySlot, EFaction OurDefaultFaction)
{
	LobbySlotIndex = OurLobbySlot;

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());

	GI->ShowWidget(EWidgetType::Lobby, ESlateVisibility::Collapsed);

	//a// And lobby interactivability? Does something need to be done about it? TODO?
}

void ARTSPlayerController::Server_ChangeTeamInLobby_Implementation(ETeam NewTeam)
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	GS->ChangeTeamInLobby(this, NewTeam);
}

void ARTSPlayerController::Server_ChangeFactionInLobby_Implementation(EFaction NewFaction)
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	GS->ChangeFactionInLobby(this, NewFaction);
}

void ARTSPlayerController::Server_ChangeStartingSpotInLobby_Implementation(int16 PlayerStartID)
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	GS->ChangeStartingSpotInLobby(this, PlayerStartID);
}

void ARTSPlayerController::Client_OnChangeTeamInLobbySuccess_Implementation(ETeam NewTeam)
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GS->SetTeamFromServer(this, NewTeam);
}

void ARTSPlayerController::Client_OnChangeFactionInLobbySuccess_Implementation(EFaction NewFaction)
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GS->SetFactionFromServer(this, NewFaction);
}

void ARTSPlayerController::Client_OnChangeStartingSpotInLobbySuccess_Implementation(int16 PlayerStartID)
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GS->SetStartingSpotFromServer(this, PlayerStartID);
}

void ARTSPlayerController::Client_OnChangeTeamInLobbyFailed_Implementation(ETeam TeamServerSide)
{
	// Not expected to happen on server
	assert(!GetWorld()->IsServer());

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GS->SetTeamFromServer(this, TeamServerSide);

	// TODO maybe show apopup widget saying team change failed
}

void ARTSPlayerController::Client_OnChangeFactionInLobbyFailed_Implementation(EFaction FactionServerSide)
{
	assert(!GetWorld()->IsServer());

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GS->SetFactionFromServer(this, FactionServerSide);

	// TODO maybe show a popup widget saying faction change failed
}

void ARTSPlayerController::Client_OnChangeStartingSpotInLobbyFailed_Implementation(int16 PlayerStartID)
{
	assert(!GetWorld()->IsServer());
	
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GS->SetStartingSpotFromServer(this, PlayerStartID);
}

void ARTSPlayerController::Server_SendLobbyChatMessage_Implementation(const FString & Message)
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	PS = CastChecked<ARTSPlayerState>(PlayerState);

	GS->Multicast_SendLobbyChatMessage(PS, Message);
}


//=============================================================================================
//	Match Initialization
//=============================================================================================

void ARTSPlayerController::Server_AckPSSetupOnClient_Implementation()
{
	/* Can't call RPC directly on GS because ownership */
	GS->Server_AckInitialValuesReceived();
}

void ARTSPlayerController::Server_AckLevelStreamedIn_Implementation()
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GS->AckLevelStreamingComplete();
}


//==============================================================================================
//	Controls Settings
//==============================================================================================

void ARTSPlayerController::SetCameraKeyboardMoveSpeed(float NewValue)
{
	CameraMoveSpeed = NewValue;
}

void ARTSPlayerController::SetCameraMouseMoveSpeed(float NewValue)
{
	CameraEdgeMoveSpeed = NewValue;
}

void ARTSPlayerController::SetCameraMaxSpeed(float NewValue)
{
	if (GetPawn() != nullptr)
	{
		UPlayerCameraMovement * PawnMovement = CastChecked<UPlayerCameraMovement>(GetPawn()->GetMovementComponent());
		PawnMovement->SetMaxSpeed(NewValue);
	}
}

void ARTSPlayerController::SetCameraAcceleration(float NewValue)
{
	if (GetPawn() != nullptr)
	{
		UPlayerCameraMovement * PawnMovement = CastChecked<UPlayerCameraMovement>(GetPawn()->GetMovementComponent());
		PawnMovement->SetAcceleration(NewValue);
	}
}

void ARTSPlayerController::SetEnableCameraMovementLag(bool bIsEnabled)
{
	if (SpringArm != nullptr)
	{
		SpringArm->bEnableCameraLag = bIsEnabled;
	}
}

void ARTSPlayerController::SetCameraMovementLagSpeed(float NewValue)
{
	if (SpringArm != nullptr)
	{
		/* bEnableCameraLag also needs to be true for this to have any chance of working */
		
		SpringArm->CameraLagSpeed = NewValue;
	}
}

void ARTSPlayerController::SetCameraTurningBoost(float NewValue)
{
	if (GetPawn() != nullptr)
	{
		UPlayerCameraMovement * PawnMovement = CastChecked<UPlayerCameraMovement>(GetPawn()->GetMovementComponent());
		PawnMovement->SetTurningBoost(NewValue);
	}
}

void ARTSPlayerController::SetCameraDeceleration(float NewValue)
{
	if (GetPawn() != nullptr)
	{
		UPlayerCameraMovement * PawnMovement = CastChecked<UPlayerCameraMovement>(GetPawn()->GetMovementComponent());
		PawnMovement->SetDeceleration(NewValue);
	}
}

void ARTSPlayerController::SetCameraEdgeMovementThreshold(float NewValue)
{
	CameraEdgeThreshold = NewValue;
}

void ARTSPlayerController::SetCameraZoomIncrementalAmount(float NewValue)
{
	CameraZoomIncrementalAmount = NewValue;
}

void ARTSPlayerController::SetCameraZoomSpeed(float NewValue)
{
	CameraZoomSpeed = NewValue;
}

void ARTSPlayerController::SetMMBLookYawSensitivity(float NewValue)
{
	InputYawScale = NewValue;
}

void ARTSPlayerController::SetMMBLookPitchSensitivity(float NewValue)
{
	InputPitchScale = NewValue;
}

void ARTSPlayerController::SetInvertMMBYawLook(bool bInvert)
{
	const bool bChange = (bInvert && InputYawScale < 0.f) || (!bInvert && InputYawScale > 0.f);

	if (bChange)
	{
		InputYawScale *= -1.f;
	}
}

void ARTSPlayerController::SetInvertMMBPitchLook(bool bInvert)
{
	const bool bChange = (bInvert && InputPitchScale < 0.f) || (!bInvert && InputPitchScale > 0.f);

	if (bChange)
	{
		InputPitchScale *= -1.f;
	}
}

void ARTSPlayerController::SetEnableMMBLookLag(bool bEnableLag)
{
	// Can be null when doing this from GI::Initialize
	if (SpringArm != nullptr)
	{
		SpringArm->bEnableCameraRotationLag = bEnableLag;
	}
}

void ARTSPlayerController::SetMMBLookLagAmount(float NewValue)
{
	if (SpringArm != nullptr)
	{
		SpringArm->CameraRotationLagSpeed = NewValue;
	}
}

void ARTSPlayerController::SetMouseMovementThreshold(float NewValue)
{
	MouseMovementThreshold = NewValue;
}

void ARTSPlayerController::SetDefaultCameraYaw(float NewValue)
{
	DefaultCameraRotation = FRotator(DefaultCameraRotation.Pitch, NewValue,
		DefaultCameraRotation.Roll);

	StartResetRotation = DefaultCameraRotation;
	if (StartResetRotation.Yaw >= 180.f)
	{
		/* This helps the lerp in tick always choose fastest direction instead of always going
		counter clockwise */
		StartResetRotation -= FRotator(0.f, 360.f, 0.f);
	}
	TargetResetRotation = DefaultCameraRotation;

	// Reset camera to default straight away	
	SetControlRotation(DefaultCameraRotation);

	/* Update the ghost pool */
	for (const auto & Pair : GhostPool)
	{
		AGhostBuilding * Ghost = Pair.Value;
		Ghost->SetDefaultRotation(NewValue);
	}
}

void ARTSPlayerController::SetDefaultCameraPitch(float NewValue)
{
	/* Only adjust spring arms relative rot, do nothing with control rot. Control rot only
	handles yaw */
	if (SpringArm != nullptr)
	{
		// Reset camera to default straight away	
		SpringArm->SetRelativeRotation(FRotator(-NewValue, 0.f, 0.f));
	}

	/* Need to do something else. MinCameraPitch and MaxCameraPitch need to take into account
	the pitch just applied. Looking down means negative pitch so these are reversed */
	PlayerCameraManager->ViewPitchMin = NewValue - URTSGameUserSettings::MAX_CAMERA_PITCH;
	PlayerCameraManager->ViewPitchMax = NewValue - URTSGameUserSettings::MIN_CAMERA_PITCH;
}

void ARTSPlayerController::SetDefaultCameraZoomAmount(float NewValue)
{
	DefaultCameraZoomAmount = NewValue;

	if (SpringArm != nullptr)
	{
		// Reset camera to default straight away
		TargetZoomAmount = DefaultCameraZoomAmount;
		MouseWheelZoomCurveAccumulatedTime = 0.f;
		SetSpringArmTargetArmLength(DefaultCameraZoomAmount);
	}
}

void ARTSPlayerController::SetResetCameraToDefaultRate(float NewValue)
{
	ResetCameraToDefaultRate = NewValue;
}

void ARTSPlayerController::SetDoubleClickTime(float NewValue)
{
	UInputSettings::GetInputSettings()->DoubleClickTime = NewValue;
}

void ARTSPlayerController::SetGhostRotationRadius(float NewValue)
{
	GhostRotationRadius = NewValue;
}

void ARTSPlayerController::SetGhostRotationSpeed(float NewValue)
{
	GhostRotationSpeed = NewValue;
}

// TODO Also try and get MMB movement to work even when mouse offscreen

void ARTSPlayerController::SetupCameraCurves()
{
	/* Get min/max from curves. If curve is not usable then null it */

	if (ResetCameraCurve != nullptr)
	{
		float MinY, MaxY;
		ResetCameraCurve->GetValueRange(MinY, MaxY);
		if (MinY == 0.f && MaxY == 1.f)
		{
			ResetCameraCurve->GetTimeRange(ResetCameraCurveMin, ResetCameraCurveMax);
		}
		else
		{
			ResetCameraCurve = nullptr;

			UE_LOG(RTSLOG, Warning, TEXT("Player controller %s has camera reset curve %s which is "
				"not usable - it needs the Y axis to start at 0 and end at 1. Camera will update "
				"instantly instead"), *GetFName().ToString(), *ResetCameraCurve->GetFName().ToString());
		}
	}

	if (CameraMouseWheelZoomCurve != nullptr)
	{
		float MinY, MaxY;
		CameraMouseWheelZoomCurve->GetValueRange(MinY, MaxY);
		if (MinY == 0.f && MaxY == 1.f)
		{
			CameraMouseWheelZoomCurve->GetTimeRange(MouseWheelZoomCurveMin, MouseWheelZoomCurveMax);
		}
		else
		{
			CameraMouseWheelZoomCurve = nullptr;

			UE_LOG(RTSLOG, Warning, TEXT("Player controller %s has camera mouse wheel zoom curve "
				"%s which is not usable - it needs the Y axis to start at 0 and end at 1. Camera "
				"will update instantly instead"), *GetName(), *CameraMouseWheelZoomCurve->GetName());
		}
	}
}

void ARTSPlayerController::AssignViewportValues()
{
	// TODO Appears to not work in standalone. Probably will not work for packaged game either

#if WITH_EDITOR
	/* If in standalone... */
	if (GetWorld()->WorldType != EWorldType::PIE)
	{
		/* Need a delay otherwise GetViewportSize will return both 0. 
		Also I tried polling 
		CastChecked<ULocalPlayer>(Player)->ViewportClient->Viewport == nullptr waiting 
		for it to be non-nullptr but this doesn't work */
		RECALL(ARTSPlayerController::AssignViewportValues, 0.2f);
	}
#endif

	int32 SizeX, SizeY;
	GetViewportSize(SizeX, SizeY);

	// Implicit conversion from int to float here
	ViewportSize_X = SizeX;
	ViewportSize_Y = SizeY;

#if WITH_EDITOR
	/* Take into account the little black bars along the edges of the viewport when playing PIE */
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		FSceneViewport * SceneViewport = static_cast<FSceneViewport*>(CastChecked<ULocalPlayer>(Player)->ViewportClient->ViewportFrame);
		const FMargin WindowPadding = SceneViewport->FindWindow()->GetWindowBorderSize();
		ViewportSize_X -= WindowPadding.Left + WindowPadding.Right;
		ViewportSize_Y -= WindowPadding.Top + WindowPadding.Bottom;

		/* I really do not know why but it appears that when doing PIE with 2 players the server and
		client both have different viewport sizes so this code here tries to correct for that.
		Also PIE with just 1 player gives the viewport size of the client i nthe 2 player case which
		is just really odd, so this code corrects for these cases.
		I'm pretty sure I do nothing to manipulate the viewport/window/whatever so maybe this
		is a problem in engine source? */
		const ULevelEditorPlaySettings * PlaySettings = GetDefault<ULevelEditorPlaySettings>();
		int32 NumPIEPlayers;
		PlaySettings->GetPlayNumberOfClients(NumPIEPlayers);
		if (NumPIEPlayers == 1 || GetWorld()->IsServer() == false)
		{
			const float TitleBarSize = SceneViewport->FindWindow()->GetTitleBarSize().Get();
			ViewportSize_Y -= TitleBarSize;
		}
	}
#endif
}


//==========================================================================================
//	Development Cheat Widget
//==========================================================================================

#if WITH_EDITOR
void ARTSPlayerController::OnDevelopmentWidgetRequest(EDevelopmentAction Action)
{
	/* May need to do things like cancel ghost here */
	
	if (Action == EDevelopmentAction::DealMassiveDamageToSelectable)
	{
		/* Set the cursor to the most appropriate default engine .png file */
		FString FullCursorPath = FString::Printf(TEXT("%s%sEditor/Slate/ContentBrowser/SCC_MarkedForDelete"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
		/* The cursor setting logic expects a path relative to PROJECTNAME/Content/ folder, 
		so adjust for that */
		FPaths::MakePathRelativeTo(FullCursorPath, *FPaths::ProjectContentDir());
		FCursorInfo CursorInfo(FullCursorPath, FVector2D(0.5f, 0.5f));
		SetMouseCursor(CursorInfo);

		/* Flag this so that on our next LMB press we will carry out this action */
		DevelopmentInputIntercept_LMBPress = EDevelopmentAction::DealMassiveDamageToSelectable;

		/* Flag that the LMB release should do nothing while we have this ability active just 
		to be safe */
		DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreAllLMBRelease;

		/* Flag that RMB presses should cancel this */
		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::CancelDealMassiveDamage;
	}
	else if (Action == EDevelopmentAction::DamageSelectable)
	{
		/* Set mouse cursor */
		FString FullCursorPath = FString::Printf(TEXT("%s%sDocumentation/Source/Shared/Icons/Source/icon_DevicePowerOff_40x"), FPlatformProcess::BaseDir(), *FPaths::EngineDir());
		FPaths::MakePathRelativeTo(FullCursorPath, *FPaths::ProjectContentDir());
		FCursorInfo CursorInfo(FullCursorPath, FVector2D(0.5f, 0.5f));
		SetMouseCursor(CursorInfo);

		DevelopmentInputIntercept_LMBPress = EDevelopmentAction::DamageSelectable;
		DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreAllLMBRelease;
		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::CancelDamageSelectable;
	}
	else if (Action == EDevelopmentAction::AwardExperience)
	{
		/* Set mouse cursor */
		FString FullCursorPath = FString::Printf(TEXT("%s%sEditor/Slate/Icons/icon_Cascade_HigherLOD_40x"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
		FPaths::MakePathRelativeTo(FullCursorPath, *FPaths::ProjectContentDir());
		FCursorInfo CursorInfo(FullCursorPath, FVector2D(0.5f, 0.5f));
		SetMouseCursor(CursorInfo);

		DevelopmentInputIntercept_LMBPress = EDevelopmentAction::AwardExperience;
		DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreAllLMBRelease;
		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::CancelAwardExperience;
	}
	else if (Action == EDevelopmentAction::AwardLotsOfExperience)
	{
		/* Set mouse cursor */
		FString FullCursorPath = FString::Printf(TEXT("%s%sEditor/Slate/Icons/icon_Cascade_HighestLOD_40x"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
		FPaths::MakePathRelativeTo(FullCursorPath, *FPaths::ProjectContentDir());
		FCursorInfo CursorInfo(FullCursorPath, FVector2D(0.5f, 0.5f));
		SetMouseCursor(CursorInfo);

		DevelopmentInputIntercept_LMBPress = EDevelopmentAction::AwardLotsOfExperience;
		DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreAllLMBRelease;
		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::CancelAwardLotsOfExperience;
	}
	else if (Action == EDevelopmentAction::AwardExperienceToLocalPlayer)
	{
		Server_AwardExperienceToPlayer();
	}
	else if (Action == EDevelopmentAction::GiveRandomInventoryItem)
	{
		/* Set mouse cursor */
		FString FullCursorPath = FString::Printf(TEXT("%s%sEditor/Slate/Icons/icon_possess_40x"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
		FPaths::MakePathRelativeTo(FullCursorPath, *FPaths::ProjectContentDir());
		FCursorInfo CursorInfo(FullCursorPath, FVector2D(0.5f, 0.5f));
		SetMouseCursor(CursorInfo);

		DevelopmentInputIntercept_LMBPress = EDevelopmentAction::GiveRandomInventoryItem;
		DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreAllLMBRelease;
		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::CancelGiveRandomInventoryItem;
	}
	else if (Action == EDevelopmentAction::GiveSpecificInventoryItem_SelectionPhase)
	{
		/* Open a widget that lets the player choose which item to give */
		DevelopmentCheatWidget->ShowPopupWidget(Action);
		
		/* RMB clicks will close the popup menu */
		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::ClosePopupMenu;
	}
	else if (Action == EDevelopmentAction::GetUnitAIInfo)
	{
		if (HasSelection())
		{
			if (Statics::IsValid(Selected[0]))
			{
				AInfantry * Infantry = Cast<AInfantry>(Selected[0]);
				if (Infantry != nullptr)
				{
					Infantry->DisplayAIControllerOnScreenDebugInfo();
				}
			}
		}
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Request for unimplemented development action: \"%s\""),
			TO_STRING(EDevelopmentAction, Action));
	}
}

void ARTSPlayerController::OnDevelopmentWidgetRequest(EDevelopmentAction Action, int32 InAuxilleryData)
{
	if (Action == EDevelopmentAction::GiveSpecificInventoryItem_SelectTarget)
	{
		DevelopmentWidgetAuxilleryData = InAuxilleryData;
		
		/* Set mouse cursor */
		FString FullCursorPath = FString::Printf(TEXT("%s%sEditor/Slate/Icons/icon_possess_40x"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
		FPaths::MakePathRelativeTo(FullCursorPath, *FPaths::ProjectContentDir());
		FCursorInfo CursorInfo(FullCursorPath, FVector2D(0.5f, 0.5f));
		SetMouseCursor(CursorInfo);

		DevelopmentInputIntercept_LMBPress = EDevelopmentAction::GiveSpecificInventoryItem_SelectTarget;
		DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreAllLMBRelease;
		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::CancelGiveSpecificInventoryItem_SelectTarget;
	}
}

bool ARTSPlayerController::ExecuteDevelopmentInputInterceptAction_LMBPress()
{
#if WITH_EDITOR

	if (DevelopmentInputIntercept_LMBPress == EDevelopmentAction::DealMassiveDamageToSelectable 
		|| DevelopmentInputIntercept_LMBPress == EDevelopmentAction::DamageSelectable)
	{
		FHitResult Hit;
		if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(SELECTABLES_CHANNEL), false, Hit))
		{
			AActor * HitActor = Hit.GetActor();
			if (Statics::IsValid(HitActor) && IsASelectable(HitActor))
			{
				const float DamageAmount = (DevelopmentInputIntercept_LMBPress == EDevelopmentAction::DealMassiveDamageToSelectable)
					? 9999999.f : 20.f;
				
				Server_DealDamageToSelectable(nullptr, HitActor, DamageAmount);

				/* We'll make it a rule that if CTRL is held down while clicking then we do not 
				cancel the ability and user can use it again without having to re-click the 
				widget button */
				if (PlayerInput->IsCtrlPressed() == false)
				{
					/* Return mouse cursor to match cursor */
					SetMouseCursor(*DefaultCursor);

					DevelopmentInputIntercept_LMBPress = EDevelopmentAction::None;
					DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreNextLMBRelease;
					DevelopmentInputIntercept_RMBPress = DevelopmentCheatWidget->IsPopupMenuShowing() 
						? EDevelopmentAction::ClosePopupMenu : EDevelopmentAction::None;
				}
			}
		}

		return true;
	}
	else if (DevelopmentInputIntercept_LMBPress == EDevelopmentAction::AwardExperience
		|| DevelopmentInputIntercept_LMBPress == EDevelopmentAction::AwardLotsOfExperience)
	{
		FHitResult Hit;
		if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(SELECTABLES_CHANNEL), false, Hit))
		{
			AActor * HitActor = Hit.GetActor();
			if (Statics::IsValid(HitActor) && IsASelectable(HitActor))
			{
				ISelectable * AsSelectable = CastChecked<ISelectable>(HitActor);

				const bool bCanClassGainExperience = AsSelectable->CanClassGainExperience();

				if (bCanClassGainExperience)
				{
					if (AsSelectable->GetRank() < LevelingUpOptions::MAX_LEVEL)
					{
						float ExperienceAmountToAward;
						if (DevelopmentInputIntercept_LMBPress == EDevelopmentAction::AwardExperience)
						{
							ExperienceAmountToAward = AsSelectable->GetTotalExperienceRequiredForNextLevel() * FMath::RandRange(0.55f, 0.65f);
						}
						else // AwardLotsOfExperience
						{
							// Add enough XP to level up at least twice if possible
							ExperienceAmountToAward = AsSelectable->GetTotalExperienceRequiredForNextLevel() * FMath::RandRange(1.f, 1.1f);
							if (AsSelectable->GetRank() + 1 < LevelingUpOptions::MAX_LEVEL) // If at least two levels off max level
							{
								ExperienceAmountToAward += AsSelectable->GetTotalExperienceRequiredForLevel(AsSelectable->GetRank() + 2) * FMath::RandRange(1.f, 1.1f);
							}
						}

						Server_AwardExperience(HitActor, ExperienceAmountToAward);

						if (PlayerInput->IsCtrlPressed() == false)
						{
							/* Return mouse cursor to match cursor */
							SetMouseCursor(*DefaultCursor);

							DevelopmentInputIntercept_LMBPress = EDevelopmentAction::None;
							DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreNextLMBRelease;
							DevelopmentInputIntercept_RMBPress = DevelopmentCheatWidget->IsPopupMenuShowing()
								? EDevelopmentAction::ClosePopupMenu : EDevelopmentAction::None;
						}
					}
				}
			}
		}

		return true;
	}
	else if (DevelopmentInputIntercept_LMBPress == EDevelopmentAction::GiveRandomInventoryItem)
	{
		FHitResult Hit;
		if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(SELECTABLES_CHANNEL), false, Hit))
		{
			AActor * HitActor = Hit.GetActor();
			if (Statics::IsValid(HitActor) && IsASelectable(HitActor))
			{
				ISelectable * AsSelectable = CastChecked<ISelectable>(HitActor);

				FInventory * Inventory = AsSelectable->GetInventoryModifiable();
				if (Inventory != nullptr)
				{
					/* Pick random item. Should return not-None because we checked this 
					earlier at some point  */
					const EInventoryItem RandomItem = GI->GetRandomInventoryItem();
					
					Server_GiveInventoryItem(HitActor, RandomItem);

					if (PlayerInput->IsCtrlPressed() == false)
					{
						/* Return mouse cursor to match cursor */
						SetMouseCursor(*DefaultCursor);

						DevelopmentInputIntercept_LMBPress = EDevelopmentAction::None;
						DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreNextLMBRelease;
						DevelopmentInputIntercept_RMBPress = DevelopmentCheatWidget->IsPopupMenuShowing()
							? EDevelopmentAction::ClosePopupMenu : EDevelopmentAction::None;
					}
				}
			}
		}
		
		return true;
	}
	else if (DevelopmentInputIntercept_LMBPress == EDevelopmentAction::GiveSpecificInventoryItem_SelectTarget)
	{
		FHitResult Hit;
		if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(SELECTABLES_CHANNEL), false, Hit))
		{
			AActor * HitActor = Hit.GetActor();
			if (Statics::IsValid(HitActor) && IsASelectable(HitActor))
			{
				ISelectable * AsSelectable = CastChecked<ISelectable>(HitActor);

				FInventory * Inventory = AsSelectable->GetInventoryModifiable();
				if (Inventory != nullptr)
				{
					const EInventoryItem ItemType = Statics::ArrayIndexToInventoryItem(DevelopmentWidgetAuxilleryData);
					
					Server_GiveInventoryItem(HitActor, ItemType);

					if (PlayerInput->IsCtrlPressed() == false)
					{
						/* Return mouse cursor to match cursor */
						SetMouseCursor(*DefaultCursor);

						/* Hide the popup menu. Some users may like this to not happen. 
						Actually I'm going to not make it happen */
						//DevelopmentCheatWidget->HidePopupWidget();

						DevelopmentInputIntercept_LMBPress = EDevelopmentAction::None;
						DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::IgnoreNextLMBRelease;
						DevelopmentInputIntercept_RMBPress = DevelopmentCheatWidget->IsPopupMenuShowing()
							? EDevelopmentAction::ClosePopupMenu : EDevelopmentAction::None;
					}
				}
			}
		}

		return true;
	}

	return false;

#else
	return false;
#endif
}

bool ARTSPlayerController::ExecuteDevelopmentInputInterceptAction_LMBRelease()
{
#if WITH_EDITOR

	if (DevelopmentInputIntercept_LMBRelease == EDevelopmentAction::IgnoreAllLMBRelease)
	{
		return true;
	}
	else if (DevelopmentInputIntercept_LMBRelease == EDevelopmentAction::IgnoreNextLMBRelease)
	{
		DevelopmentInputIntercept_LMBRelease = EDevelopmentAction::None;
		return true;
	}

	return false;

#else
	return false;
#endif
}

bool ARTSPlayerController::ExecuteDevelopmentInputInterceptAction_RMBPress()
{
#if WITH_EDITOR
	if (DevelopmentInputIntercept_RMBPress == EDevelopmentAction::ClosePopupMenu)
	{
		/* Hide the popup menu widget */
		DevelopmentCheatWidget->HidePopupWidget();

		DevelopmentInputIntercept_RMBPress = EDevelopmentAction::None;

		return true;
	}
	else if (DevelopmentInputIntercept_RMBPress == EDevelopmentAction::CancelDealMassiveDamage 
		|| DevelopmentInputIntercept_RMBPress == EDevelopmentAction::CancelDamageSelectable 
		|| DevelopmentInputIntercept_RMBPress == EDevelopmentAction::CancelAwardExperience 
		|| DevelopmentInputIntercept_RMBPress == EDevelopmentAction::CancelAwardLotsOfExperience
		|| DevelopmentInputIntercept_RMBPress == EDevelopmentAction::CancelGiveRandomInventoryItem 
		|| DevelopmentInputIntercept_RMBPress == EDevelopmentAction::CancelGiveSpecificInventoryItem_SelectTarget)
	{
		/* Return mouse cursor to match cursor */
		SetMouseCursor(*DefaultCursor);
		
		DevelopmentInputIntercept_LMBPress = EDevelopmentAction::None;
		DevelopmentInputIntercept_LMBRelease = (Development_bIsLMBPressed)
			? EDevelopmentAction::IgnoreNextLMBRelease : EDevelopmentAction::None;
		DevelopmentInputIntercept_RMBPress = DevelopmentCheatWidget->IsPopupMenuShowing() 
			? EDevelopmentAction::ClosePopupMenu : EDevelopmentAction::None;
		
		return true;
	}

	return false;

#else
	return false;
#endif
}
#endif // WITH_EDITOR

//#if WITH_EDITORONLY_DATA
void ARTSPlayerController::Server_DealDamageToSelectable_Implementation(AActor * DamageInstigator, 
	AActor * Target, float DamageAmount)
{
	Target->TakeDamage(DamageAmount, FDamageEvent(UDamageType_Default::StaticClass()), nullptr, DamageInstigator);
}

void ARTSPlayerController::Server_AwardExperience_Implementation(AActor * ExperienceGainer, 
	float ExperienceAmount)
{
	CastChecked<ISelectable>(ExperienceGainer)->GainExperience(ExperienceAmount, false);
}

void ARTSPlayerController::Server_AwardExperienceToPlayer_Implementation()
{
	if (PS->GetRank() < LevelingUpOptions::MAX_COMMANDER_LEVEL)
	{
		const FCommanderLevelUpInfo & NextLevelInfo = FactionInfo->GetCommanderLevelUpInfo(PS->GetRank() + 1);

		/* Award 55 - 65% of the amount needed to gain a level */
		const float ExperienceGainAmount = NextLevelInfo.GetExperienceRequired() * FMath::RandRange(0.55f, 0.65f);
		PS->GainExperience(ExperienceGainAmount, false);
	}
}

void ARTSPlayerController::Server_GiveInventoryItem_Implementation(AActor * ItemRecipient, EInventoryItem ItemType)
{
	const FInventoryItemInfo * ItemInfo = GI->GetInventoryItemInfo(ItemType);

	ISelectable * AsSelectable = CastChecked<ISelectable>(ItemRecipient);

	GS->Server_TryPutItemInInventory(AsSelectable, AsSelectable->GetAttributesBase().IsSelected(),
		AsSelectable->GetAttributesBase().IsPrimarySelected(), AsSelectable->Selectable_GetPS(),
		ItemType, 1, *ItemInfo, EItemAquireReason::MagicallyCreated, HUD, nullptr);
}
//#endif


//=============================================================================================
//	Server RPC _Validate Functions
//=============================================================================================
//	All here so they are out of the way but for performance maybe it would be better to move 
//	them close to their _Implementation versions TODO look into this
//---------------------------------------------------------------------------------------------

bool ARTSPlayerController::Server_PlaceBuilding_Validate(EBuildingType BuildingType,
	const FVector_NetQuantize10 & Location, const FRotator & Rotation, uint8 ConstructionInstigatorID)
{
	return BuildingType != EBuildingType::NotBuilding;
}

bool ARTSPlayerController::Server_GiveLayFoundationCommand_Validate(EBuildingType BuildingType,
	const FVector_NetQuantize10 & Location, const FRotator & Rotation, uint8 BuildersID)
{
	return true;
}

bool ARTSPlayerController::Server_IssueContextCommand_Validate(const FContextCommandWithTarget & Command)
{
	return true;
}

bool ARTSPlayerController::Server_IssueLocationTargetingContextCommand_Validate(const FContextCommandWithLocation & Command)
{
	return true;
}

bool ARTSPlayerController::Server_IssueInstantContextCommand_Validate(const FContextCommand & Command)
{
	return true;
}

bool ARTSPlayerController::Server_IssueRightClickCommand_Validate(const FRightClickCommandWithTarget & Command)
{
	return true;
}

bool ARTSPlayerController::Server_IssueRightClickCommandOnInventoryItem_Validate(const FRightClickCommandOnInventoryItem & Command)
{
	return true;
}

bool ARTSPlayerController::Server_AckSetupForMatchComplete_Validate()
{
	return true;
}

bool ARTSPlayerController::Server_RequestBuyItemFromShop_Validate(AActor * Shop, uint8 ShopSlotIndex)
{
	return true;
}

bool ARTSPlayerController::Server_SellInventoryItem_Validate(uint8 SellerID, uint8 InventorySlotIndex, EInventoryItem ItemInSlot)
{
	return true;
}

bool ARTSPlayerController::Server_SendInMatchChatMessageToEveryone_Validate(const FString & Message)
{
	// Just return true - not going to check length like lobby because performance
	return true;
}

bool ARTSPlayerController::Server_SendInMatchChatMessageToTeam_Validate(const FString & Message)
{
	// Just return true - not going to check length like lobby because performance
	return true;
}

bool ARTSPlayerController::Server_ChangeTeamInLobby_Validate(ETeam NewTeam)
{
	return true;
}

bool ARTSPlayerController::Server_ChangeFactionInLobby_Validate(EFaction NewFaction)
{
	return true;
}

bool ARTSPlayerController::Server_ChangeStartingSpotInLobby_Validate(int16 PlayerStartID)
{
	return true;
}

bool ARTSPlayerController::Server_SendLobbyChatMessage_Validate(const FString & Message)
{
	/* Instead of returning true could check whether message's length is low enough for signs
	of modifed client */
	return true;
}

bool ARTSPlayerController::Server_AckPSSetupOnClient_Validate()
{
	return true;
}

bool ARTSPlayerController::Server_AckLevelStreamedIn_Validate()
{
	return true;
}

bool ARTSPlayerController::Server_RequestExecuteCommanderAbility_Validate(ECommanderAbility AbilityType)
{
	return true;
}

bool ARTSPlayerController::Server_RequestExecuteCommanderAbility_PlayerTargeting_Validate(ECommanderAbility AbilityType, uint8 TargetsPlayerID)
{
	return true;
}

bool ARTSPlayerController::Server_RequestExecuteCommanderAbility_SelectableTargeting_Validate(ECommanderAbility AbilityType,
	AActor * SelectableTarget)
{
	return true;
}

bool ARTSPlayerController::Server_RequestExecuteCommanderAbility_LocationTargeting_Validate(ECommanderAbility AbilityType,
	const FVector_NetQuantize & AbilityLocation)
{
	return true;
}

bool ARTSPlayerController::Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingSelectable_Validate(ECommanderAbility AbilityType, AActor * SelectableTarget)
{
	return true;
}

bool ARTSPlayerController::Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingLocation_Validate(ECommanderAbility AbilityType, const FVector_NetQuantize & AbilityLocation)
{
	return true;
}

bool ARTSPlayerController::Server_IssueInstantUseInventoryItemCommand_Validate(uint8 SelectableID, uint8 InventorySlotIndex, EInventoryItem ItemType)
{
	return true;
}

bool ARTSPlayerController::Server_IssueLocationTargetingUseInventoryItemCommand_Validate(uint8 SelectableID, uint8 InventorySlotIndex,
	EInventoryItem ItemType, const FVector_NetQuantize & Location)
{
	return true;
}

bool ARTSPlayerController::Server_IssueSelectableTargetingUseInventoryItemCommand_Validate(uint8 SelectableID, uint8 InventorySlotIndex,
	EInventoryItem ItemType, AActor * TargetSelectable)
{
	return true;
}

bool ARTSPlayerController::Server_RequestUnloadSingleUnitFromGarrison_Validate(ABuilding * BuildingGarrison, AActor * SelectableToUnload)
{
	return true;
}

bool ARTSPlayerController::Server_RequestUnloadGarrison_Validate(ABuilding * BuildingGarrison)
{
	return true;
}

//#if WITH_EDITOR
bool ARTSPlayerController::Server_DealDamageToSelectable_Validate(AActor * DamageInstigator,
	AActor * Target, float DamageAmount)
{
	return true;
}

bool ARTSPlayerController::Server_AwardExperience_Validate(AActor * ExperienceGainer,
	float ExperienceAmount)
{
	return true;
}

bool ARTSPlayerController::Server_AwardExperienceToPlayer_Validate()
{
	return true;
}

bool ARTSPlayerController::Server_GiveInventoryItem_Validate(AActor * ItemRecipient, EInventoryItem ItemType)
{
	return true;
}

bool ARTSPlayerController::Server_RequestAquireCommanderSkill_Validate(uint8 AllNodesArrayIndex)
{
	return true;
}
//#endif
