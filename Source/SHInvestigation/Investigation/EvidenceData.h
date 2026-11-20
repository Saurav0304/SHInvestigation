// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EvidenceData.generated.h"

class UTexture2D;

/**
 * Describes one piece of evidence (name, description, icon).
 *
 * Content lives in data assets (DA_Evidence_*), not in code or in the actor, so designers can
 * add evidence without touching C++. The asset is also the evidence "identity": the GameState
 * stores references to these assets, and asset references replicate as stable network names.
 */
UCLASS(BlueprintType)
class SHINVESTIGATION_API UEvidenceData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evidence")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evidence", meta = (MultiLine = "true"))
	FText Description;

	/** Optional image shown in the inspect panel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<UTexture2D> Icon;
};
