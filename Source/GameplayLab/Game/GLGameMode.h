// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GLGameMode.generated.h"

class ADroppedItem;
class AGLCharacter;

UCLASS()
class GAMEPLAYLAB_API AGLGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AGLGameMode();

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	
	/** 월드에 랜덤한 드랍 아이템을 스폰하는 테스트용 함수입니다. */
	void SpawnRandomItem() const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "CharacterClass")
	TSubclassOf<AGLCharacter> WarriorCharacter;
	
	UPROPERTY(EditDefaultsOnly, Category = "CharacterClass")
	TSubclassOf<AGLCharacter> WizardCharacter;
	
	UPROPERTY(EditDefaultsOnly, Category = "DroppedItem")
	TSubclassOf<ADroppedItem> DroppedItemClass;
};
