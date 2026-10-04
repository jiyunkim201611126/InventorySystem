// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionFocusComponent.generated.h"

class IInteractableInterface;
class UCameraComponent;

UCLASS()
class GAMEPLAYLAB_API UInteractionFocusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionFocusComponent();
	
	void SetCameraComponent(UCameraComponent* InCameraComponent);
	
	//~ Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of UActorComponent Interface
	
	AActor* GetInteractableActor() const;
	
private:
	void SetInteractableActor(AActor* NewInteractableActor);

	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
	float InteractionDistance = 500.0f;

	TWeakObjectPtr<UCameraComponent> CameraComponent;
	TWeakObjectPtr<AActor> InteractableActor;
};
