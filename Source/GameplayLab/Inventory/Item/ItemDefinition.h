// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "Fragment/ItemFragment.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemDefinition.generated.h"

UENUM(BlueprintType)
enum class EItemGrade : uint8
{
	Common,
	Rare,
	Unique,
	Legendary,
};

/**
 * 아이템의 정적 데이터를 담당하는 클래스입니다.
 * 예시로는 최대 스택 수, 장착 가능 여부, 사용 가능 클래스 등이 있습니다.
 */
UCLASS(Abstract, Blueprintable)
class GAMEPLAYLAB_API UItemDefinition : public UObject
{
	GENERATED_BODY()
	
public:
	/** 타입에 일치하는 Fragment를 찾아 반환하는 함수입니다. */
	template <typename FragmentType>
	const FragmentType* FindFragment() const
	{
		static_assert(std::is_base_of_v<FItemFragment, FragmentType>, "FragmentType은 반드시 FItemFragment를 상속받아야 합니다.");

		for (const TInstancedStruct<FItemFragment>& ItemFragment : ItemFragments)
		{
			if (const FragmentType* FoundFragment = ItemFragment.GetPtr<FragmentType>())
			{
				return FoundFragment;
			}
		}

		return nullptr;
	}

public:
	UPROPERTY(EditDefaultsOnly)
	uint64 ItemId = 0;
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"))
	uint64 MaxStack = 1;
	
	UPROPERTY(EditDefaultsOnly)
	EItemGrade Grade = EItemGrade::Common;
	
	/** Fragment를 조합하는 배열이며, 절대로 중복된 Fragment를 할당해선 안 됩니다. */
	UPROPERTY(EditDefaultsOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FItemFragment>> ItemFragments;
};

/**
 * 편집 편의성을 위해 제공되는 데이터 테이블입니다.
 * Fragment도 헤더파일마다 하나씩 정의되어 있습니다.
 * 에디터 스크립트를 통해 파싱해 ItemDefinition 블루프린트 클래스에 값을 할당합니다.
 * Definition 테이블은 예외로 런타임 중에도 사용할 수 있으며, Id 기준으로 Definition 클래스를 찾아 반환하는 기능을 ItemManagerSubsystem에서 제공합니다.
 * 
 * 변수 추가 시 반드시 ItemDefinition의 변수 이름과 일치시켜야 합니다.
 */
USTRUCT(BlueprintType)
struct GAMEPLAYLAB_API FItemDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UItemDefinition> ItemDefinition;
	
	UPROPERTY(EditDefaultsOnly)
	uint64 ItemId = 0;
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"))
	uint64 MaxStack = 1;
	
	UPROPERTY(EditDefaultsOnly)
	EItemGrade Grade = EItemGrade::Common;
};
