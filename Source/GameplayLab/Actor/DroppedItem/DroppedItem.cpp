// Fill out your copyright notice in the Description page of Project Settings.

#include "DroppedItem.h"

#include "Character/GLCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/AssetManager.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/Item/ItemDefinition.h"
#include "Inventory/Item/ItemInstance.h"
#include "Inventory/Item/Fragment/ItemFragment_Droppable.h"
#include "Inventory/Item/Fragment/ItemFragment_Equippable.h"
#include "Net/UnrealNetwork.h"
#include "Player/GLPlayerController.h"

ADroppedItem::ADroppedItem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;
	
	InteractComponent = CreateDefaultSubobject<USphereComponent>(TEXT("InteractComponent"));
	SetRootComponent(InteractComponent);
	
	DropMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DropMeshComponent"));
	DropMeshComponent->SetupAttachment(InteractComponent);
}

void ADroppedItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADroppedItem, ItemInstance);
}

bool ADroppedItem::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	if (CanPickupItem(ViewTarget))
	{
		return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
	}
	return false;
}

void ADroppedItem::SetItemInstance(const TSubclassOf<UItemDefinition>& InItemDefinitionClass, const uint64 InQuantity)
{
	if (!ensureMsgf(HasAuthority(), TEXT("ItemInstance는 서버에서만 생성할 수 있습니다."))
		|| !ensureMsgf(!IsValid(ItemInstance), TEXT("DroppedItem의 ItemInstance는 한 번만 설정할 수 있습니다."))
		|| !ensureMsgf(InItemDefinitionClass && InQuantity > 0, TEXT("유효한 ItemDefinition과 1 이상의 Quantity가 필요합니다.")))
	{
		return;
	}

	ItemInstance = NewObject<UItemInstance>(this);
	ItemInstance->SetItemDefinition(InItemDefinitionClass);
	ItemInstance->SetQuantity(InQuantity);
	AddReplicatedSubObject(ItemInstance);
}

void ADroppedItem::BeginPlay()
{
	Super::BeginPlay();
	
	if (!HasAuthority())
	{
		return;
	}

	// 레벨에 직접 배치한 경우 에디터에서 설정한 초기값으로 ItemInstance를 생성합니다.
	if (!IsValid(ItemInstance))
	{
		if (!InitialItemDefinitionClass || InitialQuantity == 0)
		{
			Destroy();
			return;
		}

		SetItemInstance(InitialItemDefinitionClass, InitialQuantity);
	}

	if (!IsValid(ItemInstance))
	{
		Destroy();
		return;
	}

	InitializeDropMesh();
}

void ADroppedItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 비동기 로드가 진행 중이었다면 중단합니다.
	if (DropMeshLoadHandle.IsValid())
	{
		DropMeshLoadHandle->CancelHandle();
		DropMeshLoadHandle.Reset();
	}
	
	Super::EndPlay(EndPlayReason);
}

void ADroppedItem::OnRep_ItemInstance()
{
	InitializeDropMesh();
}

void ADroppedItem::InitializeDropMesh()
{
	if (!IsValid(ItemInstance) || ItemInstance->GetQuantity() == 0)
	{
		return;
	}

	// ItemDefinition의 CDO를 통해 드롭 아이템을 표시할 메쉬를 가져옵니다.
	const UItemDefinition* DefinitionCDO = ItemInstance->GetItemDefinitionCDO();
	if (!DefinitionCDO)
	{
		return;
	}

	const FItemFragment_Droppable* Droppable = DefinitionCDO->FindFragment<FItemFragment_Droppable>();
	if (!Droppable || Droppable->DropMesh.IsNull())
	{
		if (HasAuthority())
		{
			Destroy();
		}
		return;
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	OriginScale = Droppable->MeshScale;
	DropMeshComponent->SetRelativeScale3D(OriginScale);

	// 메쉬 비동기 로드를 시작합니다.
	LoadDropMesh(Droppable->DropMesh);
}

void ADroppedItem::LoadDropMesh(const TSoftObjectPtr<UStaticMesh>& DropMesh)
{
	if (UStaticMesh* LoadedMesh = DropMesh.Get())
	{
		// 이미 로드가 되어있다면 바로 메쉬를 할당합니다.
		DropMeshComponent->SetStaticMesh(LoadedMesh);
		return;
	}
	
	DropMeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(DropMesh.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::OnDropMeshLoaded, DropMesh));
}

void ADroppedItem::OnDropMeshLoaded(const TSoftObjectPtr<UStaticMesh> DropMesh)
{
	if (UStaticMesh* LoadedMesh = DropMesh.Get())
	{
		DropMeshComponent->SetStaticMesh(LoadedMesh);
	}
	
	DropMeshLoadHandle.Reset();
}

