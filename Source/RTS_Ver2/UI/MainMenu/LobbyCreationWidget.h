// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"
#include "LobbyCreationWidget.generated.h"

class UTextBlock;
class UButton;
class UEditableText;
class UImage;
class UMapInfoWidget;
class UMatchRulesWidget;


/**
 *	Widget for setting up lobby for a networked match. Things like whether it's LAN, max
 *	number of players etc can be configured
 */
UCLASS(Abstract)
class RTS_VER2_API ULobbyCreationWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool Setup() override;

protected:

	virtual void SetupBindings();

	virtual void SetDefaultValues();

	void SetMap(const FMapInfo * MapInfo);

	/** 
	 *	When opening the map selection widget, whether to try show the player start location 
	 *	widgets on each map info image. Also affects the map image of the currently selected map 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bTryShowPlayerStartsOnMaps;

	/* Button to create lobby */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Create;

	/* Button to return to previous menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	/* Text for what to call lobby */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableText * Text_LobbyName;

	/* LAN, online */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NetworkType;

	/* Change network type button */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_NetworkTypeLeft;

	/* Another change network type button */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_NetworkTypeRight;

	/* Enter password here for lobby. If left blank then lobby will have no password */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableText * Text_Password;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NumSlots;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_DecreaseNumSlots;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_IncreaseNumSlots;

	/* Widget for the rules of the match */
	UPROPERTY(meta = (BindWidgetOptional))
	UMatchRulesWidget * Widget_MatchRules;

	/* Widget that shows info about current map */
	UPROPERTY(meta = (BindWidgetOptional))
	UMapInfoWidget * Widget_SelectedMap;

	/* Button to open widget to select map */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_ChangeMap;

	UFUNCTION()
	void UIBinding_OnCreateButtonClicked();

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();

	UFUNCTION()
	void UIBinding_OnLobbyNameTextChanged(const FText & NewText);

	UFUNCTION()
	void UIBinding_OnNetworkTypeLeftButtonClicked();

	UFUNCTION()
	void UIBinding_OnNetworkTypeRightButtonClicked();

	UFUNCTION()
	void UIBinding_OnPasswordTextChanged(const FText & NewText);

	UFUNCTION()
	void UIBinding_OnDecreaseNumSlotsButtonClicked();

	UFUNCTION()
	void UIBinding_OnIncreaseNumSlotsButtonClicked();

	UFUNCTION()
	void UIBinding_OnChangeMapButtonClicked();

	/* Given some FText return whether the network type it represents is for LAN or not */
	static bool IsTextForLAN(const FText & InText);

	/* Given some FText return whether the network type it represents is for online */
	static bool IsTextForOnline(const FText & InText);

public:

	/* Lets another widget update what map info is displayed */
	virtual void UpdateMapDisplay(const FMapInfo & MapInfo) override;
};
