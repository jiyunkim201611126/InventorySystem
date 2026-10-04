// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemDataParserSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "Inventory/Item/Fragment/ItemFragment.h"
#include "Inventory/Item/ItemDefinition.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Subsystems/ImportSubsystem.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogItemDataParser, Log, All);

namespace
{
	const FName ItemDefinitionPropertyName = GET_MEMBER_NAME_CHECKED(FItemDefinitionRow, ItemDefinition);
	const FName ItemFragmentsPropertyName = GET_MEMBER_NAME_CHECKED(UItemDefinition, ItemFragments);

	/** 테이블 행 구조체와 동일 이름 프로퍼티가 존재하는지, 그리고 타입까지 일치하는지 검사하는 함수입니다. */
	bool ValidatePropertyMapping(const UStruct* SourceType, const UStruct* DestinationType, const FString& ErrorContext)
	{
		bool bIsValid = true;

		// SourceType의 프로퍼티들을 순회합니다.
		for (TFieldIterator<FProperty> It(SourceType, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			const FProperty* SourceProperty = *It;
			
			// ItemDefinition 프로퍼티라면 생략합니다.
			if (SourceProperty->GetFName() == ItemDefinitionPropertyName)
			{
				continue;
			}

			// 이름이 일치하는 프로퍼티가 있는지 확인합니다.
			const FProperty* DestinationProperty = DestinationType->FindPropertyByName(SourceProperty->GetFName());
			if (!DestinationProperty)
			{
				UE_LOG(LogItemDataParser, Error, TEXT("%s: 대상 구조체 %s에 %s 프로퍼티가 없습니다."), *ErrorContext, *DestinationType->GetName(), *SourceProperty->GetName());
				bIsValid = false;
				continue;
			}

			// 일치하는 이름의 프로퍼티가 타입이 같은지 확인합니다.
			if (!SourceProperty->SameType(DestinationProperty))
			{
				UE_LOG(LogItemDataParser, Error, TEXT("%s: %s 프로퍼티의 타입이 대상 구조체와 다릅니다."), *ErrorContext, *SourceProperty->GetName());
				bIsValid = false;
			}
		}
		
		// 역방향으로도 확인합니다.
		for (TFieldIterator<FProperty> It(DestinationType, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			const FProperty* DestinationProperty = *It;
			
			// ItemDefinition 프로퍼티거나 ItemFragments 프로퍼티라면 생략합니다.
			if (DestinationProperty->GetFName() == ItemDefinitionPropertyName || DestinationProperty->GetFName() == ItemFragmentsPropertyName)
			{
				continue;
			}
			
			const FProperty* SourceProperty = SourceType->FindPropertyByName(DestinationProperty->GetFName());
			if (!SourceProperty)
			{
				UE_LOG(LogItemDataParser, Error, TEXT("%s: 대상 구조체에 %s 프로퍼티가 없습니다."), *ErrorContext, *DestinationProperty->GetName());
				bIsValid = false;
				continue;
			}

			if (!DestinationProperty->SameType(SourceProperty))
			{
				UE_LOG(LogItemDataParser, Error, TEXT("%s: %s 프로퍼티의 타입이 대상 구조체와 다릅니다."), *ErrorContext, *DestinationProperty->GetName());
				bIsValid = false;
			}
		}

		return bIsValid;
	}

	/** 인자로 들어온 DataTable 구조체 리플렉션 객체와 매칭되는 ItemFragment 구조체 리플렉션 객체를 반환하는 함수입니다. */
	UScriptStruct* ResolveFragmentStruct(const UScriptStruct* RowStruct, const UDataTable* DataTable)
	{
		// 테이블용 구조체 선언 시 meta로 등록한 문자열, 즉 ItemFragment 구조체 리플렉션 객체 경로를 가져옵니다.
		const FString FragmentStructPath = RowStruct->GetMetaData(TEXT("ItemFragment"));
		if (FragmentStructPath.IsEmpty())
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s: 행 구조체에 ItemFragment 메타데이터가 없습니다."), *DataTable->GetPathName());
			return nullptr;
		}

		// 리플렉션 객체를 가져옵니다.
		UScriptStruct* FragmentStruct = FindObject<UScriptStruct>(nullptr, *FragmentStructPath);
		if (!FragmentStruct)
		{
			FragmentStruct = LoadObject<UScriptStruct>(nullptr, *FragmentStructPath);
		}
		
		if (!FragmentStruct || !FragmentStruct->IsChildOf(FItemFragment::StaticStruct()))
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s: ItemFragment 메타데이터가 유효한 Fragment 구조체를 가리키지 않습니다."), *DataTable->GetPathName());
			return nullptr;
		}

		return FragmentStruct;
	}

