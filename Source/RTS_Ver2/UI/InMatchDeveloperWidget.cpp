// Fill out your copyright notice in the Description page of Project Settings.

#include "InMatchDeveloperWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/BorderSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/Image.h"

#include "Statics/Statics.h"


const float UInMatchDeveloperWidget::STARTING_Y_POSITION = -470.f;
const float UInMatchDeveloperWidget::Y_SPACING_BETWEEN_BUTTONS = 80.f;
const FVector2D UInMatchDeveloperWidget::BUTTON_SIZE = FVector2D(150.f, 75.f);

UInMatchDeveloperWidget::UInMatchDeveloperWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	PopupMenuWidget_BP = UDevelopmentPopupWidget::StaticClass();
}

bool UInMatchDeveloperWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	// Setup button bindings
	if (IsWidgetBound(Button_DestroySelectable))
	{
		Button_DestroySelectable->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnDestroySelectableButtonClicked);
	}
	if (IsWidgetBound(Button_DamageSelectable))
	{
		Button_DamageSelectable->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnDamageSelectableButtonClicked);
	}
	if (IsWidgetBound(Button_AwardExperience))
	{
		Button_AwardExperience->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnAwardExperienceButtonClicked);
	}
	if (IsWidgetBound(Button_AwardLotsOfExperience))
	{
		Button_AwardLotsOfExperience->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnAwardLotsOfExperienceButtonClicked);
	}
	if (IsWidgetBound(Button_AwardExperienceToPlayer))
	{
		Button_AwardExperienceToPlayer->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnAwardExperienceToPlayerButtonClicked);
	}
	if (IsWidgetBound(Button_GiveRandomInventoryItem))
	{
		/* Only make visible if inventory items exist */
		if (Statics::NUM_INVENTORY_ITEM_TYPES > 0)
		{
			Button_GiveRandomInventoryItem->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnGiveRandomItemButtonClicked);
		}
		else
		{
			Button_GiveRandomInventoryItem->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (IsWidgetBound(Button_GiveSpecificInventoryItem))
	{
		/* Only make visible if inventory items exist */
		if (Statics::NUM_INVENTORY_ITEM_TYPES > 0)
		{
			Button_GiveSpecificInventoryItem->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnGiveSpecificItemButtonClicked);
		}
		else
		{
			Button_GiveSpecificInventoryItem->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (IsWidgetBound(Button_GetAIControllerInfo))
	{
		Button_GetAIControllerInfo->OnClicked.AddDynamic(this, &UInMatchDeveloperWidget::UIBinding_OnGetUnitAIInfoButtonClicked);
	}

	return false;
}

bool UInMatchDeveloperWidget::IsEditorOnly() const
{
	/* Actually this could be useful in packaged builds too */
	return true;
}

TSharedRef<SWidget> UInMatchDeveloperWidget::RebuildWidget()
{
	if (GetClass() == StaticClass())
	{
		//========================================================================================
		// Create a default version of this widget to support 'out of the box' functionality 
		//========================================================================================

		//========================================================================================
		//	Root widget
		
		/* Assuming we don't already have a canvas panel root widget. If we do then we don't need 
		to create this canvas panel */
		assert(GetRootWidget() == nullptr); 
		UCanvasPanel * RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = RootWidget;

		//========================================================================================
		//	Super

		const TSharedRef <SWidget> Ref = Super::RebuildWidget();

		//========================================================================================
		//	Button_DestroySelectable

		UButton * Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		UCanvasPanelSlot * ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 1.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_DestroySelectable = Button;
	
		// Add text to it that says something like "Destroy"
		UTextBlock * TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("Destroy"));

		//========================================================================================
		//	Button_DamageSelectable

		Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 0.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_DamageSelectable = Button;

		// Add text that says something like "Damage"
		TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("Damage"));

		//========================================================================================
		//	Button_AwardExperience

		Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 2.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_AwardExperience = Button;

		// Add text that says something like "Award EXP"
		TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("Award XP"));

		//========================================================================================
		//	Button_AwardLotsOfExperience

		Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 3.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_AwardLotsOfExperience = Button;

		TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("Award Lots of XP"));

		//========================================================================================
		//	Button_AwardLotsOfExperience

		Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 4.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_AwardExperienceToPlayer = Button;

		TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("Player XP"));

		//========================================================================================
		//	Button_GiveRandomInventoryItem

		Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 5.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_GiveRandomInventoryItem = Button;

		TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("Random Item"));

		//========================================================================================
		//	Button_GiveSpecificInventoryItem

		Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 6.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_GiveSpecificInventoryItem = Button;

		TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("Item"));

		//========================================================================================
		//	Button_GetAIControllerInfo

		Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button);

		ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
		ButtonSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
		ButtonSlot->SetPosition(FVector2D(-200.f, STARTING_Y_POSITION + Y_SPACING_BETWEEN_BUTTONS * 7.f));
		ButtonSlot->SetSize(BUTTON_SIZE);
		ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		Button_GetAIControllerInfo = Button;

		TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		Button->AddChild(TextBlock);
		TextBlock->SetText(FText::FromString("AI Info"));

		//========================================================================================
		//	Return value

		return Ref;
	}
	else
	{
		return Super::RebuildWidget();
	}
}

