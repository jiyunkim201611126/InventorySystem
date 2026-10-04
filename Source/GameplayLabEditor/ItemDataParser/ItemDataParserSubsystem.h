// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "ItemDataParserSubsystem.generated.h"

class UDataTable;
class UFactory;

/** 
 * ItemDefinition, ItemFragment 테이블들을 파싱해서 ItemDefinition 블루프린트 클래스에 적용하는 클래스입니다.
 */
UCLASS()
class GAMEPLAYLABEDITOR_API UItemDataParserSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 모든 Item 데이터 테이블을 파싱하고 실제로 변경된 Definition만 저장합니다. */
	UFUNCTION(BlueprintCallable, Category = "GameplayLab|Item Data")
	int32 ParseAllItemDataTables();

private:
	/** 새로운 에셋을 임포트하거나, 기존 에셋을 리임포트할 때 호출되는 콜백으로, Item 관련 테이블인 경우 ParseAllItemDataTables가 호출됩니다. */
	void OnAssetPostImport(UFactory* Factory, UObject* CreatedObject);
	void OnAssetReimport(UObject* CreatedObject);
	bool IsItemDataTable(const UDataTable* DataTable) const;
};
