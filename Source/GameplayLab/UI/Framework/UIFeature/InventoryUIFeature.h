// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GLGameUIFeature.h"
#include "InventoryUIFeature.generated.h"

class UGLActivatableWidget;

UCLASS()
class GAMEPLAYLAB_API UInventoryUIFeature : public UGLGameUIFeature
{
	GENERATED_BODY()
	
public:
	void ToggleInventory();
	
protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UGLActivatableWidget> ScreenInventoryWidgetClass;
	
	TWeakObjectPtr<UGLActivatableWidget> ScreenInventory;
};
