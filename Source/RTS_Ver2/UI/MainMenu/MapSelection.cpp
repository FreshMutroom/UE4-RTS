// Fill out your copyright notice in the Description page of Project Settings.

#include "MapSelection.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"

#include "Statics/Statics.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerController.h"
#include "Statics/DevelopmentStatics.h"


//=============================================================================================
//	Player Start Widget
//=============================================================================================

UPlayerStartWidget::UPlayerStartWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedLobbySlot = -1;
}

bool UPlayerStartWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Text_PlayerIndex))
	{
		Text_PlayerIndex->SetText(FText::GetEmpty());
	}

	if (IsWidgetBound(Button_Button))
	{
		Button_Button->OnClicked.AddDynamic(this, &UPlayerStartWidget::UIBinding_OnButtonClicked);
	}

	return false;
}

void UPlayerStartWidget::SetupForPlayerSpot(uint8 InPlayerStartUniqueID, FVector2D ScreenLocation, bool bIsClickable)
{
	PlayerStartUniqueID = InPlayerStartUniqueID;
	
	// Change location on screen
	SetPositionInViewport(ScreenLocation);

	if (IsWidgetBound(Button_Button))
	{
		if (bIsClickable)
		{
			Button_Button->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Button_Button->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (!IsInViewport())
	{
		/* Reasoning behind ZOrder: just want to to appear above map image. Is not some precise
		number and can probably be reduced */
		AddToViewport(UINT8_MAX + 1);
	}
}

void UPlayerStartWidget::UIBinding_OnButtonClicked()
{
	/* ULobbyWidget::SetAreSlotsLocked should make it so this button is never clickable when 
	slots are locked */
	
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());

	/* Check if starting spot either is empty or already belongs to us */
	if (AssignedLobbySlot == -1)
	{
		// Slot belongs to no one. Assign ourselves to it
		PlayCon->Server_ChangeStartingSpotInLobby(PlayerStartUniqueID);
	}
	else if (AssignedLobbySlot == PlayCon->GetLobbySlotIndex())
	{
		// Slot already has us assigned. Behavior here is to unassign us by setting param to -1
		PlayCon->Server_ChangeStartingSpotInLobby(-1);
	}
	else
	{
		/* Assigned to either a CPU player or another human player. Do nothing */
	}

	/* When server replicates change then actually update appearance */
}

bool UPlayerStartWidget::IsHost() const
{
	return GetWorld()->IsServer();
}

void UPlayerStartWidget::SetAssignedPlayer(int16 PlayerIndex)
{
	AssignedLobbySlot = PlayerIndex;
	
	if (IsWidgetBound(Text_PlayerIndex))
	{
		if (PlayerIndex == -1)
		{
			// -1 is code for unassign
			Text_PlayerIndex->SetText(FText::GetEmpty());
		}
		else
		{
			// Add 1 so lobby slot 1 player will show a 1. Assuming PlayerIndex is 0 indexed
			Text_PlayerIndex->SetText(FText::AsNumber(PlayerIndex + 1));
		}
	}
}

int16 UPlayerStartWidget::GetAssignedPlayer() const
{
	return AssignedLobbySlot;
}


//=============================================================================================
//	Single Map Info Widget
//=============================================================================================

UMapInfoWidget::UMapInfoWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UMapInfoWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	SetupBindings();

	if (IsWidgetBound(Text_Description))
	{
		Text_Description->SetIsReadOnly(true);
		// This is to remove "I" mouse cursor showing when hovering
		Text_Description->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	return false;
}

void UMapInfoWidget::SetRefToOwningWidget(UMapSelectionWidget * ParentWidget)
{
	MapSelectionWidget = ParentWidget;
}

void UMapInfoWidget::SetMap(const FMapInfo * InMapInfo, EPlayerStartDisplay PlayerStartRule)
{
	MapInfo = InMapInfo;

	// Set display name 
	if (IsWidgetBound(Text_Name))
	{
		Text_Name->SetText(MapInfo->GetDisplayName());
	}
	
	// Set user-specified image
	if (IsWidgetBound(Image_Custom))
	{
		Image_Custom->SetBrushFromTexture(InMapInfo->GetUserDefinedImage());
	}

	// Set minimap image.
	if (IsWidgetBound(Image_Minimap))
	{
		UTexture2D * MinimapImage = MapInfo->GetMinimapTexture();
	
		// Would like to promote this from warning to fatal eventually
		UE_CLOG(MinimapImage == nullptr, RTSLOG, Warning, TEXT("Minimap image null"));

		Image_Minimap->SetBrushFromTexture(MinimapImage);

		// Optionally draw the player start locations overtop of the image
		if (PlayerStartRule == EPlayerStartDisplay::VisibleButUnclickable 
			|| PlayerStartRule == EPlayerStartDisplay::VisibleAndClickable)
		{ 
			const bool bMakeButtonsClickable = (PlayerStartRule == EPlayerStartDisplay::VisibleAndClickable);
			
			SetMinimapLocationValues();

			const FVector2D ImageWidgetLoc = MinimapImageScreenLoc;
			const FVector2D ImageWidgetDim = MinimapImageScreenDim;
			const FBoxSphereBounds & MapBounds = InMapInfo->GetMapBounds();

			const TArray < FPlayerStartInfo > & PlayerStarts = InMapInfo->GetPlayerStarts();

			// Create player start widgets overtop of the image - one for each location
			for (uint8 i = 0; i < PlayerStarts.Num(); ++i)
			{
				const FPlayerStartInfo & Info = PlayerStarts[i];
				const uint8 PlayerStartID = Info.GetUniqueID();
				
				UE_CLOG(PlayerStartID != i, RTSLOG, Fatal, TEXT("ID mismatch: %d and %d. "
				"Player start unique IDs in the level bounds are probably not unique"), PlayerStartID, i);

				UPlayerStartWidget * Widget;

				if (PlayerStartWidgets.IsValidIndex(i) && PlayerStartWidgets[i] != nullptr)
				{
					// Reuse an already created widget
					Widget = PlayerStartWidgets[i];
				}
				else
				{
					// Create a new widget
					Widget = CreateWidget<UPlayerStartWidget>(GI, PlayerStartWidget_BP);
					
					// Put in array
					PlayerStartWidgets.Emplace(Widget);

					Widget->Setup();
				}

				//-----------------------------------------------------------------
				//	Figure out the screen location widget should be placed at
				//-----------------------------------------------------------------

				/* @See UMinimap::WorldCoordsToMinimapCoords for similar logic. */

				/* Figure out percentage across map point is. Range 0 ... 1 
				(0, 0) = top left corner, (1, 1) = bottom right corner
				This code will probably not work if the level bounds has non-zero rotation */
				const FVector2D Percentage = FVector2D((Info.GetLocation() - MapBounds.Origin + MapBounds.BoxExtent) / (MapBounds.BoxExtent * 2.f));

				// Figure out location on screen. 
				const FVector2D ScreenLocation = ImageWidgetLoc + (ImageWidgetDim / Percentage);

				Widget->SetupForPlayerSpot(Info.GetUniqueID(), ScreenLocation, bMakeButtonsClickable);
			}

			bIsDisplayingPlayerStarts = true;
		}
		else
		{
			bIsDisplayingPlayerStarts = false;
		}
	}

	// Set discription text
	if (IsWidgetBound(Text_Description))
	{
		Text_Description->SetText(MapInfo->GetDescription());
	}
	
	// Set max players text
	if (IsWidgetBound(Text_MaxPlayers))
	{
		const FText MaxPlayersAsText = Statics::IntToText(MapInfo->GetMaxPlayers());
		Text_MaxPlayers->SetText(MaxPlayersAsText);
	}
}

const FText & UMapInfoWidget::GetSetMap() const
{
	return MapInfo->GetDisplayName();
}

uint8 UMapInfoWidget::GetSetMapID() const
{
	return MapInfo->GetUniqueID();
}

const FMapInfo * UMapInfoWidget::GetSetMapInfo() const
{
	return MapInfo;
}

void UMapInfoWidget::SetMinimapLocationValues()
{
	if (IsWidgetBound(Image_Minimap))
	{
		/** 
		 *	Right now user is limited to canvas panels as the only parents for Image_Minimap. 
		 *	GetCachedGeometry doesn't work as expected at both design time and run time and I 
		 *	currently don't know of any other way to get screen location of widget. 
		 */

		if (Image_Minimap->Slot->IsA(UCanvasPanelSlot::StaticClass()))
		{
			UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanelSlot>(Image_Minimap->Slot);

			// TODO remove hardcode
			const FVector2D AnchorOffset = CanvasSlot->GetAnchors().Maximum * FVector2D(1920.f, 1080.f);
			MinimapImageScreenLoc = AnchorOffset + CanvasSlot->GetPosition();
			MinimapImageScreenDim = CanvasSlot->GetSize();
		}
		else
		{
			UE_LOG(RTSLOG, Warning, TEXT("%s minimap image will have incorrect player start "
				"locations because not using canvas panel as its parent FIXME"), *GetClass()->GetName());

			// Assign garbage value
			MinimapImageScreenLoc = FVector2D::ZeroVector;
			MinimapImageScreenDim = FVector2D::ZeroVector;
		}
	}
}

void UMapInfoWidget::SetupBindings()
{
	if (IsWidgetBound(Button_Select))
	{
		Button_Select->OnClicked.AddUniqueDynamic(this, &UMapInfoWidget::UIBinding_OnSelectButtonClicked);
	}
}

void UMapInfoWidget::UIBinding_OnSelectButtonClicked()
{
	if (MapSelectionWidget != nullptr)
	{
		MapSelectionWidget->SetCurrentMap(MapInfo);
	}
}

void UMapInfoWidget::UpdatePlayerStartAssignments(const TArray<int16>& PlayerStartArray)
{
	if (IsWidgetBound(Image_Minimap))
	{
		if (bIsDisplayingPlayerStarts)
		{
			/* Map widget to what player should be assigned to it */
			TMap < UPlayerStartWidget * , uint8 > WidgetsWithPlayers;

			for (int32 i = 0; i < PlayerStartArray.Num(); ++i)
			{
				const int16 StartSpotIndex = PlayerStartArray[i];
				
				if (StartSpotIndex >= 0)
				{
					UPlayerStartWidget * Widget = PlayerStartWidgets[StartSpotIndex];

					UE_CLOG(WidgetsWithPlayers.Contains(Widget), RTSLOG, Fatal, TEXT("Trying to assign "
						"different players to the same starting spot"));

					WidgetsWithPlayers.Emplace(Widget, i);
				}
			}

			// Update widgets 
			for (const auto & Elem : PlayerStartWidgets)
			{
				if (WidgetsWithPlayers.Contains(Elem))
				{
					Elem->SetAssignedPlayer(WidgetsWithPlayers[Elem]);
				}
				else
				{
					Elem->SetAssignedPlayer(-1);
				}
			}
		}
	}
}

void UMapInfoWidget::GetStartingSpots(TArray<int16>& OutStartingSpots)
{
	assert(OutStartingSpots.Num() == 0);
	OutStartingSpots.Reserve(PlayerStartWidgets.Num());
	
	for (int32 i = 0; i < PlayerStartWidgets.Num(); ++i)
	{
		UPlayerStartWidget * Widget = PlayerStartWidgets[i];
		OutStartingSpots.Emplace(Widget->GetAssignedPlayer());
	}
}

#if WITH_EDITOR
void UMapInfoWidget::OnWidgetRebuilt()
{
	/* Bear in mind this is called while playing in PIE/Standalone but desired to be executed 
	during design time only */
	
	/* Originally had SetMinimapLocationValues() here but oddly enough this func is called 
	after the func SetMap() so pointless to leave it here */

	Super::OnWidgetRebuilt();
}
#endif // WITH_EDITOR


//=============================================================================================
//	Map Selection Widget
//=============================================================================================

UMapSelectionWidget::UMapSelectionWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowPlayerStartsOnMinimaps = true;
}

