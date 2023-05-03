// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/*
//========================================================================================
//	Migrating RTS project to this project
//========================================================================================
//========================================================================================

 
	- Have updated .uproject file, target.cs, editorTarget.cs and build.cs

	- FIXME: make sure to check bookmarks in old project




//========================================================================================
//	Tests to make sure project packages correctly
//========================================================================================


	- .uproject, build.cs, target.cs and editorTarget.cs all copied and map folder structure created: PASS
	
	- Added development statics C++ file: PASS

	- Added level volume (previously fog volume) to map:

	- Copied basically all code over to this project: FAIL



//============================================================================================
//	History of some steps taken to try and get packaged game to run
//============================================================================================


	- Created clone of 4.20 project. Clone ended up being a non-C++ clone or something... 
	visual studio was not openable from the editor. This project actually ran and crashed 
	instantly because "RTS" module or something was not found. The project was also renamed 
	in the cloning process so that might be why.

	I never got round to creating a clone with the exact same name to try and see if it would 
	fully work but the fact that I actually got a crash message ment this was progress. 

	- Next step: created a fresh project called RTS_Ver2 (this project). Copied small amounts 
	of code to it and it packaged OK. So then proceeded to copy all code from original to here. 
	Packaged game still does not run.

	- Tried cloning this project... packaged games do not run.
	- Tried cloning the original RTS project... NOW it does not run. But before I got a crash 
	screen... strange. Also all the clones now have visual studio access via editor so that must 
	have something to do with it.




//=============================================================================================
//	Consideration list before completing
//=============================================================================================

	- Check where I cast to ISelectable and see if some of those can be removed since have 
	switched to storing ABuilding and AInfantry pointers in the PS building/unit arrays

	- After looking at a talk that went on about hashmaps, something like 
	if (MyTMap.Contains(Something) == false) MyTMap.Emplace(Something) calcs the hash twice 
	and is apparently slower (thought compiler would optimize that). So take away is perhaps 
	remove the need for COntains calls and just prefill the TMap with all the key values. I 
	think ARTSPlayerState might have a few cases like this 

	- Also on the topic of TMaps, consider making as many TMaps as possible WITH_EDITORONLY_DATA 
	and having them actually have their contexts be inside a TArray for performance 

	- Consider writing a custom TArray<uint8> class that serializes it in such a way that 
	the ArrayNum is only a uint8. Can use this for PC commands and abilities
*/


//-----------------------------------------------------------------------------------------------
//===============================================================================================
//	Engine source changes
//===============================================================================================
//-----------------------------------------------------------------------------------------------

// Search "RTSProject" in code for them. 
// Off the top of my head they are:
// - changed some access specifiers in ReplicationGraph.h from private to protected/public
