// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemInstance.generated.h"

class UItemDefinition;

/**
 * 아이템의 동적 데이터를 담당하는 클래스입니다.
 */
UCLASS(NotBlueprintable, BlueprintType)
class GAMEPLAYLAB_API UItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
	/** UObject Replicate를 위해 오버라이드합니다. */
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetItemDefinition(const TSubclassOf<UItemDefinition> InItemDefinition);
	void SetQuantity(uint64 InQuantity);
	
	const TSubclassOf<UItemDefinition>& GetItemDefinition() const;
	const UItemDefinition* GetItemDefinitionCDO() const;
	uint64 GetQuantity() const;
	
private:
	UFUNCTION()
	void OnRep_ItemDefinition();
	
	UFUNCTION()
	void OnRep_Quantity();
	
private:
	/**
	 * ItemInstance가 복제되었다고 해서 ItemDefinition 참조도 확실하다고 단정할 수 없습니다.
	 * 클라이언트에선 서버에게 데이터를 받기 전까지 nullptr일 수 있습니다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ItemDefinition)
	TSubclassOf<UItemDefinition> ItemDefinition;
	
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Quantity)
	uint64 Quantity = 1;
};
