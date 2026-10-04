// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/Framework/UIPolicy/GLGameUIPolicy.h"
#include "GLUIManagerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FGLUIFeatureTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag UIFeatureTag;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UGLGameUIFeature> UIFeatureClass;
};

/** GameUIPolicy와 UIFeature 클래스를 생성하고 관리하는 서브시스템입니다. */
UCLASS(Config = Game)
class GAMEPLAYLAB_API UGLUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool EnsureCreateRootLayout(ULocalPlayer* LocalPlayer) const;
	UGLGameUIPolicy* GetCurrentUIPolicy() const;
	bool SetCurrentUIPolicy(TSubclassOf<UGLGameUIPolicy> PolicyClass);

	template <typename FeatureT>
	FeatureT* GetOrCreateUIFeature(const FGameplayTag& FeatureTag) const
	{
		static_assert(TIsDerivedFrom<FeatureT, UGLGameUIFeature>::IsDerived, "FeatureT는 반드시 UGLGameUIFeature를 상속받아야 합니다.");

		if (!FeatureTag.IsValid() || !CurrentPolicy)
		{
			return nullptr;
		}

		const UGameInstance* GameInstance = GetGameInstance();
		ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		if (!EnsureCreateRootLayout(LocalPlayer))
		{
			return nullptr;
		}

		if (const TSoftClassPtr<UGLGameUIFeature>* UIFeatureClass = UIFeatureClasses.Find(FeatureTag))
		{
			const TSubclassOf<FeatureT> LoadedUIFeatureClass = UIFeatureClass->LoadSynchronous();
			return CurrentPolicy->GetOrCreateUIFeature<FeatureT>(LoadedUIFeatureClass);
		}
		return nullptr;
	}

private:
	void LoadUIFeatureTable();
	void SwitchToPolicy(UGLGameUIPolicy* InPolicy);

protected:
	UPROPERTY(Config)
	TSoftClassPtr<UGLGameUIPolicy> DefaultUIPolicyClass;

	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> UIFeatureDataTable;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGLGameUIPolicy> CurrentPolicy;

	TMap<FGameplayTag, TSoftClassPtr<UGLGameUIFeature>> UIFeatureClasses;
};
