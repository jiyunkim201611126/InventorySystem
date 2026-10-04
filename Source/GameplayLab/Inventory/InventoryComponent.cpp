// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryComponent.h"

#include "GameFramework/Actor.h"
#include "Item/ItemDefinition.h"
#include "Item/ItemInstance.h"
#include "Net/UnrealNetwork.h"

int32 FInventoryItemEntry::GetSlotIndex() const
{
	return SlotIndex;
}

bool FInventoryList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
	return FastArrayDeltaSerialize<FInventoryItemEntry, FInventoryList>(Entries, DeltaParams, *this);
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		RemoveEntryLookup(Entries[Index]);
	}
	
	if (OwnerComponent.IsValid() && OwnerComponent->OnInventoryItemRemoved.IsBound())
	{
		for (const int32 Index : RemovedIndices)
		{
			OwnerComponent->OnInventoryItemRemoved.Broadcast(Entries[Index].ItemInstance, Entries[Index].SlotIndex);
		}
	}
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		UpdateEntryLookup(Entries[Index]);
	}
	
	if (OwnerComponent.IsValid())
	{
		for (const int32 Index : AddedIndices)
		{
			FInventoryItemEntry& Entry = Entries[Index];
			Entry.CachedSlotIndex = Entry.SlotIndex;
			
			if (OwnerComponent->OnInventoryItemAdded.IsBound())
			{
				OwnerComponent->OnInventoryItemAdded.Broadcast(Entry.ItemInstance, Entry.SlotIndex);
			}
		}
	}
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	// 뒤늦게 ItemInstance 참조가 해결되는 경우에도 이 콜백이 호출됩니다.
	for (const int32 Index : ChangedIndices)
	{
		FInventoryItemEntry& Entry = Entries[Index];
		
		if (OwnerComponent.IsValid())
		{
			if (Entry.CachedSlotIndex != Entry.SlotIndex)
			{
				// 위치 변경 시 들어오는 분기입니다.
				OwnerComponent->OnInventoryItemMoved.Broadcast(Entry.ItemInstance, Entry.CachedSlotIndex, Entry.SlotIndex);
			}
			else
			{
				OwnerComponent->OnInventoryItemChanged.Broadcast(Entry.ItemInstance, Entry.SlotIndex);
			}
		}
		
		Entry.CachedSlotIndex = Entry.SlotIndex;
		
		UpdateEntryLookup(Entries[Index]);
	}
}

const TArray<FInventoryItemEntry>& FInventoryList::GetEntries() const
{
	return Entries;
}

int32 FInventoryList::Num() const
{
	return Entries.Num();
}

uint64 FInventoryList::AddItem(UInventoryComponent* InOwnerComponent, const TSubclassOf<UItemDefinition>& InItemDefinition, const uint64 InQuantity, const int32 MaxItemSlots, const UItemInstance* TemplateInstance)
{
	// InventoryComponent에서 일부 수량이라도 추가할 수 있는지 확인한 뒤 호출하므로, 여기서는 검증하지 않습니다.
	const uint64 MaxStack = InItemDefinition.GetDefaultObject()->MaxStack;
	uint64 RemainingQuantity = InQuantity;
	TArray<TWeakObjectPtr<UItemInstance>> MatchingInstances;
	ItemDefinitionToItemInstances.MultiFind(InItemDefinition, MatchingInstances, true);

	// 인벤토리 내 동일 아이템을 순회합니다.
	for (const auto& MatchingInstance : MatchingInstances)
	{
		UItemInstance* ItemInstance = MatchingInstance.Get();
		if (!ItemInstance || ItemInstance->GetQuantity() >= MaxStack)
		{
			// 스택이 이미 꽉 찬 아이템은 생략합니다.
			continue;
		}

		// 넣을 수 있는 만큼 넣습니다.
		const uint64 AddedQuantity = FMath::Min(RemainingQuantity, MaxStack - ItemInstance->GetQuantity());
		ItemInstance->SetQuantity(ItemInstance->GetQuantity() + AddedQuantity);
		RemainingQuantity -= AddedQuantity;
		if (RemainingQuantity == 0)
		{
			return InQuantity;
		}
	}

	// 기존 아이템에 넣고 남은 수량이 있다면, 인벤토리가 꽉 찰 때까지 넣습니다.
	while (RemainingQuantity > 0 && Entries.Num() < MaxItemSlots)
	{
		const uint64 NewItemQuantity = FMath::Min(RemainingQuantity, MaxStack);
		UItemInstance* NewItemInstance;
		if (TemplateInstance)
		{
			NewItemInstance = DuplicateObject<UItemInstance>(TemplateInstance, InOwnerComponent);
		}
		else
		{
			NewItemInstance = NewObject<UItemInstance>(InOwnerComponent);
			NewItemInstance->SetItemDefinition(InItemDefinition);
		}
		NewItemInstance->SetQuantity(NewItemQuantity);
		
		const int32 NewSlotIndex = FindAvailableSlotIndex(MaxItemSlots);
		if (NewSlotIndex == INDEX_NONE)
		{
			break;
		}
		
		FInventoryItemEntry& NewEntry = Entries.AddDefaulted_GetRef();
		NewEntry.ItemInstance = NewItemInstance;
		NewEntry.SlotIndex = NewSlotIndex;

		// PlayerController의 복제 대상에게 ItemInstance도 함께 복제하도록 등록합니다.
		InOwnerComponent->AddReplicatedSubObject(NewItemInstance);
		
		// 배열에 새 항목이 추가됐음을 알리고, 서버의 MultiMap 검색 캐시에 새 항목을 반영합니다.
		MarkItemDirty(NewEntry);
		UpdateEntryLookup(NewEntry);
		RemainingQuantity -= NewItemQuantity;
	}

	return InQuantity - RemainingQuantity;
}

