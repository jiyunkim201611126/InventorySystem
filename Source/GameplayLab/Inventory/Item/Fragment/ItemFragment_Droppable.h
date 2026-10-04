// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "ItemFragment_Droppable.generated.h"

/**
 * 바닥에 떨어뜨릴 수 있는 아이템이 가지는 값입니다.
 */
USTRUCT(BlueprintType)
struct GAMEPLAYLAB_API FItemFragment_Droppable : public FItemFragment
{
	GENERATED_BODY()
	
public:
	/** 드랍될 아이템의 메쉬입니다. */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> DropMesh;
	
	UPROPERTY(EditAnywhere)
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);
};

/**
 * FItemFragment_Droppable을 Definition에 할당하기 위한 데이터 테이블 행입니다.
 * meta로 등록한 ItemFragment는 위 구조체의 리플렉션 객체 경로입니다.
 */
USTRUCT(BlueprintType, meta = (ItemFragment = "/Script/GameplayLab.ItemFragment_Droppable"))
struct GAMEPLAYLAB_API FItemFragment_DroppableTableRow : public FItemFragmentTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> DropMesh;

	UPROPERTY(EditAnywhere)
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);
};
