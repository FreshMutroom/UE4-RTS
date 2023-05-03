// Fill out your copyright notice in the Description page of Project Settings.

#include "KeyMappingMenu.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

#include "GameFramework/RTSGameInstance.h"
#include "Settings/KeyMappings.h"
#include "Settings/RTSGameUserSettings.h"
#include "UI/InMatchWidgets/Components/MyButton.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSPlayerController.h"


USingleKeyBindingWidget::USingleKeyBindingWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActionType = EKeyMappingAction::None;
	AxisType = EKeyMappingAxis::None;

#if WITH_EDITOR
	RunOnPostEditLogic();
#endif
}

void USingleKeyBindingWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, 
	UKeyBindingsWidget * InKeyBindingsWidget)
{
	assert(ActionType != EKeyMappingAction::None || AxisType != EKeyMappingAxis::None);
	
	PC = InPlayerController;
	OwningWidget = InKeyBindingsWidget;

	/* Initialize pointer to the info struct for the action this widget is for */
	if (IsForActionMapping())
	{
		BindingInfo = &KeyMappings::ActionInfos[KeyMappings::ActionTypeToArrayIndex(ActionType)]; 
	}
	else
	{
		BindingInfo = &AxisMappings::AxisInfos[AxisMappings::AxisTypeToArrayIndex(AxisType)];
	}

	if (IsWidgetBound(Text_Action))
	{
		Text_Action->SetText(BindingInfo->DisplayName);
	}

	if (IsWidgetBound(Button_Remap))
	{
		if (BindingInfo->bCanBeRemapped)
		{
			Button_Remap->SetPC(InPlayerController);
			Button_Remap->SetOwningWidget();
			Button_Remap->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
			Button_Remap->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleKeyBindingWidget::UIBinding_OnRemapButtonLeftMousePress);
			Button_Remap->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleKeyBindingWidget::UIBinding_OnRemapButtonLeftMouseRelease);
			Button_Remap->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleKeyBindingWidget::UIBinding_OnRemapButtonRightMousePress);
			Button_Remap->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleKeyBindingWidget::UIBinding_OnRemapButtonRightMouseRelease);
			Button_Remap->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());

		}
		else
		{
			/* Since the action cannot be remapped then hide the remapping button */
			Button_Remap->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	NumKeyElementWidgetsBound += IsWidgetBound(Border_KeyElement1);
	NumKeyElementWidgetsBound += IsWidgetBound(Border_KeyElement2);
	NumKeyElementWidgetsBound += IsWidgetBound(Border_KeyElement3);
	NumKeyElementWidgetsBound += IsWidgetBound(Border_KeyElement4);
	NumKeyElementWidgetsBound += IsWidgetBound(Border_KeyElement5);
	NumKeyElementWidgetsBound += IsWidgetBound(Border_KeyElement6);
	NumKeyElementWidgetsBound += IsWidgetBound(Border_KeyElement7);

	BorderKeyElements[0] = Border_KeyElement1;
	BorderKeyElements[1] = Border_KeyElement2;
	BorderKeyElements[2] = Border_KeyElement3;
	BorderKeyElements[3] = Border_KeyElement4;
	BorderKeyElements[4] = Border_KeyElement5;
	BorderKeyElements[5] = Border_KeyElement6;
	BorderKeyElements[6] = Border_KeyElement7;

	if (NumKeyElementWidgetsBound > 0 && IsForActionMapping() == false)
	{
		// Axis mappings cannot have modifier keys so hide every border except the first one
		HideKeyElementWidgetsFromIndexCheckIfNull(1);
	}
}

void USingleKeyBindingWidget::SetTypes(EKeyMappingAction InActionType, EKeyMappingAxis InAxisType)
{
	ActionType = InActionType;
	AxisType = InAxisType;
}

void USingleKeyBindingWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	GInitRunaway();
}