void ADroppedItem::Highlight()
{
	if (bIsHighlighting)
	{
		return;
	}

	bIsHighlighting = true;
	HighlightElapsedTime = 0.0f;
	DropMeshComponent->SetRelativeScale3D(OriginScale);
	SetActorTickEnabled(true);
}

void ADroppedItem::UnHighlight()
{
	bIsHighlighting = false;
	HighlightElapsedTime = 0.0f;
	DropMeshComponent->SetRelativeScale3D(OriginScale);
	SetActorTickEnabled(false);
}

bool ADroppedItem::CanInteract(AActor* Interactor)
{
	if (const APawn* InteractorPawn = Cast<APawn>(Interactor))
	{
		if (const AGLPlayerController* PlayerController = InteractorPawn->GetController<AGLPlayerController>())
		{
			if (const UInventoryComponent* InventoryComponent = PlayerController->GetInventoryComponent())
			{
				return CanPickupItem(Interactor) && InventoryComponent->CanAddItem(ItemInstance);
			}
		}
	}
	return false;
}

void ADroppedItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// 간단하게 하이라이팅을 구현했습니다.
	if (bIsHighlighting)
	{
		constexpr float PulsePeriod = 1.0f;
		constexpr float AddScale = 0.1f;

		// 매 프레임 시간 누적과 함께 PulsePeriod로 나눈 나머지를 저장합니다. (0.f ~ PulsePeriod 반복)
		HighlightElapsedTime = FMath::Fmod(HighlightElapsedTime + DeltaSeconds, PulsePeriod);
		
		// cos 함수로 -1 ~ 1까지 왕복하는 값을 만들고, 이를 통해 0 ~ 1까지 왕복하는 값을 만듭니다.
		const float PulseAlpha = (1.0f - FMath::Cos(2.0f * PI * HighlightElapsedTime)) * 0.5f;
		
		// 결과적으로 가까이 가서 화면 중앙에 놓으면 커졌다 작아졌다 하는 메쉬를 볼 수 있습니다.
		const FVector Scale = FMath::Lerp(OriginScale, OriginScale + AddScale, PulseAlpha);
		DropMeshComponent->SetRelativeScale3D(Scale);
	}
}

void ADroppedItem::Interact(AActor* Interactor)
{
	// 이미 네트워크 관련성을 검사했지만, 상호작용 시에도 한 번 검증합니다.
	if (!CanPickupItem(Interactor))
	{
		return;
	}
	
	// 서버에서 캐릭터가 실제 습득 가능 거리에 있는지 검증합니다.
	constexpr float MaxPickupDistance = 300.f;
	if (FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) > FMath::Square(MaxPickupDistance))
	{
		return;
	}
	
	const APawn* InteractorPawn = Cast<APawn>(Interactor);
	if (!InteractorPawn)
	{
		return;
	}

	const AGLPlayerController* PlayerController = Cast<AGLPlayerController>(InteractorPawn->GetController());
	if (!PlayerController)
	{
		return;
	}
	
	if (UInventoryComponent* InventoryComponent = PlayerController->GetInventoryComponent())
	{
		const uint64 AddedQuantity = InventoryComponent->AddItem(ItemInstance);
		const uint64 RemainingQuantity = ItemInstance->GetQuantity() - AddedQuantity;
		ItemInstance->SetQuantity(RemainingQuantity);
		if (RemainingQuantity == 0)
		{
			Destroy();
		}
		else if (AddedQuantity > 0)
		{
			ForceNetUpdate();
		}
	}
}

bool ADroppedItem::CanPickupItem(const AActor* PlayerCharacterActor) const
{
	if (!IsValid(ItemInstance) || ItemInstance->GetQuantity() == 0)
	{
		return false;
	}
	
	const UItemDefinition* ItemDefinition = ItemInstance->GetItemDefinitionCDO();
	if (!ItemDefinition)
	{
		return false;
	}
	
	const FItemFragment_Equippable* Equippable = ItemDefinition->FindFragment<FItemFragment_Equippable>();
	if (!Equippable)
	{
		// 장착 아이템이 아닌 경우 true를 반환합니다.
		return true;
	}
	
	const EClassType ItemClassType = Equippable->ClassType;
	if (ItemClassType == EClassType::None)
	{
		// 공용 아이템인 경우 true를 반환합니다.
		return true;
	}
	
	const AGLCharacter* PlayerCharacter = Cast<AGLCharacter>(PlayerCharacterActor);
	if (!PlayerCharacter || PlayerCharacter->GetClassType() != ItemClassType)
	{
		// 아이템의 클래스와 캐릭터의 클래스가 일치하지 않는다면 false를 반환합니다.
		return false;
	}
	
	return true;
}
