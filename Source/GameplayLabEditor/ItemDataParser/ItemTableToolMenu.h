// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UToolMenu;

/**
 * Item DataTable 파싱 기능을 에디터 메인 메뉴에 등록하는 클래스입니다.
 */
class FItemTableToolMenu final
{
public:
	void Register();
	void Unregister();

private:
	using ThisClass = FItemTableToolMenu;

	void RegisterMenus();
	void FillItemTableToolMenu(UToolMenu* Menu);
	void ExecuteParseAllItemDataTables();
};
