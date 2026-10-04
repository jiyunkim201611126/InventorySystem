// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "DroppedItem.generated.h"

class UItemInstance;
class UItemDefinition;
class USphereComponent;
struct FStreamableHandle;

UCLASS()
class GAMEPLAYLAB_API ADroppedItem : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ADroppedItem();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	
	/** 서버에서 ItemInstance를 생성하고 초기화합니다. 한 번만 호출할 수 있습니다. */
	void SetItemInstance(const TSubclassOf<UItemDefinition>& InItemDefinitionClass, uint64 InQuantity = 1);
	
	//~ Begin IInteraction Interface
	virtual void Highlight() override;
	virtual void UnHighlight() override;
	virtual bool CanInteract(AActor* Interactor) override;
	virtual void Interact(AActor* Interactor) override;
	//~ End of IInteraction Interface
	
	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	UFUNCTION()
	void OnRep_ItemInstance();
	
	void InitializeDropMesh();
	void LoadDropMesh(const TSoftObjectPtr<UStaticMesh>& DropMesh);
	void OnDropMeshLoaded(const TSoftObjectPtr<UStaticMesh> DropMesh);
	
	bool CanPickupItem(const AActor* PlayerCharacterActor) const;
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> DropMeshComponent;
	
	/** 레벨 배치용 초기 설정입니다. */
	UPROPERTY(EditAnywhere, Category = "Item")
	TSubclassOf<UItemDefinition> InitialItemDefinitionClass;
	
	UPROPERTY(EditAnywhere, Category = "Item", meta = (ClampMin = "1"))
	uint64 InitialQuantity = 1;
	
private:
	UPROPERTY(ReplicatedUsing = OnRep_ItemInstance)
	TObjectPtr<UItemInstance> ItemInstance;
	
	TSharedPtr<FStreamableHandle> DropMeshLoadHandle;
	
	uint8 bIsHighlighting : 1 = false;
	float HighlightElapsedTime = 0.0f;
	FVector OriginScale = FVector::OneVector;
};
