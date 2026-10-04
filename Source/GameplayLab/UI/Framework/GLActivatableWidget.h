// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GLActivatableWidget.generated.h"

class UGLWidgetController;

/**
 * 활성화되면 키보드나 게임패드 등의 입력을 직접 받을 수 있는 위젯 클래스입니다.
 */
UCLASS(Abstract)
class GAMEPLAYLAB_API UGLActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetController(UGLWidgetController* InWidgetController);

protected:
	UFUNCTION(BlueprintNativeEvent)
	void WidgetControllerSet();

public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UGLWidgetController> WidgetController;
};