	/** 테이블 행과 객체, Fragment의 동일 이름 프로퍼티 값을 비교해 하나라도 다르면 true를 반환합니다. */
	bool HasPropertyChanges(const UStruct* SourceType, const void* SourceData, const UStruct* DestinationType, const void* DestinationData)
	{
		for (TFieldIterator<FProperty> It(SourceType, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			const FProperty* SourceProperty = *It;
			if (SourceProperty->GetFName() == ItemDefinitionPropertyName)
			{
				continue;
			}

			const FProperty* DestinationProperty = DestinationType->FindPropertyByName(SourceProperty->GetFName());
			if (!DestinationProperty || !SourceProperty->SameType(DestinationProperty))
			{
				continue;
			}

			// SourceProperty와 DestinationProperty의 실제 값 주소를 가져옵니다.
			const void* SourceValue = SourceProperty->ContainerPtrToValuePtr<void>(SourceData);
			const void* DestinationValue = DestinationProperty->ContainerPtrToValuePtr<void>(DestinationData);
			
			// 두 값을 비교해 다르다면 true를 반환합니다.
			if (!DestinationProperty->Identical(SourceValue, DestinationValue))
			{
				return true;
			}
		}

		return false;
	}

	/** 변경 사항이 있다면 Definition 블루프린트 클래스와 CDO의 변경 전 상태를 기록하고, 변경 목록에 블루프린트를 추가합니다. */
	void BeginDefinitionEdit(UItemDefinition* Definition, TSet<UBlueprint*>& ChangedBlueprints)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Definition->GetClass()->ClassGeneratedBy);
		if (!Blueprint || ChangedBlueprints.Contains(Blueprint))
		{
			return;
		}