void USingleKeyBindingWidget::UpdateAppearanceForCurrentBindingValue(URTSGameInstance * GameInst, URTSGameUserSettings * GameUserSettings)
{
	const FKeyWithModifiers & AssignedKey = IsForActionMapping()
		? GameUserSettings->GetKey(ActionType)
		: GameUserSettings->GetKey(AxisType);
	
	const FKeyInfo & AssignedKeyInfo = GameInst->GetInputKeyInfo(AssignedKey.Key);
	
	// Text
	if (IsWidgetBound(Text_Key))
	{
		Text_Key->SetText(GetDisplayText(AssignedKey, AssignedKeyInfo));
	}

	// Image
	if (NumKeyElementWidgetsBound > 0)
	{
		if (IsForActionMapping())
		{
			// Ordering here is CTRL first then ALT then SHIFT. You can change this by changing 
			// the ordering of the if/else statements here

			// Here we're assuming all 7 border and if using text blocks are bound

			if (AssignedKey.HasCTRLModifier())
			{
				SetBrushAndText_Modifier(GameInst->KeyMappings_GetCTRLModifierImage(), Border_KeyElement1, Text_KeyElement1);
				SetBrushAndText_PlusSymbol(GameInst->KeyMappings_GetPlusSymbolImage(), Border_KeyElement2, Text_KeyElement2);

				if (AssignedKey.HasALTModifier())
				{
					SetBrushAndText_Modifier(GameInst->KeyMappings_GetALTModifierImage(), Border_KeyElement3, Text_KeyElement3);
					SetBrushAndText_PlusSymbol(GameInst->KeyMappings_GetPlusSymbolImage(), Border_KeyElement4, Text_KeyElement4);

					if (AssignedKey.HasSHIFTModifier())
					{
						SetBrushAndText_Modifier(GameInst->KeyMappings_GetSHIFTModifierImage(), Border_KeyElement5, Text_KeyElement5);
						SetBrushAndText_PlusSymbol(GameInst->KeyMappings_GetPlusSymbolImage(), Border_KeyElement6, Text_KeyElement6);
						SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement7, Text_KeyElement7);
					}
					else
					{
						SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement5, Text_KeyElement5);
						HideKeyElementWidgetsFromIndex(5);
					}
				}
				else if (AssignedKey.HasSHIFTModifier())
				{
					SetBrushAndText_Modifier(GameInst->KeyMappings_GetSHIFTModifierImage(), Border_KeyElement3, Text_KeyElement3);
					SetBrushAndText_PlusSymbol(GameInst->KeyMappings_GetPlusSymbolImage(), Border_KeyElement4, Text_KeyElement4);
					SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement5, Text_KeyElement5);
					HideKeyElementWidgetsFromIndex(5);
				}
				else
				{
					SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement3, Text_KeyElement3);
					HideKeyElementWidgetsFromIndex(3);
				}
			}
			else if (AssignedKey.HasALTModifier())
			{
				SetBrushAndText_Modifier(GameInst->KeyMappings_GetALTModifierImage(), Border_KeyElement1, Text_KeyElement1);
				SetBrushAndText_PlusSymbol(GameInst->KeyMappings_GetPlusSymbolImage(), Border_KeyElement2, Text_KeyElement2);

				if (AssignedKey.HasSHIFTModifier())
				{
					SetBrushAndText_Modifier(GameInst->KeyMappings_GetSHIFTModifierImage(), Border_KeyElement3, Text_KeyElement3);
					SetBrushAndText_PlusSymbol(GameInst->KeyMappings_GetPlusSymbolImage(), Border_KeyElement4, Text_KeyElement4);
					SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement5, Text_KeyElement5);
					HideKeyElementWidgetsFromIndex(5);
				}
				else
				{
					SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement3, Text_KeyElement3);
					HideKeyElementWidgetsFromIndex(3);
				}
			}
			else if (AssignedKey.HasSHIFTModifier())
			{
				SetBrushAndText_Modifier(GameInst->KeyMappings_GetSHIFTModifierImage(), Border_KeyElement1, Text_KeyElement1);
				SetBrushAndText_PlusSymbol(GameInst->KeyMappings_GetPlusSymbolImage(), Border_KeyElement2, Text_KeyElement2);
				SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement3, Text_KeyElement3);
				HideKeyElementWidgetsFromIndex(3);
			}
			else
			{
				SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement1, Text_KeyElement1);
				HideKeyElementWidgetsFromIndex(1);
			}
		}
		else
		{
			// Just the first image cause axis mappings cannot have modifiers
			SetBrushAndText_Key(AssignedKeyInfo, Border_KeyElement1, Text_KeyElement1);
		}
	}
}

