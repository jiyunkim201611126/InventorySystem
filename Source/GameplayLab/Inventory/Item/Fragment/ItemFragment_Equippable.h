// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "Data/ClassType.h"
#include "ItemFragment_Equippable.generated.h"

/**
 * 장착 가능한 아이템이 가지는 값입니다.
 */
USTRUCT(BlueprintType)
struct GAMEPLAYLAB_API FItemFragment_Equippable : public FItemFragment
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	EClassType ClassType = EClassType::None;
};

/** 
 * FItemFragment_Equippable을 Definition에 할당하기 위한 데이터 테이블 행입니다.
 * meta로 등록한 ItemFragment는 위 구조체의 리플렉션 객체 경로입니다.
 */
USTRUCT(BlueprintType, meta = (ItemFragment = "/Script/GameplayLab.ItemFragment_Equippable"))
struct GAMEPLAYLAB_API FItemFragment_EquippableTableRow : public FItemFragmentTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EClassType ClassType = EClassType::None;
};
