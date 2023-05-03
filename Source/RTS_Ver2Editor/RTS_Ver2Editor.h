#pragma once

class UEditorUtilityWidget;


/** 
 *	This module was created to create the editor utility widget for the editor play settings. 
 *	I did not end up using it for that though or anything really.
 */
class FRTS_Ver2EditorModule : public IModuleInterface
{
public:
	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};