bool UMapSelectionWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	/* Main reason: to make description box uneditable */
	if (IsWidgetBound(Widget_CurrentSelectedMap))
	{
		Widget_CurrentSelectedMap->Setup();
	}

	/* Setup button functionality */
	SetupBindings();

	return false;
}

void UMapSelectionWidget::SetupMapList()
{
	if (bHasSetupMapInfoList)
	{
		return;
	}
	
	bHasSetupMapInfoList = true;

	const EPlayerStartDisplay PlayerStartRule = bShowPlayerStartsOnMinimaps
		? EPlayerStartDisplay::VisibleButUnclickable : EPlayerStartDisplay::Hidden;

	/* Both bool and widget being bound must be true to auto-populate panel. Otherwise it 
	is assumed user has created their list already in editor and that there are enough map 
	info widgets for each map in map pool */
	if (bAutoPopulateMapList && IsWidgetBound(Panel_MapList))
	{
		UE_CLOG(MapInfoWidget_BP == nullptr, RTSLOG, Fatal, TEXT("Map selection widget %s is trying "
			"to create map info widgets but no map info widget blueprint is set"),
			*GetClass()->GetName());
		
		for (const auto & Elem : GI->GetMapPool())
		{
			UMapInfoWidget * MapInfoWidget = CreateWidget<UMapInfoWidget>(GetWorld()->GetFirstPlayerController(),
				MapInfoWidget_BP);

			MapInfoWidget->Setup();
			MapInfoWidget->SetRefToOwningWidget(this);
			MapInfoWidget->SetMap(&Elem.Value, PlayerStartRule);
			
			Panel_MapList->AddChild(MapInfoWidget);

			// Let any custom behavior happen
			OnMapInfoAddedToPanel(MapInfoWidget);
		}
	}
	else
	{
		// Setup the widgets already created by user in editor

		/* First get all child widgets and filter the non-map slot widgets */
		TArray <UWidget *> Children;
		WidgetTree->GetAllWidgets(Children);

		TArray <UMapInfoWidget *> MapSlots;
		for (const auto & Widget : Children)
		{
			if (Widget->IsA(UMapInfoWidget::StaticClass()))
			{
				MapSlots.Emplace(CastChecked<UMapInfoWidget>(Widget));
			}
		}

		/* Now for each map slot give it a different map from the map pool */
		const TMap <FName, FMapInfo> & MapPool = GI->GetMapPool();
		int32 i = 0;
		for (const auto & Elem : MapPool)
		{
			if (MapSlots.IsValidIndex(i))
			{
				UE_CLOG(!MapSlots.IsValidIndex(i), RTSLOG, Fatal, TEXT("Map selection widget %s does "
					"not have enough map info widgets to display info for every map in map pool. "
					"Num created in editor: %d, num required: %d"),
					*GetClass()->GetName(), i, MapPool.Num());
				
				MapSlots[i]->Setup();
				MapSlots[i]->SetRefToOwningWidget(this);
				MapSlots[i]->SetMap(&Elem.Value, PlayerStartRule);
				i++;
			}
			else
			{
				break;
			}
		}
	}
}

void UMapSelectionWidget::SetupBindings()
{
	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddUniqueDynamic(this, &UMapSelectionWidget::UIBinding_OnReturnButtonClicked);
	}
}

void UMapSelectionWidget::OnMapInfoAddedToPanel(UMapInfoWidget * WidgetJustAdded)
{
}

void UMapSelectionWidget::UIBinding_OnReturnButtonClicked()
{
	UpdatedWidget->UpdateMapDisplay(*CurrentMapInfo);
	GI->ShowPreviousWidget();
}

void UMapSelectionWidget::SetCurrentMap(const FMapInfo * MapInfo)
{
	CurrentMapInfo = MapInfo;

	if (IsWidgetBound(Widget_CurrentSelectedMap))
	{
		const EPlayerStartDisplay PlayerStartRule = bShowPlayerStartsOnMinimaps
			? EPlayerStartDisplay::VisibleButUnclickable : EPlayerStartDisplay::Hidden;
		
		Widget_CurrentSelectedMap->SetMap(MapInfo, PlayerStartRule);
	}
}

void UMapSelectionWidget::SetRefToUpdatedWidget(UMainMenuWidgetBase * OwningWidget)
{
	UpdatedWidget = OwningWidget;
}