bool USingleKeyBindingWidget::IsForActionMapping() const
{
	return (ActionType != EKeyMappingAction::None);
}

EKeyMappingAction USingleKeyBindingWidget::GetActionType() const
{
	return ActionType;
}

EKeyMappingAxis USingleKeyBindingWidget::GetAxisType() const
{
	return AxisType;
}

FText USingleKeyBindingWidget::GetDisplayText(const FKeyWithModifiers & Key, const FKeyInfo & KeyInfo)
{
	FString String; 
	if (Key.HasCTRLModifier())
	{
		String += "CTRL + ";
	}
	if (Key.HasALTModifier())
	{
		String += "ALT + ";
	}
	if (Key.HasSHIFTModifier())
	{
		String += "SHIFT + ";
	}
	String += KeyInfo.GetText();
	return FText::FromString(String);
}

void USingleKeyBindingWidget::SetBrushAndText_Modifier(const FSlateBrush & ModifierBrush, UBorder * Border, UTextBlock * TextBlock)
{
	Border->SetBrush(ModifierBrush);
	Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (IsWidgetBound(TextBlock))
	{
		// My way of essentially hiding the text
		TextBlock->SetText(FText::GetEmpty());
	}
}

void USingleKeyBindingWidget::SetBrushAndText_PlusSymbol(const FSlateBrush & InBrush, UBorder * Border, UTextBlock * TextBlock)
{
	Border->SetBrush(InBrush);
	Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (IsWidgetBound(TextBlock))
	{
		// My way of essentially hiding the text
		TextBlock->SetText(FText::GetEmpty());
	}
}

