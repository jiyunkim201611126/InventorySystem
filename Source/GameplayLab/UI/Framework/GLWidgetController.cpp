// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLWidgetController.h"

void UGLWidgetController::SetWidgetControllerParams(const FGLWidgetControllerParams& WidgetControllerParams)
{
	PlayerController = WidgetControllerParams.PlayerController;
}

void UGLWidgetController::BindCallbacks()
{
}

void UGLWidgetController::BroadcastInitialValue()
{
}

APlayerController* UGLWidgetController::GetPlayerController() const
{
	return PlayerController;
}