void UInMatchDeveloperWidget::UIBinding_OnDestroySelectableButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::DealMassiveDamageToSelectable);
#endif
}

void UInMatchDeveloperWidget::UIBinding_OnDamageSelectableButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::DamageSelectable);
#endif
}

void UInMatchDeveloperWidget::UIBinding_OnAwardExperienceButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::AwardExperience);
#endif
}

void UInMatchDeveloperWidget::UIBinding_OnAwardLotsOfExperienceButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::AwardLotsOfExperience);
#endif
}

void UInMatchDeveloperWidget::UIBinding_OnAwardExperienceToPlayerButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::AwardExperienceToLocalPlayer);
#endif
}

void UInMatchDeveloperWidget::UIBinding_OnGiveRandomItemButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::GiveRandomInventoryItem);
#endif
}

void UInMatchDeveloperWidget::UIBinding_OnGiveSpecificItemButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::GiveSpecificInventoryItem_SelectionPhase);
#endif
}

void UInMatchDeveloperWidget::UIBinding_OnGetUnitAIInfoButtonClicked()
{
#if WITH_EDITOR
	PC->OnDevelopmentWidgetRequest(EDevelopmentAction::GetUnitAIInfo);
#endif
}

bool UInMatchDeveloperWidget::IsPopupMenuShowing() const
{ 
	return (PopupMenuWidget != nullptr) && (PopupMenuWidget->GetVisibility() == ESlateVisibility::Visible);
}

void UInMatchDeveloperWidget::ShowPopupWidget(EDevelopmentAction PopupWidgetsPurpose)
{	
	if (PopupMenuWidget == nullptr)
	{
		// Create widget first
		PopupMenuWidget = CreateWidget<UDevelopmentPopupWidget>(PC, PopupMenuWidget_BP);
		PopupMenuWidget->SetupWidget(GI, PC);
		PopupMenuWidget->AddToViewport(ARTSPlayerController::GetWidgetZOrder(EMatchWidgetType::DevelopmentCheatsPopupMenu));
	}

	PopupMenuWidget->SetAppearanceFor(PopupWidgetsPurpose);

	PopupMenuWidget->SetVisibility(ESlateVisibility::Visible);
}

void UInMatchDeveloperWidget::HidePopupWidget()
{
	PopupMenuWidget->SetVisibility(ESlateVisibility::Hidden);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//================================================================================================
//	Popup Menu Widget
//================================================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////

const int32 UDevelopmentPopupWidget::NUM_GRID_PANEL_CHILDREN_PER_ROW = 8;

UDevelopmentPopupWidget::UDevelopmentPopupWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	Button_BP = UDevelopmentPopupWidgetButton::StaticClass();
}

bool UDevelopmentPopupWidget::IsEditorOnly() const
{
	return true;
}

TSharedRef<SWidget> UDevelopmentPopupWidget::RebuildWidget()
{
	if (GetClass() == StaticClass())
	{
		//========================================================================================
		// Create a default version of this widget to support 'out of the box' functionality 
		//========================================================================================

		//========================================================================================
		//	Root widget

		/* Assuming we don't already have a canvas panel root widget. If we do then we don't need
		to create this canvas panel */
		assert(GetRootWidget() == nullptr);
		UCanvasPanel * RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = RootWidget;

		//========================================================================================
		//	Super

		const TSharedRef <SWidget> Ref = Super::RebuildWidget();

		//========================================================================================
		//	Border for looks

		UBorder * Border = WidgetTree->ConstructWidget<UBorder>();
		RootWidget->AddChild(Border);

		UCanvasPanelSlot * BordersSlot = CastChecked<UCanvasPanelSlot>(Border->Slot);
		BordersSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		BordersSlot->SetPosition(FVector2D(400.f, 0.f));
		BordersSlot->SetSize(FVector2D(96.f * 8.f, 96.f * 8.f));
		BordersSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		
		/* Make border look prettier */
		Border->SetPadding(FMargin(20.f, 20.f, 20.f, 20.f));
		Border->SetBrushColor(FLinearColor(1.f, 0.2f, 0.2f, 0.6f));

		//========================================================================================
		//	Scroll box cause there could be a lot of children of grid

		UScrollBox * ScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
		Border->AddChild(ScrollBox);

		UBorderSlot * ScrollBoxsSlot = CastChecked<UBorderSlot>(ScrollBox->Slot);
		ScrollBoxsSlot->SetHorizontalAlignment(HAlign_Fill);
		ScrollBoxsSlot->SetVerticalAlignment(VAlign_Fill);

		//========================================================================================
		//	Panel_Selections

		Panel_Selections = WidgetTree->ConstructWidget<UGridPanel>();
		ScrollBox->AddChild(Panel_Selections);

		//========================================================================================
		//	Return value

		return Ref;
	}
	else
	{
		return Super::RebuildWidget();
	}
}

