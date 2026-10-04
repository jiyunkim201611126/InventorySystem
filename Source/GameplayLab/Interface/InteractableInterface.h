// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

/**
 * 하이라이팅과 인터랙션을 담당합니다.
 * 필요하다면 하이라이팅 전용 인터페이스를 분리할 수 있습니다.
 */
UINTERFACE()
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class GAMEPLAYLAB_API IInteractableInterface
{
	GENERATED_BODY()

public:
	virtual void Highlight() = 0;
	virtual void UnHighlight() = 0;
	virtual bool CanInteract(AActor* Interactor);
	virtual void Interact(AActor* Interactor) = 0;
};
