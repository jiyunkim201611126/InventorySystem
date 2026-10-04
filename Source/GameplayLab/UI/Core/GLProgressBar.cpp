// Copyright JETBLU, Inc. All Rights Reserved.

#include "GLProgressBar.h"

#include "Components/ProgressBar.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"

void UGLProgressBar::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GhostPercentSetTimerHandle);
		World->GetTimerManager().ClearTimer(PercentInterpTimerHandle);
	}
	
	Super::NativeDestruct();
}

void UGLProgressBar::SetProgressBarStyle(const FProgressBarStyle& FrontProgressBarStyle, const FProgressBarStyle& GhostProgressBarStyle, UMaterialInterface* PreviewProgressBarMaterial)
{
	FrontProgressBar->SetWidgetStyle(FrontProgressBarStyle);
	GhostProgressBar->SetWidgetStyle(GhostProgressBarStyle);

	if (PreviewProgressBarMaterial)
	{
		FProgressBarStyle PreviewProgressBarStyle = FrontProgressBarStyle;
		FSlateBrush PreviewBrush = PreviewProgressBarStyle.FillImage;
		PreviewBarDynamicMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, PreviewProgressBarMaterial);
		PreviewBrush.SetResourceObject(PreviewBarDynamicMaterialInstance);
		PreviewProgressBarStyle.SetFillImage(PreviewBrush);
		PreviewProgressBar->SetWidgetStyle(PreviewProgressBarStyle);
		PreviewProgressBar->SynchronizeProperties();
	}
}

void UGLProgressBar::SetFillColorAndOpacity(const FLinearColor& InLinearColor)
{
	FrontProgressBar->SetFillColorAndOpacity(InLinearColor);
	PreviewProgressBar->SetFillColorAndOpacity(InLinearColor);
	GhostProgressBar->SetFillColorAndOpacity(InLinearColor);
}

void UGLProgressBar::SetBarPercent(const float InPercent, const bool bShouldInterp)
{
	FrontProgressBar->SetPercent(InPercent);
	PreviewProgressBar->SetVisibility(ESlateVisibility::Collapsed);

	if (bShouldInterp)
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this, InPercent]()
		{
			BarPercentSet(InPercent);
		});

		if (const UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				GhostPercentSetTimerHandle,
				TimerDelegate,
				GhostStartDelay,
				false);
		}
	}
	else
	{
		GhostProgressBar->SetPercent(InPercent);
	}
}

void UGLProgressBar::SetPreviewBarPercent(const float InPercent)
{
	if (const UWorld* World = GetWorld())
	{
		PlayPreviewBlinking(World->GetTimeSeconds());
	}
	PreviewProgressBar->SetPercent(FrontProgressBar->GetPercent());
	PreviewProgressBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FrontProgressBar->SetPercent(InPercent);
}

void UGLProgressBar::StopPreview(const float InPercent)
{
	PreviewProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	FrontProgressBar->SetPercent(InPercent);
}

void UGLProgressBar::BarPercentSet(const float InPercent)
{
	GhostPercentTarget = InPercent;

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PercentInterpTimerHandle);
		World->GetTimerManager().SetTimer(
			PercentInterpTimerHandle,
			this,
			&ThisClass::InterpGhostBar,
			GhostInterpDelay,
			true);
	}
}

void UGLProgressBar::InterpGhostBar()
{
	if (const UWorld* World = GetWorld())
	{
		const float CurrentGhostBarPercent = GhostProgressBar->GetPercent();
		const float NextGhostBarPercent = UKismetMathLibrary::FInterpTo(CurrentGhostBarPercent, GhostPercentTarget, World->GetDeltaSeconds(), GhostInterpSpeed);
		GhostProgressBar->SetPercent(NextGhostBarPercent);

		if (UKismetMathLibrary::NearlyEqual_FloatFloat(NextGhostBarPercent, GhostPercentTarget))
		{
			World->GetTimerManager().ClearTimer(PercentInterpTimerHandle);
		}
	}
}
