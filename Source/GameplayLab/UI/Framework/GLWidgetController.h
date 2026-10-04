// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GLWidgetController.generated.h"

class APlayerController;

/** WidgetController 초기화에 사용하는 범용 참조입니다. */
USTRUCT(BlueprintType)
struct FGLWidgetControllerParams
{
	GENERATED_BODY()

	FGLWidgetControllerParams() = default;
	FGLWidgetControllerParams(APlayerController* InPlayerController)
		: PlayerController(InPlayerController)
	{
	}

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerController> PlayerController;
};

/**
 * View와 Model 사이의 바인딩을 담당하는, ViewModel 역할의 범용 WidgetController입니다.
 */
UCLASS(Abstract, Blueprintable)
class GAMEPLAYLAB_API UGLWidgetController : public UObject
{
	GENERATED_BODY()

public:
	virtual void SetWidgetControllerParams(const FGLWidgetControllerParams& WidgetControllerParams);
	virtual void BindCallbacks();

	/** 생성 직후 View에 초기값을 표시할 때 사용합니다. */
	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValue();

	APlayerController* GetPlayerController() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerController> PlayerController;
};
