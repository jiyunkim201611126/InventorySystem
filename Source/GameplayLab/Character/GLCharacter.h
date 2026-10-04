// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/ClassType.h"
#include "GameFramework/Character.h"
#include "GLCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UInteractionFocusComponent;
class USpringArmComponent;
struct FInputActionValue;

UCLASS(Abstract, NotBlueprintType)
class GAMEPLAYLAB_API AGLCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGLCharacter();
	
	//~ Begin ACharacter Interface
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	//~ End of ACharacter Interface
	
	EClassType GetClassType() const;
	
private:
	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void Interact();
	
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor);
	
	/** 테스트용으로 키 입력 시 드랍 아이템이 스폰되도록 서버에 요청합니다. */
	void SpawnItem();
	
	UFUNCTION(Server, Reliable)
	void ServerSpawnItem();
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInteractionFocusComponent> InteractionFocusComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SpawnItemAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Class")
	EClassType ClassType = EClassType::Warrior;
};
