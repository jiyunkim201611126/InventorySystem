// Fill out your copyright notice in the Description page of Project Settings.

#include "GLPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Inventory/InventoryComponent.h"
#include "UI/GLUITags.h"
#include "UI/Framework/GLUIManagerSubsystem.h"
#include "UI/Framework/UIFeature/InventoryUIFeature.h"

AGLPlayerController::AGLPlayerController()
{
	bReplicateUsingRegisteredSubObjectList = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void AGLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}
	
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (InputSubsystem && InputMappingContext)
	{
		InputSubsystem->AddMappingContext(InputMappingContext, 0);
	}
	
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	
	EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Completed, this, &ThisClass::ToggleInventory);
}

void AGLPlayerController::ToggleInventory()
{
	if (const UGLUIManagerSubsystem* UIManagerSubsystem = GetGameInstance()->GetSubsystem<UGLUIManagerSubsystem>())
	{
		if (UInventoryUIFeature* Feature = UIManagerSubsystem->GetOrCreateUIFeature<UInventoryUIFeature>(GLGameplayTags::UI_Feature_Inventory))
		{
			Feature->ToggleInventory();
		}
	}
}

UInventoryComponent* AGLPlayerController::GetInventoryComponent() const
{
	return InventoryComponent;
}