void UDevelopmentPopupWidget::OnWidgetAddedToSelectionsPanel(UGridSlot * GridSlot, UDevelopmentPopupWidgetButton * JustAdded)
{
	/* Position child at right spot. By default UGridPanel::AddChildToGrid actually just plonks 
	all the children on top of each other */

	const int32 Column = (Panel_Selections->GetChildrenCount() + 1) % NUM_GRID_PANEL_CHILDREN_PER_ROW;
	const int32 Row = (Panel_Selections->GetChildrenCount() + 1) / NUM_GRID_PANEL_CHILDREN_PER_ROW;
	GridSlot->SetColumn(Column);
	GridSlot->SetRow(Row);
}

void UDevelopmentPopupWidget::SetAppearanceFor(EDevelopmentAction Action)
{	
	if (Action == EDevelopmentAction::GiveSpecificInventoryItem_SelectionPhase)
	{
		/* Check if panel is a grid. If not then do nothing which isn't very good */
		if (Panel_Selections->IsA(UGridPanel::StaticClass()))
		{
			UGridPanel * Grid = CastChecked<UGridPanel>(Panel_Selections);

			for (uint8 i = 0; i < Statics::NUM_INVENTORY_ITEM_TYPES; ++i)
			{
				UWidget * Child = Panel_Selections->GetChildAt(i);
				UDevelopmentPopupWidgetButton * ChildAsButtonWidget;
				if (Child == nullptr)
				{
					// Create widget first
					ChildAsButtonWidget = CreateWidget<UDevelopmentPopupWidgetButton>(PC, Button_BP);
					
					UGridSlot * GridSlot = Grid->AddChildToGrid(ChildAsButtonWidget);

					OnWidgetAddedToSelectionsPanel(GridSlot, ChildAsButtonWidget);

					ChildAsButtonWidget->SetupWidget(GI, PC);
				}
				else
				{
					ChildAsButtonWidget = CastChecked<UDevelopmentPopupWidgetButton>(Child);
				}

				const EInventoryItem ItemType = Statics::ArrayIndexToInventoryItem(i);
				const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(ItemType);
				ChildAsButtonWidget->SetupFor(ItemType, *ItemsInfo);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//================================================================================================
//	Single Popup Menu Entry
//================================================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////

UDevelopmentPopupWidgetButton::UDevelopmentPopupWidgetButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	ClickFunctionality = EButtonClickBehavior::Nothing;
}

bool UDevelopmentPopupWidgetButton::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}
	
	if (IsWidgetBound(Button_Button))
	{
		Button_Button->OnClicked.AddDynamic(this, &UDevelopmentPopupWidgetButton::UIBinding_OnButtonClicked);
	}

	return false;
}

bool UDevelopmentPopupWidgetButton::IsEditorOnly() const
{
	return true;
}

TSharedRef<SWidget> UDevelopmentPopupWidgetButton::RebuildWidget()
{
	if (GetClass() == StaticClass())
	{
		//========================================================================================
		// Create a default version of this widget to support 'out of the box' functionality 
		//========================================================================================

		//========================================================================================
		//	Root widget

		assert(GetRootWidget() == nullptr);
		UCanvasPanel * RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = RootWidget;

		//========================================================================================
		//	Super

		const TSharedRef <SWidget> Ref = Super::RebuildWidget();

		//========================================================================================
		//	Button_Button

		Button_Button = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button_Button);

		UCanvasPanelSlot * ButtonsSlot = CastChecked<UCanvasPanelSlot>(Button_Button->Slot);
		ButtonsSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		ButtonsSlot->SetPosition(FVector2D(0.f, 0.f));
		ButtonsSlot->SetSize(FVector2D(96.f, 96.f));
		ButtonsSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		//========================================================================================
		//	Image_Image

		Image_Image = WidgetTree->ConstructWidget<UImage>();
		Button_Button->AddChild(Image_Image);

		UButtonSlot * ImagesSlot = CastChecked<UButtonSlot>(Image_Image->Slot);
		ImagesSlot->SetHorizontalAlignment(HAlign_Fill);
		ImagesSlot->SetVerticalAlignment(VAlign_Fill);

		//========================================================================================
		//	Return value

		return Ref;
	}
	else
	{
		return Super::RebuildWidget();
	}
}

void UDevelopmentPopupWidgetButton::UIBinding_OnButtonClicked()
{
#if WITH_EDITOR
	if (ClickFunctionality == EButtonClickBehavior::GiveInventoryItem)
	{
		PC->OnDevelopmentWidgetRequest(EDevelopmentAction::GiveSpecificInventoryItem_SelectTarget, ClickAuxilleryInfo);
	}
#endif
}

void UDevelopmentPopupWidgetButton::SetupFor(EInventoryItem ItemType, const FInventoryItemInfo & ItemsInfo)
{
	ClickFunctionality = EButtonClickBehavior::GiveInventoryItem;
	ClickAuxilleryInfo = Statics::InventoryItemToArrayIndex(ItemType);

	// Set image
	if (IsWidgetBound(Image_Image))
	{
		Image_Image->SetBrush(ItemsInfo.GetDisplayImage());
	}
}