		Blueprint->Modify();
		Definition->Modify();
		ChangedBlueprints.Add(Blueprint);
	}

	/** 테이블 행의 프로퍼티 값을 동일한 이름과 타입을 가진 대상 프로퍼티에 복사합니다. */
	void CopyMatchingProperties(const UStruct* SourceType, const void* SourceData, const UStruct* DestinationType, void* DestinationData)
	{
		for (TFieldIterator<FProperty> It(SourceType, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			const FProperty* SourceProperty = *It;
			if (SourceProperty->GetFName() == ItemDefinitionPropertyName)
			{
				continue;
			}

			const FProperty* DestinationProperty = DestinationType->FindPropertyByName(SourceProperty->GetFName());
			if (!DestinationProperty || !SourceProperty->SameType(DestinationProperty))
			{
				continue;
			}

			// 메모리에서 각 프로퍼티의 실제 값 주소를 가져오고, DestinationValue에 SourceValue 값을 복사합니다.
			const void* SourceValue = SourceProperty->ContainerPtrToValuePtr<void>(SourceData);
			void* DestinationValue = DestinationProperty->ContainerPtrToValuePtr<void>(DestinationData);
			DestinationProperty->CopyCompleteValue(DestinationValue, SourceValue);
		}
	}

	/** Definition의 ItemFragments를 테이블 행과 동기화합니다. */
	void SynchronizeFragment(UItemDefinition* Definition, const UScriptStruct* FragmentStruct, const UScriptStruct* RowStruct, const uint8* RowData, TSet<UBlueprint*>& ChangedBlueprints)
	{
		// 현재 Definition이 가진 모든 Fragment를 순회하면서 FragmentStruct와 같은 타입의 Fragment 배열 인덱스를 찾습니다.
		const int32 MatchingFragmentIndex = Definition->ItemFragments.IndexOfByPredicate(
			[FragmentStruct](const TInstancedStruct<FItemFragment>& Fragment)
			{
				return Fragment.GetScriptStruct() == FragmentStruct;
			});

		// Fragment 테이블에 현재 Definition을 대상으로 하는 행이 없고, Definition에는 Fragment가 있다면 그 Fragment를 제거합니다.
		if (!RowData)
		{
			if (MatchingFragmentIndex != INDEX_NONE)
			{
				BeginDefinitionEdit(Definition, ChangedBlueprints);
				Definition->ItemFragments.RemoveAt(MatchingFragmentIndex);
			}
			return;
		}

		// Fragment 테이블에 현재 Definition을 대상으로 하는 행이 있고, Definition에는 Fragment가 없다면 추가하고 값을 복사합니다.
		if (MatchingFragmentIndex == INDEX_NONE)
		{
			BeginDefinitionEdit(Definition, ChangedBlueprints);

			TInstancedStruct<FItemFragment>& NewFragment = Definition->ItemFragments.AddDefaulted_GetRef();
			NewFragment.InitializeAsScriptStruct(FragmentStruct);
			CopyMatchingProperties(RowStruct, RowData, FragmentStruct, NewFragment.GetMutableMemory());
			return;
		}

		// 둘 다 있는 경우, 값이 변경됐는지 확인하고 값을 복사합니다.
		TInstancedStruct<FItemFragment>& ExistingFragment = Definition->ItemFragments[MatchingFragmentIndex];
		if (HasPropertyChanges(RowStruct, RowData, FragmentStruct, ExistingFragment.GetMemory()))
		{
			BeginDefinitionEdit(Definition, ChangedBlueprints);
			CopyMatchingProperties(RowStruct, RowData, FragmentStruct, ExistingFragment.GetMutableMemory());
		}
	}
}

void UItemDataParserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 에셋 Import 후, 에셋 Reimport후 호출되는 콜백을 등록합니다.
	Collection.InitializeDependency<UImportSubsystem>();
	if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
	{
		ImportSubsystem->OnAssetPostImport.AddUObject(this, &ThisClass::OnAssetPostImport);
		ImportSubsystem->OnAssetReimport.AddUObject(this, &ThisClass::OnAssetReimport);
	}

}

void UItemDataParserSubsystem::Deinitialize()
{
	if (GEditor)
	{
		if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
		{
			ImportSubsystem->OnAssetPostImport.RemoveAll(this);
			ImportSubsystem->OnAssetReimport.RemoveAll(this);
		}
	}

	Super::Deinitialize();
}