void USingleKeyBindingWidget::SetBrushAndText_Key(const FKeyInfo & KeyInfo, UBorder * Border, 
	UTextBlock * TextBlock)
{
	if (KeyInfo.GetImageDisplayMethod() == EInputKeyDisplayMethod::ImageOnly)
	{
		Border->SetBrush(KeyInfo.GetFullBrush());
		Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else // Assumed ImageAndAddTextAtRuntime
	{
		assert(KeyInfo.GetImageDisplayMethod() == EInputKeyDisplayMethod::ImageAndAddTextAtRuntime);

		Border->SetBrush(KeyInfo.GetPartialBrush());
		Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		if (IsWidgetBound(TextBlock))
		{
			// Change the size of the text. Setting font is the only way I know
			TextBlock->SetFont(KeyInfo.GetPartialBrushTextFont());
			TextBlock->SetText(KeyInfo.GetPartialBrushText());

			// Vis is never changed. SetText(FText::GetEmpty()) is done instead
			//TextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}

void USingleKeyBindingWidget::HideKeyElementWidgetsFromIndex(int32 Index)
{
	// Just hiding the borders here cause we assume the texts are children of them
	for (int32 i = Index; i < 7; ++i)
	{
		BorderKeyElements[i]->SetVisibility(ESlateVisibility::Hidden);
	}
}

void USingleKeyBindingWidget::HideKeyElementWidgetsFromIndexCheckIfNull(int32 Index)
{
	for (int32 i = Index; i < 7; ++i)
	{
		if (BorderKeyElements[i] != nullptr)
		{
			BorderKeyElements[i]->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void USingleKeyBindingWidget::UIBinding_OnRemapButtonLeftMousePress()
{
	PC->OnLMBPressed_RemapKey(Button_Remap);
}

void USingleKeyBindingWidget::UIBinding_OnRemapButtonLeftMouseRelease()
{
	PC->OnLMBReleased_RemapKey(Button_Remap, this);
}

void USingleKeyBindingWidget::UIBinding_OnRemapButtonRightMousePress()
{
	PC->OnRMBPressed_RemapKey(Button_Remap);
}

void USingleKeyBindingWidget::UIBinding_OnRemapButtonRightMouseRelease()
{
	PC->OnRMBReleased_RemapKey(Button_Remap);
}

void USingleKeyBindingWidget::OnClicked()
{
	if (BindingInfo->bCanBeRemapped)
	{
		OwningWidget->OnSingleBindingWidgetClicked(this);
	}
	else
	{
		// TODO try show a warning saying "you cannot rempa that key". Note we could be 
		// in either main menu or in match
	}
}

#if WITH_EDITOR
void USingleKeyBindingWidget::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	RunOnPostEditLogic();
}

void USingleKeyBindingWidget::RunOnPostEditLogic()
{
	bCanEditActionType = (AxisType == EKeyMappingAxis::None);
	bCanEditAxisType = (ActionType == EKeyMappingAction::None);
}
#endif


void UPressAnyKeyWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	GInitRunaway();
}

void UPressAnyKeyWidget::SetupForAboutToBeShown(URTSGameInstance * GameInst)
{
	if (IsWidgetBound(Text_Message))
	{
		/* Make sure the message includes the name of the key currently bound to cancel */
		URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
		const FKeyWithModifiers & CancelKey = Settings->GetKey(EKeyMappingAction::OpenPauseMenuSlashCancel);
		if (CancelKey.Key != EKeys::Invalid)
		{
			const FKeyInfo & KeyInfo = GameInst->GetInputKeyInfo(CancelKey.Key);
			Text_Message->SetText(FText::Format(NSLOCTEXT("PressAnyKeyWidget", "Message", "Press any key to "
				"rebind action. Hold down {0} to cancel"), USingleKeyBindingWidget::GetDisplayText(CancelKey, KeyInfo)));
		}
		else // Nothing is bound to the cancel key
		{
			Text_Message->SetText(NSLOCTEXT("PressAnyKeyWidget", "Message2", "Press any key to "
				"rebind action"));
		}
	}
}


void URebindingCollisionWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, UKeyBindingsWidget * InKeyBindingsWidget)
{
	PC = InPlayerController;
	KeyBindingsWidget = InKeyBindingsWidget;

	if (IsWidgetBound(Button_Confirm))
	{
		Button_Confirm->SetPC(InPlayerController);
		Button_Confirm->SetOwningWidget();
		Button_Confirm->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Confirm->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnConfirmButtonLeftMousePress);
		Button_Confirm->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnConfirmButtonLeftMouseRelease);
		Button_Confirm->GetOnRightMouseButtonPressDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnConfirmButtonRightMousePress);
		Button_Confirm->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnConfirmButtonRightMouseRelease);
		Button_Confirm->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
	if (IsWidgetBound(Button_Cancel))
	{
		Button_Cancel->SetPC(InPlayerController);
		Button_Cancel->SetOwningWidget();
		Button_Cancel->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Cancel->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnCancelButtonLeftMousePress);
		Button_Cancel->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnCancelButtonLeftMouseRelease);
		Button_Cancel->GetOnRightMouseButtonPressDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnCancelButtonRightMousePress);
		Button_Cancel->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &URebindingCollisionWidget::UIBinding_OnCancelButtonRightMouseRelease);
		Button_Cancel->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
}

void URebindingCollisionWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	GInitRunaway();
}

