// Fill out your copyright notice in the Description page of Project Settings.

#include "GLCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InteractionFocusComponent.h"
#include "Components/CapsuleComponent.h"
#include "Game/GLGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interface/InteractableInterface.h"

AGLCharacter::AGLCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	InteractionFocusComponent = CreateDefaultSubobject<UInteractionFocusComponent>(TEXT("InteractionFocusComponent"));
}

void AGLCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	const APlayerController* PlayerController = GetController<APlayerController>();
	if (!PlayerController)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}
	
	InteractionFocusComponent->SetCameraComponent(CameraComponent);
	
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (InputSubsystem && InputMappingContext)
	{
		InputSubsystem->AddMappingContext(InputMappingContext, 0);
	}
	
	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Completed, this, &ThisClass::Interact);
	
	EnhancedInput->BindAction(SpawnItemAction, ETriggerEvent::Completed, this, &ThisClass::SpawnItem);
}

EClassType AGLCharacter::GetClassType() const
{
	return ClassType;
}

void AGLCharacter::Move(const FInputActionValue& InputActionValue)
{
	if (!Controller)
	{
		return;
	}
	
	const FVector2D MoveValue = InputActionValue.Get<FVector2D>();
	
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	AddMovementInput(ForwardDirection, MoveValue.Y);
	AddMovementInput(RightDirection, MoveValue.X);
}

void AGLCharacter::Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookValue = InputActionValue.Get<FVector2D>();
	
	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
}

void AGLCharacter::Interact()
{
	if (AActor* InteractableActor = InteractionFocusComponent->GetInteractableActor())
	{
		if (IInteractableInterface* Interaction = Cast<IInteractableInterface>(InteractableActor))
		{
			if (Interaction->CanInteract(this))
			{
				ServerInteract(InteractableActor);
			}
		}
	}
}

void AGLCharacter::ServerInteract_Implementation(AActor* InteractableActor)
{
	if (!IsValid(InteractableActor))
	{
		return;
	}
	
	if (IInteractableInterface* Interaction = Cast<IInteractableInterface>(InteractableActor))
	{
		if (Interaction->CanInteract(this))
		{
			Interaction->Interact(this);
		}
	}
}

void AGLCharacter::SpawnItem()
{
	ServerSpawnItem();
}

void AGLCharacter::ServerSpawnItem_Implementation()
{
	if (const AGLGameMode* GameMode = GetWorld()->GetAuthGameMode<AGLGameMode>())
	{
		GameMode->SpawnRandomItem();
	}
}