int32 UItemDataParserSubsystem::ParseAllItemDataTables()
{
	// Game 폴더 아래에 있는 DataTable 에셋을 찾기 위한 필터를 선언합니다.
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/Data/DataTable")));
	Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;

	// DataTable 에셋들의 메타데이터 목록을 가져옵니다.
	TArray<FAssetData> AssetDataList;
	FAssetRegistryModule::GetRegistry().GetAssets(Filter, AssetDataList);

	// 찾은 DataTable을 용도별로 분류해 저장할 컨테이너입니다.
	UDataTable* DefinitionTable = nullptr;
	TMap<const UScriptStruct*, UDataTable*> FragmentTables;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UDataTable* DataTable = Cast<UDataTable>(AssetData.GetAsset());
		if (!DataTable || !DataTable->GetRowStruct())
		{
			continue;
		}

		// ItemDefinition 테이블인 경우 DefinitionTable에 할당합니다.
		const UScriptStruct* RowStruct = DataTable->GetRowStruct();
		if (RowStruct == FItemDefinitionRow::StaticStruct())
		{
			DefinitionTable = DataTable;
			continue;
		}

		if (!RowStruct->IsChildOf(FItemFragmentTableRowBase::StaticStruct()))
		{
			continue;
		}

		// Fragment Table과 매칭되는 ItemFragment 구조체 리플렉션 객체를 가져오고, 프로퍼티 선언에 문제가 없는지 확인합니다.
		UScriptStruct* FragmentStruct = ResolveFragmentStruct(RowStruct, DataTable);
		if (!FragmentStruct || !ValidatePropertyMapping(RowStruct, FragmentStruct, DataTable->GetPathName()))
		{
			continue;
		}
		
		// 이미 동일한 Fragment가 등록되어 있다면 에러 로그를 출력하고 생략합니다.
		if (const UDataTable* ExistingTable = FragmentTables.FindRef(FragmentStruct))
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s와 %s가 같은 Fragment 타입을 대상으로 합니다."), *ExistingTable->GetPathName(), *DataTable->GetPathName());
			continue;
		}

		FragmentTables.Add(FragmentStruct, DataTable);
	}

	if (!DefinitionTable)
	{
		UE_LOG(LogItemDataParser, Error, TEXT("FItemDefinitionRow를 사용하는 DataTable이 없습니다."));
		return 0;
	}

	if (!ValidatePropertyMapping(FItemDefinitionRow::StaticStruct(), UItemDefinition::StaticClass(), TEXT("Definition Table")))
	{
		return 0;
	}

	// Definition 클래스와 그 클래스에 적용할 Definition 테이블 행을 연결합니다.
	TMap<UClass*, const FItemDefinitionRow*> DefinitionSources;
	// ItemId가 중복됐는지 검사하기 위한 Map입니다.
	TMap<uint64, UClass*> ItemIdToDefinition;

	// Definition 테이블의 모든 행을 순회합니다.
	for (const auto& Pair : DefinitionTable->GetRowMap())
	{
		const FItemDefinitionRow* Row = reinterpret_cast<const FItemDefinitionRow*>(Pair.Value);
		const FString ErrorContext = FString::Printf(TEXT("%s[%s]"), *DefinitionTable->GetPathName(), *Pair.Key.ToString());

		UClass* DefinitionClass = Row->ItemDefinition.LoadSynchronous();
		if (!DefinitionClass || !DefinitionClass->IsChildOf(UItemDefinition::StaticClass()))
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s: 유효한 ItemDefinition 블루프린트 클래스가 없습니다."), *ErrorContext);
			continue;
		}

		if (!Cast<UBlueprint>(DefinitionClass->ClassGeneratedBy))
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s: ItemDefinition은 수정 가능한 블루프린트 클래스여야 합니다."), *ErrorContext);
			continue;
		}

		if (DefinitionSources.Contains(DefinitionClass))
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s: Definition 클래스가 중복되었습니다."), *ErrorContext);
			continue;
		}

		if (const UClass* const* ExistingDefinition = ItemIdToDefinition.Find(Row->ItemId))
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s: ItemId %s가 %s와 중복되었습니다."), *ErrorContext, *LexToString(Row->ItemId), *(*ExistingDefinition)->GetPathName());
			continue;
		}

		DefinitionSources.Add(DefinitionClass, Row);
		ItemIdToDefinition.Add(Row->ItemId, DefinitionClass);
	}

	// 모든 Definition 블루프린트 클래스를 순회합니다.
	TSet<UBlueprint*> ChangedBlueprints;
	for (const auto& DefinitionPair : DefinitionSources)
	{
		// 모든 프로퍼티를 비교하면서 변경사항이 있다면 변경 전 내용을 기록하고 테이블 값을 블루프린트쪽에 복사합니다.
		UItemDefinition* Definition = CastChecked<UItemDefinition>(DefinitionPair.Key->GetDefaultObject());
		if (HasPropertyChanges(FItemDefinitionRow::StaticStruct(), DefinitionPair.Value, Definition->GetClass(), Definition))
		{
			BeginDefinitionEdit(Definition, ChangedBlueprints);
			CopyMatchingProperties(FItemDefinitionRow::StaticStruct(), DefinitionPair.Value, Definition->GetClass(), Definition);
		}
	}

	// 앞에서 수집한 모든 Fragment 테이블을 순회합니다.
	for (const auto& FragmentTablePair : FragmentTables)
	{
		// 다루기 편리하도록 지역변수들을 선언합니다.
		const UScriptStruct* FragmentStruct = FragmentTablePair.Key;
		const UDataTable* FragmentTable = FragmentTablePair.Value;
		const UScriptStruct* RowStruct = FragmentTable->GetRowStruct();
		TMap<UClass*, const uint8*> RowsByDefinition;
		
		// 모든 행이 유효한지 기록할 bool입니다.
		bool bAreRowsValid = true;

		// 모든 행을 순회하며 Fragment 테이블의 행을 대상 Definition 클래스 기준으로 정리합니다.
		for (const auto& RowPair : FragmentTable->GetRowMap())
		{
			const FItemFragmentTableRowBase* Row = reinterpret_cast<const FItemFragmentTableRowBase*>(RowPair.Value);
			const FString ErrorContext = FString::Printf(TEXT("%s[%s]"), *FragmentTable->GetPathName(), *RowPair.Key.ToString());
			UClass* DefinitionClass = Row->ItemDefinition.LoadSynchronous();

			if (!DefinitionClass || !DefinitionSources.Contains(DefinitionClass))
			{
				UE_LOG(LogItemDataParser, Error, TEXT("%s: Definition 테이블에 등록된 ItemDefinition을 참조해야 합니다."), *ErrorContext);
				bAreRowsValid = false;
				continue;
			}

			if (RowsByDefinition.Contains(DefinitionClass))
			{
				UE_LOG(LogItemDataParser, Error, TEXT("%s: 같은 Definition에 대한 Fragment 행이 중복되었습니다."), *ErrorContext);
				bAreRowsValid = false;
				continue;
			}

			RowsByDefinition.Add(DefinitionClass, RowPair.Value);
		}

		if (!bAreRowsValid)
		{
			UE_LOG(LogItemDataParser, Error, TEXT("%s: 유효하지 않은 행이 있어 이 테이블은 반영하지 않습니다."), *FragmentTable->GetPathName());
			continue;
		}

		// 모든 Definition 클래스를 순회하며 테이블 행과 Definition의 Fragments를 동기화합니다.
		for (const auto& DefinitionPair : DefinitionSources)
		{
			UItemDefinition* Definition = CastChecked<UItemDefinition>(DefinitionPair.Key->GetDefaultObject());
			const uint8* RowData = RowsByDefinition.FindRef(DefinitionPair.Key);
			SynchronizeFragment(Definition, FragmentStruct, RowStruct, RowData, ChangedBlueprints);
		}
	}

	// 변경된 블루프린트를 순회하며 변경 상태로 표시하고, 저장할 패키지를 수집합니다.
	TArray<UPackage*> PackagesToSave;
	for (UBlueprint* Blueprint : ChangedBlueprints)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		PackagesToSave.AddUnique(Blueprint->GetOutermost());
	}

	// 저장할 패키지가 하나 이상 있는 경우 저장 함수를 호출합니다.
	if (!PackagesToSave.IsEmpty())
	{
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
	}

	UE_LOG(LogItemDataParser, Display, TEXT("Item DataTable 파싱 완료: %d개 Definition 변경"), ChangedBlueprints.Num());
	return ChangedBlueprints.Num();
}

void UItemDataParserSubsystem::OnAssetPostImport(UFactory* Factory, UObject* CreatedObject)
{
	if (IsItemDataTable(Cast<UDataTable>(CreatedObject)))
	{
		ParseAllItemDataTables();
	}
}

void UItemDataParserSubsystem::OnAssetReimport(UObject* CreatedObject)
{
	if (IsItemDataTable(Cast<UDataTable>(CreatedObject)))
	{
		ParseAllItemDataTables();
	}
}

bool UItemDataParserSubsystem::IsItemDataTable(const UDataTable* DataTable) const
{
	if (!DataTable || !DataTable->GetRowStruct())
	{
		return false;
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	return RowStruct == FItemDefinitionRow::StaticStruct() || RowStruct->IsChildOf(FItemFragmentTableRowBase::StaticStruct());
}