bool FInventoryList::RemoveItem(UInventoryComponent* InOwnerComponent, UItemInstance* InItemInstance)
{
	// Entry들 중 ItemInstance를 소유한 Entry를 탐색합니다.
	const int32 EntryIndex = Entries.IndexOfByPredicate(
		[InItemInstance](const FInventoryItemEntry& Entry)
		{
			return Entry.ItemInstance == InItemInstance;
		});

	if (EntryIndex == INDEX_NONE)
	{
		return false;
	}

	// 해당 Entry를 제거하기 전에 먼저 검색 캐시를 갱신합니다.
	RemoveEntryLookup(Entries[EntryIndex]);
	
	// ItemInstance를 클라이언트에서도 제거하라고 요청합니다.
	InOwnerComponent->DestroyReplicatedSubObjectOnRemotePeers(InItemInstance);
	
	// 배열에 항목이 제거되었음을 알립니다.
	Entries.RemoveAt(EntryIndex);
	MarkArrayDirty();
	return true;
}

bool FInventoryList::MoveItem(const int32 StartSlotIndex, const int32 DestinationSlotIndex, const int32 MaxItemSlots)
{
	if (StartSlotIndex == DestinationSlotIndex ||
		StartSlotIndex < 0 ||
		MaxItemSlots <= StartSlotIndex ||
		DestinationSlotIndex < 0 ||
		MaxItemSlots <= DestinationSlotIndex)
	{
		return false;
	}
	
	FInventoryItemEntry* StartEntry = FindEntry(StartSlotIndex);
	FInventoryItemEntry* DestinationEntry = FindEntry(DestinationSlotIndex);
	
	// 마우스로 드래그를 시작한 출발지 슬롯은 반드시 아이템이 존재해야 합니다.
	if (!StartEntry || !IsValid(StartEntry->ItemInstance))
	{
		return false;
	}
	
	// 목적지 슬롯이 빈 슬롯이라면 StartEntry의 SlotIndex를 변경합니다.
	if (!DestinationEntry)
	{
		StartEntry->SlotIndex = DestinationSlotIndex;
		MarkItemDirty(*StartEntry);
		return true;
	}
	
	// Entry는 존재하나 ItemInstance가 없는 경우는 없어야 합니다.
	if (!IsValid(DestinationEntry->ItemInstance))
	{
		return false;
	}
	
	UItemInstance* StartItemInstance = StartEntry->ItemInstance;
	UItemInstance* DestinationItemInstance = DestinationEntry->ItemInstance;
	if (StartItemInstance->GetItemDefinition() == DestinationItemInstance->GetItemDefinition())
	{
		// 출발 슬롯과 목적지 슬롯이 동일한 아이템인 경우 들어오는 분기입니다.
		const UItemDefinition* ItemDefinitionCDO = StartItemInstance->GetItemDefinitionCDO();
		if (!ItemDefinitionCDO || !OwnerComponent.IsValid())
		{
			return false;
		}
		
		const uint64 MaxStack = ItemDefinitionCDO->MaxStack;
		const uint64 StartQuantity = StartItemInstance->GetQuantity();
		const uint64 DestinationQuantity = DestinationItemInstance->GetQuantity();
		
		if (MaxStack <= DestinationQuantity)
		{
			return false;
		}
		
		const uint64 MovedQuantity = FMath::Min(StartQuantity, MaxStack - DestinationQuantity);
		const uint64 RemainingQuantity = StartQuantity - MovedQuantity;
		
		if (RemainingQuantity == 0)
		{
			// 목적지 슬롯으로 모든 아이템이 옮겨진 경우 들어오는 분기입니다.
			if (!RemoveItem(OwnerComponent.Get(), StartItemInstance))
			{
				return false;
			}
			
			DestinationItemInstance->SetQuantity(DestinationQuantity + MovedQuantity);
			return true;
		}
		
		// 목적지 슬롯으로 일부 아이템이 옮겨진 경우 여기로 내려옵니다.
		StartItemInstance->SetQuantity(RemainingQuantity);
		DestinationItemInstance->SetQuantity(DestinationQuantity + MovedQuantity);
		return true;
	}
	
	// 출발 슬롯과 목적지 슬롯에 서로 다른 아이템이 들어있는 경우 여기로 내려옵니다.
	StartEntry->SlotIndex = DestinationSlotIndex;
	DestinationEntry->SlotIndex = StartSlotIndex;
	
	MarkItemDirty(*StartEntry);
	MarkItemDirty(*DestinationEntry);
	return true;
}

