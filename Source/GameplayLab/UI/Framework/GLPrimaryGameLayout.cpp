// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLPrimaryGameLayout.h"

#include "Engine/GameInstance.h"
#include "UI/Framework/GLUIManagerSubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UGLPrimaryGameLayout* UGLPrimaryGameLayout::GetPrimaryGameLayout(const APlayerController* PlayerController)
{
	return PlayerController ? GetPrimaryGameLayout(PlayerController->GetLocalPlayer()) : nullptr;
}

UGLPrimaryGameLayout* UGLPrimaryGameLayout::GetPrimaryGameLayout(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return nullptr;
	}

	if (const UGameInstance* GameInstance = LocalPlayer->GetGameInstance())
	{
		if (UGLUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UGLUIManagerSubsystem>())
		{
			if (UGLGameUIPolicy* Policy = UIManager->GetCurrentUIPolicy())
			{
				return Policy->GetOrCreateRootLayout(LocalPlayer);
			}
		}
	}
	return nullptr;
}

UCommonActivatableWidgetContainerBase* UGLPrimaryGameLayout::GetLayerWidget(const FGameplayTag LayerTag) const
{
	return Layers.FindRef(LayerTag);
}

void UGLPrimaryGameLayout::RegisterLayer(const FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget)
{
	if (!IsDesignTime() && ensure(LayerTag.IsValid()) && ensure(LayerWidget))
	{
		LayerWidget->OnTransitioningChanged.AddUObject(this, &ThisClass::OnWidgetStackTransitioning);
		LayerWidget->SetTransitionDuration(0.f);
		Layers.Add(LayerTag, LayerWidget);
	}
}

void UGLPrimaryGameLayout::OnWidgetStackTransitioning(UCommonActivatableWidgetContainerBase* Widget, bool bIsTransitioning)
{
}
