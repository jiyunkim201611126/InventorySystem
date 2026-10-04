// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLActivatableWidget.h"

void UGLActivatableWidget::SetWidgetController(UGLWidgetController* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}

void UGLActivatableWidget::WidgetControllerSet_Implementation()
{
}