bool FInventoryList::CanAddItem(const TSubclassOf<UItemDefinition>& InItemDefinition, const uint64 InQuantity, const int32 MaxItemSlots) const
{
	const UItemDefinition* Definition = InItemDefinition.GetDefaultObject();
	if (!Definition || Definition->MaxStack == 0 || InQuantity == 0)
	{
		// 비정상 데이터인 경우 false를 반환합니다.
		return false;
	}
	
	const uint64 MaxStack = Definition->MaxStack;
	
	// 동일 아이템이 있는지 확인합니다.
	TArray<TWeakObjectPtr<UItemInstance>> MatchingInstances;
	ItemDefinitionToItemInstances.MultiFind(InItemDefinition, MatchingInstances, true);

	// 기존 스택에 1개라도 들어갈 수 있다면 일부 수량을 추가할 수 있으므로 true를 반환합니다.
	for (const auto& MatchingInstance : MatchingInstances)
	{
		const UItemInstance* ItemInstance = MatchingInstance.Get();
		if (ItemInstance && ItemInstance->GetQuantity() < MaxStack)
		{
			return true;
		}
	}
	
	// 인벤토리 내에 동일 아이템이 없거나 기존 스택이 전부 찼다면 현재 인벤토리 공간을 검사해 반환합니다.
	return Entries.Num() < MaxItemSlots;
}

bool FInventoryList::ContainsItemInLookup(const UItemInstance* InItemInstance) const
{
	const TSubclassOf<UItemDefinition>& ItemDefinition = InItemInstance->GetItemDefinition();
	for (auto It = ItemDefinitionToItemInstances.CreateConstKeyIterator(ItemDefinition); It; ++It)
	{
		if (It.Value().Get() == InItemInstance)
		{
			return true;
		}
	}
	
	return false;
}

void FInventoryList::UpdateEntryLookup(FInventoryItemEntry& Entry)
{
	UItemInstance* ItemInstance = Entry.ItemInstance.Get();
	const TSubclassOf<UItemDefinition> ItemDefinition = IsValid(ItemInstance) ? ItemInstance->GetItemDefinition() : TSubclassOf<UItemDefinition>();

	// 기존 Entry가 참조하던 Instance나 Definition에 변경이 없다면, 검색용 MultiMap을 갱신할 필요가 없습니다.
	if (Entry.CachedItemDefinition == ItemDefinition && Entry.CachedItemInstance.Get() == ItemInstance)
	{
		return;
	}

	// 검색용 MultiMap과 캐시를 갱신합니다.
	RemoveEntryLookup(Entry);
	if (IsValid(ItemInstance) && ItemDefinition)
	{
		Entry.CachedItemDefinition = ItemDefinition;
		Entry.CachedItemInstance = ItemInstance;
		ItemDefinitionToItemInstances.Add(ItemDefinition, Entry.CachedItemInstance);
	}
}

void FInventoryList::UpdateEntryLookup(const UItemInstance* InItemInstance)
{
	// 이미 캐시에 등록된 ItemInstance는 생략합니다.
	if (!IsValid(InItemInstance) || ContainsItemInLookup(InItemInstance))
	{
		return;
	}
	
	FInventoryItemEntry* Entry = Entries.FindByPredicate(
		[InItemInstance](const FInventoryItemEntry& Entry)
		{
			return Entry.ItemInstance.Get() == InItemInstance;
		});
	
	if (Entry)
	{
		UpdateEntryLookup(*Entry);
	}
}

void FInventoryList::RemoveEntryLookup(FInventoryItemEntry& Entry)
{
	// 삭제할 Entry의 기존 검색용 정보를 제거합니다.
	if (Entry.CachedItemDefinition)
	{
		for (auto It = ItemDefinitionToItemInstances.CreateKeyIterator(Entry.CachedItemDefinition); It; ++It)
		{
			// Replicate되면 이미 Entry의 ItemInstance는 nullptr일 수 있습니다.
			// 따라서 TWeakObjectPtr로 하나 더 캐싱해두고, 객체 자체가 아닌 '무엇을 캐싱했었는가'를 확인해 제거합니다.
			if (It.Value().HasSameIndexAndSerialNumber(Entry.CachedItemInstance))
			{
				It.RemoveCurrent();
				break;
			}
		}
	}

	Entry.CachedItemDefinition = nullptr;
	Entry.CachedItemInstance.Reset();
}

