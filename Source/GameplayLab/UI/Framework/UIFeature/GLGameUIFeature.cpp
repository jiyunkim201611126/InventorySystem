// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLGameUIFeature.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "UI/Framework/GLPrimaryGameLayout.h"

void UGLGameUIFeature::Initialize(UGLPrimaryGameLayout* InLayoutWidget)
{
	LayoutWidget = InLayoutWidget;
	
	OnInitialized();
}

void UGLGameUIFeature::Deinitialize()
{
	OnDeinitialized();
	
	LayoutWidget.Reset();
}

void UGLGameUIFeature::OnInitialized()
{
}

void UGLGameUIFeature::OnDeinitialized()
{
}

UWorld* UGLGameUIFeature::GetWorld() const
{
	if (LayoutWidget.IsValid())
	{
		return LayoutWidget->GetWorld();
	}
	return Super::GetWorld();
}
