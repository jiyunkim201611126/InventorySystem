// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLUserWidget.h"

void UGLUserWidget::SetWidgetController(UGLWidgetController* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}

void UGLUserWidget::WidgetControllerSet_Implementation()
{
}
