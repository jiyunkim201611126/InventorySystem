// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLUIManagerSubsystem.h"

void UGLUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadUIFeatureTable();

	if (!DefaultUIPolicyClass.IsNull())
	{
		SetCurrentUIPolicy(DefaultUIPolicyClass.LoadSynchronous());
	}
}

void UGLUIManagerSubsystem::Deinitialize()
{
	SwitchToPolicy(nullptr);
	UIFeatureClasses.Empty();

	Super::Deinitialize();
}

bool UGLUIManagerSubsystem::EnsureCreateRootLayout(ULocalPlayer* LocalPlayer) const
{
	return CurrentPolicy && CurrentPolicy->GetOrCreateRootLayout(LocalPlayer);
}

UGLGameUIPolicy* UGLUIManagerSubsystem::GetCurrentUIPolicy() const
{
	return CurrentPolicy;
}

bool UGLUIManagerSubsystem::SetCurrentUIPolicy(const TSubclassOf<UGLGameUIPolicy> PolicyClass)
{
	if (!PolicyClass || PolicyClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}

	UGLGameUIPolicy* NewPolicy = NewObject<UGLGameUIPolicy>(this, PolicyClass);
	if (!NewPolicy)
	{
		return false;
	}

	SwitchToPolicy(NewPolicy);
	return true;
}

void UGLUIManagerSubsystem::LoadUIFeatureTable()
{
	UIFeatureClasses.Empty();

	if (UIFeatureDataTable.IsNull())
	{
		return;
	}

	const UDataTable* LoadedUIFeatureDataTable = UIFeatureDataTable.LoadSynchronous();
	if (!ensure(LoadedUIFeatureDataTable && LoadedUIFeatureDataTable->GetRowStruct() == FGLUIFeatureTableRow::StaticStruct()))
	{
		return;
	}

	TArray<FGLUIFeatureTableRow*> Rows;
	LoadedUIFeatureDataTable->GetAllRows(TEXT("UIFeature"), Rows);
	for (const FGLUIFeatureTableRow* Row : Rows)
	{
		if (Row && Row->UIFeatureTag.IsValid() && !Row->UIFeatureClass.IsNull())
		{
			UIFeatureClasses.Emplace(Row->UIFeatureTag, Row->UIFeatureClass);
		}
	}
}

void UGLUIManagerSubsystem::SwitchToPolicy(UGLGameUIPolicy* InPolicy)
{
	if (CurrentPolicy == InPolicy)
	{
		return;
	}

	if (CurrentPolicy)
	{
		CurrentPolicy->Deinitialize();
	}

	CurrentPolicy = InPolicy;
}
