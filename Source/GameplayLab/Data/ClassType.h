#pragma once

UENUM(BlueprintType)
enum class EClassType : uint8
{
	None UMETA(ToolTip = "클래스 타입이 없는, 공용 아이템이라는 의미로도 사용됩니다."),
	Warrior,
	Wizard,
};