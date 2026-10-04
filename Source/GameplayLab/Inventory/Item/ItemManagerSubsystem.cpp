// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemManagerSubsystem.h"

#include "Engine/DataTable.h"
#include "ItemDefinition.h"

void UItemManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CacheItemDefinitionTable();
}

void UItemManagerSubsystem::Deinitialize()
{
	ItemDefinitionTableMap.Reset();

	Super::Deinitialize();
}

TSubclassOf<UItemDefinition> UItemManagerSubsystem::GetItemDefinitionById(const uint64 ItemId) const
{
	if (const TSoftClassPtr<UItemDefinition>* ItemDefinition = ItemDefinitionTableMap.Find(ItemId))
	{
		return ItemDefinition->LoadSynchronous();
	}

	return nullptr;
}

void UItemManagerSubsystem::CacheItemDefinitionTable()
{
	ItemDefinitionTableMap.Reset();

	const UDataTable* LoadedItemDefinitionTable = ItemDefinitionTable.LoadSynchronous();
	if (!LoadedItemDefinitionTable)
	{
		ensureMsgf(false, TEXT("DT_ItemDefinition를 불러오지 못했습니다."));
		return;
	}

	if (LoadedItemDefinitionTable->GetRowStruct() != FItemDefinitionRow::StaticStruct())
	{
		ensureMsgf(false, TEXT("DT_ItemDefinition의 행 구조체가 FItemDefinitionRow가 아닙니다."));
		return;
	}

	TArray<FItemDefinitionRow*> ItemDefinitionRows;
	LoadedItemDefinitionTable->GetAllRows(TEXT("CacheItemDefinitionTable"), ItemDefinitionRows);

	for (const FItemDefinitionRow* ItemDefinitionRow : ItemDefinitionRows)
	{
		if (!ItemDefinitionRow || ItemDefinitionRow->ItemDefinition.IsNull())
		{
			continue;
		}

		if (ItemDefinitionTableMap.Contains(ItemDefinitionRow->ItemId))
		{
			ensureMsgf(false, TEXT("DT_ItemDefinition에 중복된 ItemId가 있습니다: %llu"), ItemDefinitionRow->ItemId);
			continue;
		}

		ItemDefinitionTableMap.Add(ItemDefinitionRow->ItemId, ItemDefinitionRow->ItemDefinition);
	}
}

const TMap<uint64, TSoftClassPtr<UItemDefinition>>& UItemManagerSubsystem::GetItemDefinitionTableMap() const
{
	return ItemDefinitionTableMap;
}

TSubclassOf<UItemDefinition> UItemManagerSubsystem::GetRandomItemDefinition() const
{
	if (ItemDefinitionTableMap.IsEmpty())
	{
		return nullptr;
	}

	int32 RandomIndex = FMath::RandRange(0, ItemDefinitionTableMap.Num() - 1);
	for (const auto& Pair : ItemDefinitionTableMap)
	{
		if (RandomIndex-- == 0)
		{
			return Pair.Value.LoadSynchronous();
		}
	}

	return nullptr;
}
