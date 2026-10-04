// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryComponent.generated.h"

class UItemInstance;
class UItemDefinition;
class UInventoryComponent;

/**
 * 인벤토리 아이템 배열의 원소가 되는 구조체입니다.
 */
USTRUCT()
struct GAMEPLAYLAB_API FInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	int32 GetSlotIndex() const;

private:
	friend struct FInventoryList;

	/**
	 * Entry가 복제되었다고 해서 ItemInstance의 참조도 확실하다고 단정할 수 없습니다.
	 * 클라이언트에선 서버에게 데이터를 받기 전까지 nullptr일 수 있습니다.
	 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UItemInstance> ItemInstance;

	/** 삭제/참조 교체 시 기존 검색 데이터를 제거하기 위한 로컬 캐시입니다. */
	UPROPERTY(NotReplicated, Transient)
	TSubclassOf<UItemDefinition> CachedItemDefinition;
	UPROPERTY(NotReplicated, Transient)
	TWeakObjectPtr<UItemInstance> CachedItemInstance;
	
	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;
	
	/** 위치 변경 시 기존 위치를 파악하기 위한 로컬 캐시입니다. */
	int32 CachedSlotIndex = INDEX_NONE;
};

/**
 * 인벤토리 아이템 배열입니다.
 */
USTRUCT()
struct GAMEPLAYLAB_API FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);

	/** 복제로 추가/삭제/변경된 항목만 검색 캐시에 반영합니다. */
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

	const TArray<FInventoryItemEntry>& GetEntries() const;
	int32 Num() const;

private:
	friend class UInventoryComponent;
	
	uint64 AddItem(UInventoryComponent* InOwnerComponent, const TSubclassOf<UItemDefinition>& InItemDefinition, const uint64 InQuantity, const int32 MaxItemSlots, const UItemInstance* TemplateInstance = nullptr);
	bool RemoveItem(UInventoryComponent* InOwnerComponent, UItemInstance* InItemInstance);
	bool MoveItem(const int32 StartSlotIndex, const int32 DestinationSlotIndex, const int32 MaxItemSlots);

	bool CanAddItem(const TSubclassOf<UItemDefinition>& InItemDefinition, uint64 InQuantity, int32 MaxItemSlots) const;
	bool ContainsItemInLookup(const UItemInstance* InItemInstance) const;

	/**
	 * Entry의 ItemInstance 참조와 ItemInstance 내부의 ItemDefinition 참조는 클라이언트에서 즉시 해결된다고 보장되지 않습니다.
	 * 복제 직후 nullptr일 가능성이 있기 때문에, 이 부분을 포함해서 갱신 로직을 작성했습니다.
	 * 
	 * Entry 또는 ItemInstance가 준비되면 FastArray Replicate 콜백에서 갱신하고,
	 * ItemDefinition이 나중에 준비되면 RepNotify를 통해 다시 갱신합니다.
	 * TMultiMap은 Entries로부터 파생된 캐시이므로, 실제 Entry가 확인된 경우에만 등록합니다.
	 */
	void UpdateEntryLookup(FInventoryItemEntry& Entry);
	void UpdateEntryLookup(const UItemInstance* InItemInstance);
	void RemoveEntryLookup(FInventoryItemEntry& Entry);
	
	FInventoryItemEntry* FindEntry(const UItemInstance* InItemInstance);
	FInventoryItemEntry* FindEntry(const int32 SlotIndex);
	
private:
	int32 FindAvailableSlotIndex(const int32 MaxItemSlots) const;

private:
	TWeakObjectPtr<UInventoryComponent> OwnerComponent;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FInventoryItemEntry> Entries;
	
	/** 검색용 캐시입니다. 실제 객체는 Entries가 참조합니다. */
	TMultiMap<TSubclassOf<UItemDefinition>, TWeakObjectPtr<UItemInstance>> ItemDefinitionToItemInstances;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};


DECLARE_MULTICAST_DELEGATE_TwoParams(FInventoryItemAdded, const UItemInstance*, const int32 SlotIndex);
DECLARE_MULTICAST_DELEGATE_TwoParams(FInventoryItemRemoved, const UItemInstance*, const int32 SlotIndex);
DECLARE_MULTICAST_DELEGATE_TwoParams(FInventoryItemChanged, const UItemInstance*, const int32 SlotIndex);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FInventoryItemMoved, const UItemInstance*, const int32 StartSlotIndex, const int32 DestinationSlotIndex);

UCLASS(NotBlueprintable)
class GAMEPLAYLAB_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/**
	 * 수용 가능한 만큼 추가하고 실제 추가한 수량을 반환합니다. 실패하면 0을 반환합니다.
	 * 전달받은 인스턴스의 수량은 변경하지 않으며, 새 스택이 필요하면 복사해 저장합니다.
	 */
	uint64 AddItem(const UItemInstance* InItemInstance);
	uint64 AddItem(const TSubclassOf<UItemDefinition>& InItemDefinition, uint64 InQuantity = 1);
	
	bool RemoveItem(UItemInstance* InItemInstance);
	
	bool MoveItem(const int32 StartSlotIndex, const int32 DestinationSlotIndex);
	
	/** 요청 수량 중 1개라도 추가할 공간이 있으면 true를 반환합니다. */
	bool CanAddItem(const UItemInstance* InItemInstance) const;
	bool CanAddItem(const TSubclassOf<UItemDefinition>& InItemDefinition, uint64 InQuantity = 1) const;
	
	/** FastArray 콜백 이후 ItemDefinition 참조가 해결된 경우 캐시 갱신을 다시 시도합니다. */
	void OnItemDefinitionReplicated(const UItemInstance* InItemInstance);
	void OnItemQuantityReplicated(const UItemInstance* InItemInstance);

	const TArray<FInventoryItemEntry>& GetInventoryItems() const;
	
private:
	uint64 AddItem_Internal(TSubclassOf<UItemDefinition> InItemDefinition, uint64 InQuantity, const UItemInstance* InItemInstance);
	
public:
	FInventoryItemAdded OnInventoryItemAdded;
	FInventoryItemRemoved OnInventoryItemRemoved;
	FInventoryItemChanged OnInventoryItemChanged;
	FInventoryItemMoved OnInventoryItemMoved;
	
private:
	UPROPERTY(Replicated, VisibleAnywhere)
	FInventoryList InventoryList;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxItemCapacity = 60;
};
