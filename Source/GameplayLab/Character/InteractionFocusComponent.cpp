// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractionFocusComponent.h"

#include "GameplayLab.h"
#include "Camera/CameraComponent.h"
#include "Interface/InteractableInterface.h"

UInteractionFocusComponent::UInteractionFocusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
}

void UInteractionFocusComponent::SetCameraComponent(UCameraComponent* InCameraComponent)
{
	CameraComponent = InCameraComponent;
	
	// 자신의 컴포넌트만 Tick을 활성화합니다.
	SetComponentTickEnabled(IsValid(InCameraComponent));
}

void UInteractionFocusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();
	const UCameraComponent* Camera = CameraComponent.Get();
	if (!World || !Camera)
	{
		SetInteractableActor(nullptr);
		return;
	}
	
	// 카메라 위치와 정면 방향으로 일정 거리 떨어진 위치를 계산합니다.
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * InteractionDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	
	// 두 위치로 ECC_Interact 라인트레이스해서 검출 여부와 액터 상태에 따라 하이라이팅을 수행합니다.
	AActor* NewInteractableActor = nullptr;
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Interact, QueryParams))
	{
		AActor* HitActor = Hit.GetActor();
		if (IsValid(HitActor) && Cast<IInteractableInterface>(HitActor))
		{
			NewInteractableActor = HitActor;
		}
	}

	SetInteractableActor(NewInteractableActor);
}

void UInteractionFocusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetInteractableActor(nullptr);

	Super::EndPlay(EndPlayReason);
}

AActor* UInteractionFocusComponent::GetInteractableActor() const
{
	return InteractableActor.Get();
}

void UInteractionFocusComponent::SetInteractableActor(AActor* NewInteractableActor)
{
	// 기존과 동일한 액터라면 얼리리턴합니다.
	if (InteractableActor.Get() == NewInteractableActor)
	{
		return;
	}

	// 다른 액터라면 기존 액터 하이라이팅을 제거합니다.
	if (IInteractableInterface* PreviousInteractableActor = Cast<IInteractableInterface>(InteractableActor.Get()))
	{
		PreviousInteractableActor->UnHighlight();
	}
	
	if (IInteractableInterface* CurrentInteractableActor = Cast<IInteractableInterface>(NewInteractableActor))
	{
		InteractableActor = NewInteractableActor;
		CurrentInteractableActor->Highlight();
	}
	else
	{
		InteractableActor.Reset();
	}
}