void URebindingCollisionWidget::SetupFor(const FTryBindActionResult & Result)
{
	if (IsWidgetBound(Text_AlreadyBoundAction))
	{
		FText Text;
		if (Result.AlreadyBoundToKey_Axis != EKeyMappingAxis::None)
		{
			const FInputAxisInfo & ActionInfo = AxisMappings::AxisInfos[AxisMappings::AxisTypeToArrayIndex(Result.AlreadyBoundToKey_Axis)];
			Text = FText::Format(NSLOCTEXT("SetupFor", "Axis", "Key already bound to action {0}. It will become unbound. Continue?"), ActionInfo.DisplayName);
		}
		else
		{
			/* Up to 8 actions could become unbinded if we are trying to bind an axis. 
			If there's just 1 we'll show it. If there's more than 1 we'll just say 2+ actions. 
			We could show them though if we want.
			The lower entries of the array always contain the non-Nones so just checking index 
			1 is enough to know if 2+ actions will become unbinded */
			if (Result.AlreadyBoundToKey_Action[1] != EKeyMappingAction::None)
			{
				Text = NSLOCTEXT("SetupFor", "2+ Actions", "Two or more actions will become unbound. Continue?");
			}
			else
			{
				const FInputActionInfo & ActionInfo = KeyMappings::ActionInfos[KeyMappings::ActionTypeToArrayIndex(Result.AlreadyBoundToKey_Action[0])];
				Text = FText::Format(NSLOCTEXT("SetupFor", "Action", "Key already bound to action {0}. It will become unbound. Continue?"), ActionInfo.DisplayName);
			}
		}

		Text_AlreadyBoundAction->SetText(Text);
	}
}

void URebindingCollisionWidget::OnCancelButtonClicked(ARTSPlayerController * PlayCon)
{
	PlayCon->CancelPendingKeyRebind();
	KeyBindingsWidget->OnPendingKeyBindCancelled();

	SetVisibility(ESlateVisibility::Hidden);
}

void URebindingCollisionWidget::UIBinding_OnConfirmButtonLeftMousePress()
{
	PC->OnLMBPressed_RebindingCollisionWidgetConfirm(Button_Confirm);
}

void URebindingCollisionWidget::UIBinding_OnConfirmButtonLeftMouseRelease()
{
	PC->OnLMBReleased_RebindingCollisionWidgetConfirm(Button_Confirm, this);
}

void URebindingCollisionWidget::UIBinding_OnConfirmButtonRightMousePress()
{
	PC->OnRMBPressed_RebindingCollisionWidgetConfirm(Button_Confirm);
}

void URebindingCollisionWidget::UIBinding_OnConfirmButtonRightMouseRelease()
{
	PC->OnRMBReleased_RebindingCollisionWidgetConfirm(Button_Confirm);
}

void URebindingCollisionWidget::UIBinding_OnCancelButtonLeftMousePress()
{
	PC->OnLMBPressed_RebindingCollisionWidgetCancel(Button_Cancel);
}

void URebindingCollisionWidget::UIBinding_OnCancelButtonLeftMouseRelease()
{
	PC->OnLMBReleased_RebindingCollisionWidgetCancel(Button_Cancel, this);
}

void URebindingCollisionWidget::UIBinding_OnCancelButtonRightMousePress()
{
	PC->OnRMBPressed_RebindingCollisionWidgetCancel(Button_Cancel);
}

void URebindingCollisionWidget::UIBinding_OnCancelButtonRightMouseRelease()
{
	PC->OnRMBReleased_RebindingCollisionWidgetCancel(Button_Cancel);
}


//-------------------------------------------------------------------------------------------
//===========================================================================================
//	------- Main Widget -------
//===========================================================================================
//-------------------------------------------------------------------------------------------

void UKeyBindingsWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, 
	bool bUpdateWidgetsToReflectAppliedValues)
{
	GI = GameInst;
	PC = InPlayerController;
	
	if (IsWidgetBound(Button_ResetToDefaults))
	{
		Button_ResetToDefaults->SetPC(InPlayerController);
		Button_ResetToDefaults->SetOwningWidget();
		Button_ResetToDefaults->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_ResetToDefaults->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonLeftMousePress);
		Button_ResetToDefaults->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonLeftMouseRelease);
		Button_ResetToDefaults->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonRightMousePress);
		Button_ResetToDefaults->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonRightMouseRelease);
		Button_ResetToDefaults->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	assert(PressAnyKeyWidget_BP != nullptr);

	{
		/* Create the "Press any key" widget */
		PressAnyKeyWidget = CreateWidget<UPressAnyKeyWidget>(PC, PressAnyKeyWidget_BP);
		PressAnyKeyWidget->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(PressAnyKeyWidget);
		CanvasSlot->SetZOrder(PRESS_ANY_KEY_WIDGET_Z_ORDER);
	}

	if (RebindingCollisionWidget_BP != nullptr)
	{
		/* Create the remap collision widget */
		RebindingCollisionWidget = CreateWidget<URebindingCollisionWidget>(PC, RebindingCollisionWidget_BP);
		RebindingCollisionWidget->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(RebindingCollisionWidget);
		CanvasSlot->SetZOrder(REBINDING_COLLISION_WIDGET_Z_ORDER);
		RebindingCollisionWidget->InitialSetup(GameInst, InPlayerController, this);
	}

	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	if (bAutoPopulatePanel)
	{
		UE_CLOG(IsWidgetBound(Panel_BindingWidgets) == false, RTSLOG, Fatal, TEXT("[%s]: bAutoPopulatePanel "
			"is true but Panel_BindingWidgets is not bound"), *GetClass()->GetName());

		UE_CLOG(SingleKeyBindingWidget_BP == nullptr, RTSLOG, Fatal, TEXT("No single binding widget "
			"set for widget [%s]"), *GetClass()->GetName());
		
		/* First remove any USingleKeyBindingWidget that were added in editor */
		TArray <UWidget *> AllChildren;
		WidgetTree->GetAllWidgets(AllChildren);
		for (UWidget * Widg : AllChildren)
		{
			USingleKeyBindingWidget * Widget = Cast<USingleKeyBindingWidget>(Widg);
			if (Widget != nullptr)
			{
				Widget->RemoveFromViewport();
			}
		}

		/* Create widgets */
		BindingWidgets.Init(nullptr, KeyMappings::NUM_ACTIONS + AxisMappings::NUM_AXIS);
		for (int32 i = 0; i < KeyMappings::NUM_ACTIONS; ++i)
		{
			// Create widget 
			USingleKeyBindingWidget * Widget = CreateWidget<USingleKeyBindingWidget>(PC, SingleKeyBindingWidget_BP);
			Panel_BindingWidgets->AddChild(Widget);

			// Set widget up
			const EKeyMappingAction ActionType = KeyMappings::ArrayIndexToActionType(i);
			BindingWidgets[GetBindingWidgetsArrayIndex(ActionType)] = Widget;
			Widget->SetTypes(ActionType, EKeyMappingAxis::None);
			Widget->InitialSetup(GameInst, InPlayerController, this);
			if (bUpdateWidgetsToReflectAppliedValues)
			{
				Widget->UpdateAppearanceForCurrentBindingValue(GameInst, GameUserSettings);
			}
		}
		for (int32 i = 0; i < AxisMappings::NUM_AXIS; ++i)
		{
			// Create widget
			USingleKeyBindingWidget * Widget = CreateWidget<USingleKeyBindingWidget>(PC, SingleKeyBindingWidget_BP);
			Panel_BindingWidgets->AddChild(Widget);

			// Set widget up
			const EKeyMappingAxis AxisType = AxisMappings::ArrayIndexToAxisType(i);
			BindingWidgets[GetBindingWidgetsArrayIndex(AxisType)] = Widget;
			Widget->SetTypes(EKeyMappingAction::None, AxisType);
			Widget->InitialSetup(GameInst, InPlayerController, this);
			if (bUpdateWidgetsToReflectAppliedValues)
			{
				Widget->UpdateAppearanceForCurrentBindingValue(GameInst, GameUserSettings);
			}
		}
	}
	else
	{
		/* Find all single binding widgets and set them up */
		TArray <UWidget *> AllChildren;
		WidgetTree->GetAllWidgets(AllChildren);
		BindingWidgets.Init(nullptr, KeyMappings::NUM_ACTIONS + AxisMappings::NUM_AXIS);
		for (UWidget * Widg : AllChildren)
		{
			USingleKeyBindingWidget * Widget = Cast<USingleKeyBindingWidget>(Widg);
			if (Widget != nullptr)
			{
				BindingWidgets[AssignWidgetsArrayIndex(Widget)] = Widget;
				Widget->InitialSetup(GameInst, InPlayerController, this);
				if (bUpdateWidgetsToReflectAppliedValues)
				{
					Widget->UpdateAppearanceForCurrentBindingValue(GameInst, GameUserSettings);
				}
			}
		}
	}
}

