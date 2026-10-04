// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemDataParser/ItemTableToolMenu.h"

#include "Modules/ModuleManager.h"
#include "Templates/UniquePtr.h"

class FGameplayLabEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		ItemTableToolMenu = MakeUnique<FItemTableToolMenu>();
		ItemTableToolMenu->Register();
	}

	virtual void ShutdownModule() override
	{
		if (ItemTableToolMenu)
		{
			ItemTableToolMenu->Unregister();
			ItemTableToolMenu.Reset();
		}
	}

private:
	TUniquePtr<FItemTableToolMenu> ItemTableToolMenu;
};

IMPLEMENT_MODULE(FGameplayLabEditorModule, GameplayLabEditor)
