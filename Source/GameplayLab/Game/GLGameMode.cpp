// Fill out your copyright notice in the Description page of Project Settings.

#include "GLGameMode.h"

#include "Actor/DroppedItem/DroppedItem.h"
#include "Character/GLCharacter.h"
#include "Inventory/Item/ItemManagerSubsystem.h"
#include "Player/GLPlayerController.h"

AGLGameMode::AGLGameMode()
{
	PlayerControllerClass = AGLPlayerController::StaticClass();
}

UClass* AGLGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// 테스트용으로 간편하게 홀수번째 접속 클라이언트는 Warrior, 짝수번째 접속 클라이언트는 Wizard를 반환합니다.
	const int32 PlayerCount = GetNumPlayers();
	if (PlayerCount % 2 == 1)
	{
		return WarriorCharacter.Get();
	}
	
	return WizardCharacter.Get();
}

void AGLGameMode::SpawnRandomItem() const
{
	if (!DroppedItemClass)
	{
		return;
	}
	
	const UItemManagerSubsystem* ItemManager = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
	if (!ItemManager)
	{
		return;
	}
	
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(FVector(0.f, 0.f, 50.f));
	if (ADroppedItem* DroppedItem = GetWorld()->SpawnActorDeferred<ADroppedItem>(DroppedItemClass, SpawnTransform))
	{
		DroppedItem->SetItemInstance(ItemManager->GetRandomItemDefinition());
		DroppedItem->FinishSpawning(SpawnTransform);
	}
}
