// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemInstance.h"

#include "ItemDefinition.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemInstance, ItemDefinition);
	DOREPLIFETIME(UItemInstance, Quantity);
}

void UItemInstance::SetItemDefinition(const TSubclassOf<UItemDefinition> InItemDefinition)
{
	if (!ensureMsgf(!ItemDefinition && InItemDefinition, TEXT("ItemDefinition은 유효한 값으로 한 번만 설정할 수 있습니다.")))
	{
		return;
	}

	ItemDefinition = InItemDefinition;
}

void UItemInstance::OnRep_ItemDefinition()
{
	if (UInventoryComponent* InventoryComponent = GetTypedOuter<UInventoryComponent>())
	{
		InventoryComponent->OnItemDefinitionReplicated(this);
	}
}

void UItemInstance::OnRep_Quantity()
{
	if (UInventoryComponent* InventoryComponent = GetTypedOuter<UInventoryComponent>())
	{
		InventoryComponent->OnItemQuantityReplicated(this);
	}
}

void UItemInstance::SetQuantity(const uint64 InQuantity)
{
	Quantity = InQuantity;
}

const TSubclassOf<UItemDefinition>& UItemInstance::GetItemDefinition() const
{
	return ItemDefinition;
}

const UItemDefinition* UItemInstance::GetItemDefinitionCDO() const
{
	return ItemDefinition.GetDefaultObject();
}

uint64 UItemInstance::GetQuantity() const
{
	return Quantity;
}
