// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UWidgetAnimation;
class UUserWidget;


/**
 *	Utility functions for user interface
 */
class RTS_VER2_API UIUtilities
{
public:

	/** 
	 *	Finds a widget animation on a widget. Relatively slow. You need to make sure that WidgetAnimName 
	 *	includes the _INST part
	 *	
	 *	@return - widget anim or null if no widget anim with the specified name could be found
	 */
	static UWidgetAnimation * FindWidgetAnim(UUserWidget * Widget, const FName & WidgetAnimName);
};
