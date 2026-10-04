// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemManagerSubsystem.generated.h"

class UItemDefinition;
class UDataTable;

/**
 * 인게임에 사용할 아이템 정보를 가진 Subsystem입니다.
 */
UCLASS(Config = Game)
class GAMEPLAYLAB_API UItemManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	TSubclassOf<UItemDefinition> GetItemDefinitionById(uint64 ItemId) const;
	const TMap<uint64, TSoftClassPtr<UItemDefinition>>& GetItemDefinitionTableMap() const;
	
	/** 랜덤 드랍 아이템 생성 테스트를 위해 구현한 함수입니다. */
	TSubclassOf<UItemDefinition> GetRandomItemDefinition() const;
	
private:
	void CacheItemDefinitionTable();

	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> ItemDefinitionTable;
	
	UPROPERTY(Transient)
	TMap<uint64, TSoftClassPtr<UItemDefinition>> ItemDefinitionTableMap;
};
