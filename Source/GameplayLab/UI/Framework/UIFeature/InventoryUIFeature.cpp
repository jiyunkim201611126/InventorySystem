// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryUIFeature.h"

#include "UI/GLUITags.h"
#include "UI/Framework/GLActivatableWidget.h"
#include "UI/Framework/GLPrimaryGameLayout.h"

void UInventoryUIFeature::ToggleInventory()
{
	if (!LayoutWidget.IsValid())
	{
		return;
	}
	
	if (ScreenInventory.IsValid() && ScreenInventory->IsActivated())
	{
		ScreenInventory->DeactivateWidget();
		ScreenInventory.Reset();
		return;
	}
	
	ScreenInventory = LayoutWidget->PushWidgetToLayerStack<UGLActivatableWidget>(GLGameplayTags::UI_Layer_GameMenu, ScreenInventoryWidgetClass.LoadSynchronous());
}
