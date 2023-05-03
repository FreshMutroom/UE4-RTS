#include "RTS_Ver2Editor.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

#include "UI/EditorWidgets/EditorPlaySettingsWidget.h"
#include "Statics/DevelopmentStatics.h"

IMPLEMENT_GAME_MODULE(FRTS_Ver2EditorModule, RTS_Ver2Editor);


/*
=================================================================================================== 
 *	A note to self: 
 *	I may be wrong on this but the module is not reloaded on each hot reload, meaning if you 
 *	make changes here you will have to close then restart the editor to see the changes take effect 
===================================================================================================
*/

void FRTS_Ver2EditorModule::StartupModule()
{
}

void FRTS_Ver2EditorModule::ShutdownModule()
{
}
