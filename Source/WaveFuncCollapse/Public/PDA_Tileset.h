// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WFCAlgorithm.h"
#include "PDA_Tileset.generated.h"

/**
 * 
 */
UCLASS()
class WAVEFUNCCOLLAPSE_API UPDA_Tileset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UTexture2D*> Tiles;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TMap<FColor,EPixelValues> ColorToCellType;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	int32 TileDimensions;

};
