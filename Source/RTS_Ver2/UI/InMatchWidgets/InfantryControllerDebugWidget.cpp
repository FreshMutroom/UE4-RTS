// Fill out your copyright notice in the Description page of Project Settings.


#include "InfantryControllerDebugWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

#include "MapElements/AIControllers/InfantryController.h"
#include "Statics/DevelopmentStatics.h"


TSharedRef<SWidget> UInfantryControllerDebugWidget::RebuildWidget()
{
	TSharedRef<SWidget> Ref = Super::RebuildWidget();

	if (GetClass() == StaticClass())
	{
		//---------------------------------------------------------------
		//	Root widget
		//---------------------------------------------------------------

		assert(GetRootWidget() == nullptr);
		UCanvasPanel * RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = RootCanvas;

		//---------------------------------------------------------------
		//	Text_State
		//---------------------------------------------------------------

		Text_State = WidgetTree->ConstructWidget<UTextBlock>();
		RootCanvas->AddChild(Text_State);
		UCanvasPanelSlot * Slot_Text_State = CastChecked<UCanvasPanelSlot>(Text_State->Slot);
		Slot_Text_State->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_Text_State->SetPosition(FVector2D(0.0f, 0.0f));
		Slot_Text_State->SetSize(FVector2D(200.f, 50.f));
		Slot_Text_State->SetAlignment(FVector2D(0.0f, 0.0f));

		//---------------------------------------------------------------
		//	Text_AnimState
		//---------------------------------------------------------------

		Text_AnimState = WidgetTree->ConstructWidget<UTextBlock>();
		RootCanvas->AddChild(Text_AnimState);
		UCanvasPanelSlot * Slot_Text_AnimState = CastChecked<UCanvasPanelSlot>(Text_AnimState->Slot);
		Slot_Text_AnimState->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_Text_AnimState->SetPosition(Slot_Text_State->GetPosition() + FVector2D(0.f, Slot_Text_State->GetSize().Y));
		Slot_Text_AnimState->SetSize(FVector2D(200.f, 50.f));
		Slot_Text_AnimState->SetAlignment(FVector2D(0.0f, 0.0f));

		//---------------------------------------------------------------
		//	Text_NumFailedMoves
		//---------------------------------------------------------------
		
		//Text_NumFailedMoves = WidgetTree->ConstructWidget<UTextBlock>();
		//RootCanvas->AddChild(Text_NumFailedMoves);
		//UCanvasPanelSlot * Slot_Text_NumFailedMoves = CastChecked<UCanvasPanelSlot>(Text_NumFailedMoves->Slot);
		//Slot_Text_NumFailedMoves->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		//Slot_Text_NumFailedMoves->SetPosition(Slot_Text_AnimState->GetPosition() + FVector2D(0.f, Slot_Text_AnimState->GetSize().Y));
		//Slot_Text_NumFailedMoves->SetSize(FVector2D(200.f, 50.f));
		//Slot_Text_NumFailedMoves->SetAlignment(FVector2D(0.0f, 0.0f));
	}

	return Ref;
}

void UInfantryControllerDebugWidget::SetInitialValues(AInfantryController * AIController)
{
	if (IsWidgetBound(Text_State))
	{
		Text_State->SetText(FText::FromString(ENUM_TO_STRING(EUnitState, AIController->GetUnitState())));
	}
	if (IsWidgetBound(Text_AnimState))
	{
		Text_AnimState->SetText(FText::FromString(ENUM_TO_STRING(EUnitAnimState, AIController->GetUnitAnimState())));
	}
}
