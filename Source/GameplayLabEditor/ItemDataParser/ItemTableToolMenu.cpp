// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemTableToolMenu.h"

#include "Editor.h"
#include "ItemDataParserSubsystem.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FItemTableToolMenu"

void FItemTableToolMenu::Register()
{
	// ToolMenus 시스템 초기화가 끝나면 RegisterMenus를 호출하도록 콜백을 등록합니다.
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &ThisClass::RegisterMenus));
}

void FItemTableToolMenu::Unregister()
{
	// 콜백과 이 객체 소유로 등록된 메뉴들을 모두 제거합니다.
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FItemTableToolMenu::RegisterMenus()
{
	// 이 함수에서 등록하는 메뉴 항목의 소유자를 이 객체로 지정합니다.
	FToolMenuOwnerScoped OwnerScoped(this);

	// 언리얼 에디터 상단 메인 메뉴 바를 가져옵니다.
	UToolMenu* MainMenuBar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
	// 드랍다운 메뉴 버튼들이 들어가 있는 섹션을 가져옵니다. 없다면 생성합니다.
	FToolMenuSection& Section = MainMenuBar->FindOrAddSection(TEXT("MainMenu"));
	// 해당 섹션에 ItemTableTool이라는 메뉴 버튼을 추가합니다.
	Section.AddSubMenu(
		TEXT("ItemTableTool"),
		LOCTEXT("ItemTableToolLabel", "ItemTableTool"),
		LOCTEXT("ItemTableToolTooltip", "Item DataTable 관련 도구입니다."),
		FNewToolMenuDelegate::CreateRaw(this, &ThisClass::FillItemTableToolMenu));
}

void FItemTableToolMenu::FillItemTableToolMenu(UToolMenu* Menu)
{
	// ItemTableTool 드랍다운 내부에서 메뉴 항목을 배치할 섹션을 가져옵니다. 없다면 생성합니다.
	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("ItemTableTool"));
	// ParseAllItemDataTables라는 메뉴 엔트리를 추가합니다.
	Section.AddMenuEntry(
		TEXT("ParseAllItemDataTables"),
		LOCTEXT("ParseAllItemDataTablesLabel", "ParseAllItemDataTables"),
		LOCTEXT("ParseAllItemDataTablesTooltip", "모든 Item DataTable을 ItemDefinition 블루프린트에 반영하고 저장합니다."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &ThisClass::ExecuteParseAllItemDataTables)));
}

void FItemTableToolMenu::ExecuteParseAllItemDataTables()
{
	if (!GEditor)
	{
		return;
	}

	// 아이템 테이블 파싱 및 Definition 반영과 저장을 시작합니다.
	if (UItemDataParserSubsystem* ParserSubsystem = GEditor->GetEditorSubsystem<UItemDataParserSubsystem>())
	{
		ParserSubsystem->ParseAllItemDataTables();
	}
}

#undef LOCTEXT_NAMESPACE