void UKeyBindingsWidget::UpdateAppearanceForCurrentValues()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	for (const auto & Widget : BindingWidgets)
	{
		// Container can have nulls so check first
		if (Widget != nullptr)
		{
			Widget->UpdateAppearanceForCurrentBindingValue(GI, GameUserSettings);
		}
	}
}

void UKeyBindingsWidget::OnSingleBindingWidgetClicked(USingleKeyBindingWidget * ClickedWidget)
{
	PendingRebindInstigator = ClickedWidget;

	// On the player controller flag that the player wants to remap a key
	if (ClickedWidget->IsForActionMapping())
	{
		PC->ListenForKeyRemappingInputEvents(ClickedWidget->GetActionType(), this);
	}
	else
	{
		PC->ListenForKeyRemappingInputEvents(ClickedWidget->GetAxisType(), this);
	}

	// Show the "Press a key to change the binding" widget
	PressAnyKeyWidget->SetupForAboutToBeShown(GI);
	PressAnyKeyWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

bool UKeyBindingsWidget::OnKeyBindAttempt(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
	bool bSuccess, const FTryBindActionResult & Result)
{
	PressAnyKeyWidget->SetVisibility(ESlateVisibility::Hidden);
	
	if (bSuccess)
	{
		PlayCon->CancelPendingKeyRebind();

		URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
		
		/* Update the widget that got it's new binding */
		PendingRebindInstigator->UpdateAppearanceForCurrentBindingValue(GameInst, Settings);
		PendingRebindInstigator = nullptr;

		/* Also need to update any widgets that became unbounded due to the change */
		if (Result.AlreadyBoundToKey_Axis != EKeyMappingAxis::None)
		{
			USingleKeyBindingWidget * Widget = BindingWidgets[GetBindingWidgetsArrayIndex(Result.AlreadyBoundToKey_Axis)];
			// Update corrisponding widget. It could be null I guess. Other than reset to defaults 
			// button I don't know how they could change it back though.
			if (Widget != nullptr)
			{
				Widget->UpdateAppearanceForCurrentBindingValue(GameInst, Settings);
			}
		}
		else
		{
			for (const auto & ActionType : Result.AlreadyBoundToKey_Action)
			{
				if (ActionType != EKeyMappingAction::None)
				{
					USingleKeyBindingWidget * Widget = BindingWidgets[GetBindingWidgetsArrayIndex(ActionType)];
					if (Widget != nullptr)
					{
						// Update corrisponding widget. It could be null I guess. Other than reset to defaults 
						// button I don't know how they could change it back though.
						Widget->UpdateAppearanceForCurrentBindingValue(GameInst, Settings);
					}
				}
			}
		}

		if (IsCollisionConfirmationWidgetShowingOrPlayingShowAnim())
		{
			RebindingCollisionWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		// Remember: write to disk when closing this widget

		return false;
	}
	else
	{
		if (Result.Warning == EGameWarning::None)
		{
			PendingKey = Result.KeyWeAreTryingToAssign;

			// Show the rebinding collision widget
			RebindingCollisionWidget->SetupFor(Result);
			RebindingCollisionWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			return (RebindingCollisionWidget != nullptr);
		}
		else if (Result.Warning == EGameWarning::WouldUnbindUnremappableAction)
		{
			PlayCon->CancelPendingKeyRebind();
			// This can bubble to HUD that bubbles to pause menu. Quite a lot of bubbling. 
			// Consider storing pointer to pause menu here
			PlayCon->OnMenuWarningHappened(Result.Warning);
			return false;
		}
		// This branch probably won't be taken - have checked for this already a lot but just in case 
		else if (Result.Warning == EGameWarning::NotAllowedToRemapAction)
		{
			PlayCon->CancelPendingKeyRebind();
			PlayCon->OnMenuWarningHappened(Result.Warning);
			return false;
		}
		else
		{
			assert(0); // Unexpected warning type
			return false;
		}
	}
}

void UKeyBindingsWidget::OnPendingKeyBindCancelled()
{
	PendingRebindInstigator = nullptr;
}

void UKeyBindingsWidget::OnPendingKeyBindCancelledViaCancelKey()
{
	PendingRebindInstigator = nullptr;
	PressAnyKeyWidget->SetVisibility(ESlateVisibility::Hidden);
}

bool UKeyBindingsWidget::IsCollisionConfirmationWidgetShowingOrPlayingShowAnim() const
{
	return RebindingCollisionWidget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible;
}

const FKeyWithModifiers & UKeyBindingsWidget::GetPendingKey() const
{
	return PendingKey;
}

void UKeyBindingsWidget::OnOpened()
{
	//// Probably should never need to load from disk
	//if (Option == EAppearanceOption::LoadFromDisk)
	//{
	//
	//}
	//// And should probably never need to load from game user settings. 
	//// A reason we might need to though: the user has like a quick adjust button that 
	//// remaps a key or something on the fly. Unlikely though
	//else if (Option == EAppearanceOption::LoadFromGameUserSettingsObject)
	//{
	//
	//}
	//else { /* Appearance up to date */ }
}

void UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonLeftMousePress()
{
	PC->OnLMBPressed_ResetKeyBindingsToDefaults(Button_ResetToDefaults);
}

void UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonLeftMouseRelease()
{
	PC->OnLMBReleased_ResetKeyBindingsToDefaults(Button_ResetToDefaults);
}

void UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonRightMousePress()
{
	PC->OnRMBPressed_ResetKeyBindingsToDefaults(Button_ResetToDefaults);
}

void UKeyBindingsWidget::UIBinding_OnResetToDefaultsButtonRightMouseRelease()
{
	PC->OnRMBReleased_ResetKeyBindingsToDefaults(Button_ResetToDefaults);
}

int32 UKeyBindingsWidget::AssignWidgetsArrayIndex(USingleKeyBindingWidget * Widget) const
{
	return Widget->IsForActionMapping()
		? static_cast<int32>(Widget->GetActionType()) - 1
		: KeyMappings::NUM_ACTIONS + static_cast<int32>(Widget->GetAxisType()) - 1;
}

int32 UKeyBindingsWidget::GetBindingWidgetsArrayIndex(EKeyMappingAction ActionType) const
{
	return static_cast<int32>(ActionType) - 1;
}

int32 UKeyBindingsWidget::GetBindingWidgetsArrayIndex(EKeyMappingAxis AxisType) const
{
	return KeyMappings::NUM_ACTIONS + static_cast<int32>(AxisType) - 1;
}


