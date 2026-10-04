// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemFragment.generated.h"

class UItemDefinition;

/**
 * 아이템을 구성하는 가장 작은 단위로, 아이템 관련 기능을 구현할 객체가 가져가 사용할 데이터입니다.
 * 구체적인 값은 이 구조체를 상속한 Fragment에서 정의합니다.
 */
USTRUCT(BlueprintType)
struct GAMEPLAYLAB_API FItemFragment
{
	GENERATED_BODY()
};

/**
 * Fragment 데이터 테이블 행의 공통 형식입니다.
 * 자식 구조체 선언 시 반드시 meta에 ItemFragment라는 이름으로 ItemFragment의 리플렉션 객체 경로를 등록해야 합니다.
 * 자식 구조체에 변수 추가 시 반드시 매칭되는 FItemFragment의 변수 이름과 일치시켜야 합니다.
 * ItemFragment의 리플렉션 객체란, 테이블용 구조체가 아닌 FItemFragment를 상속한 구조체의 리플렉션 객체입니다.
 */
USTRUCT(BlueprintType)
struct GAMEPLAYLAB_API FItemFragmentTableRowBase : public FTableRowBase
{
	GENERATED_BODY()

	/** Fragment가 할당될 Definition을 찾을 용도로 선언한 ItemDefinition입니다. */
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UItemDefinition> ItemDefinition;
};