FInventoryItemEntry* FInventoryList::FindEntry(const UItemInstance* InItemInstance)
{
	return Entries.FindByPredicate(
		[InItemInstance](const FInventoryItemEntry& Entry)
		{
			return Entry.ItemInstance.Get() == InItemInstance;
		});
}

FInventoryItemEntry* FInventoryList::FindEntry(const int32 SlotIndex)
{
	return Entries.FindByPredicate(
		[SlotIndex](const FInventoryItemEntry& Entry)
		{
			return Entry.SlotIndex == SlotIndex;
		});
}

int32 FInventoryList::FindAvailableSlotIndex(const int32 MaxItemSlots) const
{
	if (MaxItemSlots <= 0)
	{
		return INDEX_NONE;
	}
	
	TBitArray<> OccupiedSlots(false, MaxItemSlots);
	
	for (const FInventoryItemEntry& Entry : Entries)
	{
		if (OccupiedSlots.IsValidIndex(Entry.SlotIndex))
		{
			OccupiedSlots[Entry.SlotIndex] = true;
		}
	}
	
	return OccupiedSlots.Find(false);
}


UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);

	InventoryList.OwnerComponent = this;
	InventoryList.Entries.Reserve(MaxItemCapacity);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, InventoryList);
}

uint64 UInventoryComponent::AddItem(const UItemInstance* InItemInstance)
{
	if (!IsValid(InItemInstance))
	{
		return 0;
	}
	
	return AddItem_Internal(InItemInstance->GetItemDefinition(), InItemInstance->GetQuantity(), InItemInstance);
}

uint64 UInventoryComponent::AddItem(const TSubclassOf<UItemDefinition>& InItemDefinition, const uint64 InQuantity)
{
	return AddItem_Internal(InItemDefinition, InQuantity, nullptr);
}

uint64 UInventoryComponent::AddItem_Internal(const TSubclassOf<UItemDefinition> InItemDefinition, const uint64 InQuantity, const UItemInstance* InItemInstance)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		// 서버 권한이 없는 경우 0을 반환합니다.
		return 0;
	}
	
	// 아이템을 실제로 추가할 수 있는지 확인합니다.
	const bool bCanAdd = InItemInstance ? CanAddItem(InItemInstance) : CanAddItem(InItemDefinition, InQuantity);
	if (!bCanAdd)
	{
		return 0;
	}
	
	return InventoryList.AddItem(this, InItemDefinition, InQuantity, MaxItemCapacity, InItemInstance);
}

bool UInventoryComponent::RemoveItem(UItemInstance* InItemInstance)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !IsValid(InItemInstance))
	{
		return false;
	}

	return InventoryList.RemoveItem(this, InItemInstance);
}

bool UInventoryComponent::MoveItem(const int32 StartSlotIndex, const int32 DestinationSlotIndex)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}
	
	return InventoryList.MoveItem(StartSlotIndex, DestinationSlotIndex, MaxItemCapacity);
}

bool UInventoryComponent::CanAddItem(const UItemInstance* InItemInstance) const
{
	return IsValid(InItemInstance)
		&& !InventoryList.ContainsItemInLookup(InItemInstance)
		&& CanAddItem(InItemInstance->GetItemDefinition(), InItemInstance->GetQuantity());
}

bool UInventoryComponent::CanAddItem(const TSubclassOf<UItemDefinition>& InItemDefinition, const uint64 InQuantity) const
{
	return InventoryList.CanAddItem(InItemDefinition, InQuantity, MaxItemCapacity);
}

void UInventoryComponent::OnItemDefinitionReplicated(const UItemInstance* InItemInstance)
{
	InventoryList.UpdateEntryLookup(InItemInstance);
	
	if (const FInventoryItemEntry* Entry = InventoryList.FindEntry(InItemInstance))
	{
		OnInventoryItemChanged.Broadcast(InItemInstance, Entry->GetSlotIndex());
	}
}

void UInventoryComponent::OnItemQuantityReplicated(const UItemInstance* InItemInstance)
{
	if (const FInventoryItemEntry* Entry = InventoryList.FindEntry(InItemInstance))
	{
		OnInventoryItemChanged.Broadcast(InItemInstance, Entry->GetSlotIndex());
	}
}

const TArray<FInventoryItemEntry>& UInventoryComponent::GetInventoryItems() const
{
	return InventoryList.GetEntries();
}
