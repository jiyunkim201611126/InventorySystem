// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GLPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UInventoryComponent;

/**
 * 소유 플레이어의 개인 인벤토리를 관리하는 PlayerController입니다.
 */
UCLASS()
class GAMEPLAYLAB_API AGLPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGLPlayerController();

	UInventoryComponent* GetInventoryComponent() const;
	
protected:
	//~ Begin APlayerController Interface
	virtual void SetupInputComponent() override;
	//~ End of APlayerController Interface
	
private:
	void ToggleInventory();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInventoryComponent> InventoryComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InventoryAction;
};
