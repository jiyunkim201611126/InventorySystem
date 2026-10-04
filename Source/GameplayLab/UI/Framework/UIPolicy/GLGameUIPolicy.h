// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/World.h"
#include "UI/Framework/UIFeature/GLGameUIFeature.h"
#include "GLGameUIPolicy.generated.h"

class UGLGameUIFeature;
class UGLPrimaryGameLayout;
class UGLUIManagerSubsystem;

/**
 * Root Layout으로 어떤 위젯을 사용할지 결정하는 UI 정책용 클래스입니다.
 * RootLayout과 Layer 접근을 제공합니다.
 */
UCLASS(Abstract, Blueprintable, Within = GLUIManagerSubsystem)
class GAMEPLAYLAB_API UGLGameUIPolicy : public UObject
{
	GENERATED_BODY()

public:
	static UGLGameUIPolicy* GetGameUIPolicy(const UObject* WorldContextObject);

	UGLUIManagerSubsystem* GetOwningUIManager() const;
	
	UGLPrimaryGameLayout* GetOrCreateRootLayout(ULocalPlayer* LocalPlayer);
	UGLPrimaryGameLayout* GetRootLayout() const;

	template <typename FeatureT>
	FeatureT* GetOrCreateUIFeature(const TSubclassOf<FeatureT>& FeatureTClass)
	{
		static_assert(TIsDerivedFrom<FeatureT, UGLGameUIFeature>::IsDerived, "FeatureT는 반드시 UGLGameUIFeature를 상속받아야 합니다.");

		for (UGLGameUIFeature* UIFeature : UIFeatures)
		{
			if (UIFeature && UIFeature->GetClass() == FeatureTClass)
			{
				return Cast<FeatureT>(UIFeature);
			}
		}

		if (ensure(FeatureTClass && !FeatureTClass->HasAnyClassFlags(CLASS_Abstract)))
		{
			if (FeatureT* CreatedFeature = NewObject<FeatureT>(this, FeatureTClass))
			{
				CreatedFeature->Initialize(GetRootLayout());
				UIFeatures.Add(CreatedFeature);
				return CreatedFeature;
			}
		}
		return nullptr;
	}

	void Deinitialize();

	virtual UWorld* GetWorld() const override;

protected:
	void CreateLayoutWidget(ULocalPlayer* LocalPlayer);
	
	void AddLayoutToViewport(ULocalPlayer* LocalPlayer, UGLPrimaryGameLayout* Layout);

	virtual void OnRootLayoutAddedToViewport(ULocalPlayer* LocalPlayer, UGLPrimaryGameLayout* Layout);

protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UGLPrimaryGameLayout> RootLayoutWidgetClass;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGLGameUIFeature>> UIFeatures;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer StartUIFeatureTags;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGLPrimaryGameLayout> RootLayout;
};
