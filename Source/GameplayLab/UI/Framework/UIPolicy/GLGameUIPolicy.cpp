// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLGameUIPolicy.h"

#include "Framework/Application/SlateApplication.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "UI/Framework/GLPrimaryGameLayout.h"
#include "UI/Framework/GLUIManagerSubsystem.h"

UGLGameUIPolicy* UGLGameUIPolicy::GetGameUIPolicy(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	
	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGLUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UGLUIManagerSubsystem>())
			{
				return UIManager->GetCurrentUIPolicy();
			}
		}
	}
	return nullptr;
}

UGLUIManagerSubsystem* UGLGameUIPolicy::GetOwningUIManager() const
{
	return Cast<UGLUIManagerSubsystem>(GetOuter());
}

UGLPrimaryGameLayout* UGLGameUIPolicy::GetOrCreateRootLayout(ULocalPlayer* LocalPlayer)
{
	if (!IsValid(RootLayout))
	{
		CreateLayoutWidget(LocalPlayer);
	}
	return RootLayout;
}

UGLPrimaryGameLayout* UGLGameUIPolicy::GetRootLayout() const
{
	return RootLayout;
}

void UGLGameUIPolicy::CreateLayoutWidget(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return;
	}

	// RootLayout의 유효성을 확인하고, 없다면 생성합니다.
	if (!IsValid(RootLayout))
	{
		RootLayout = nullptr;
		
		const TSubclassOf<UGLPrimaryGameLayout> LoadedLayoutWidgetClass = RootLayoutWidgetClass.LoadSynchronous();
		if (ensure(LoadedLayoutWidgetClass && !LoadedLayoutWidgetClass->HasAnyClassFlags(CLASS_Abstract)))
		{
			if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld()))
			{
				RootLayout = CreateWidget<UGLPrimaryGameLayout>(PlayerController, LoadedLayoutWidgetClass);
			}
		}
	}

	// RootLayout 유효성과 별개로 Viewport에 추가됐는지 확인합니다.
	if (IsValid(RootLayout))
	{
		const UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld());
		if (!ViewportSubsystem || !ViewportSubsystem->IsWidgetAdded(RootLayout))
		{
			AddLayoutToViewport(LocalPlayer, RootLayout);
		}
	}
	
	if (IsValid(RootLayout))
	{
		const UGLUIManagerSubsystem* UIManagerSubsystem = GetOwningUIManager();
		if (!UIManagerSubsystem)
		{
			return;
		}
		
		for (const auto& StartUIFeatureTag : StartUIFeatureTags)
		{
			ensureAlwaysMsgf(UIManagerSubsystem->GetOrCreateUIFeature<UGLGameUIFeature>(StartUIFeatureTag), TEXT("UIFeature 생성 실패했습니다. UIFeatureTag: %s"), *StartUIFeatureTag.ToString());
		}
	}
}

void UGLGameUIPolicy::AddLayoutToViewport(ULocalPlayer* LocalPlayer, UGLPrimaryGameLayout* Layout)
{
	if (!LocalPlayer || !Layout)
	{
		return;
	}

	Layout->SetPlayerContext(FLocalPlayerContext(LocalPlayer));
	Layout->AddToPlayerScreen(1000);
	OnRootLayoutAddedToViewport(LocalPlayer, Layout);
}

void UGLGameUIPolicy::OnRootLayoutAddedToViewport(ULocalPlayer* LocalPlayer, UGLPrimaryGameLayout* Layout)
{
#if WITH_EDITOR
	if (GIsEditor && LocalPlayer)
	{
		FSlateApplication::Get().SetUserFocusToGameViewport(0);
	}
#endif
}

void UGLGameUIPolicy::Deinitialize()
{
	for (UGLGameUIFeature* UIFeature : UIFeatures)
	{
		if (UIFeature)
		{
			UIFeature->Deinitialize();
		}
	}
	UIFeatures.Empty();
	
	if (RootLayout)
	{
		RootLayout->RemoveFromParent();
		RootLayout = nullptr;
	}
}

UWorld* UGLGameUIPolicy::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (const UGLUIManagerSubsystem* UIManagerSubsystem = GetOwningUIManager())
	{
		return UIManagerSubsystem->GetWorld();
	}

	return nullptr;
}
