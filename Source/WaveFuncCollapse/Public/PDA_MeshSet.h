// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PDA_MeshSet.generated.h"

/**
 * 
 */
UCLASS()
class WAVEFUNCCOLLAPSE_API UPDA_MeshSet : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UStaticMesh*> HouseMeshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UStaticMesh*> RoadMeshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Default")
	TArray<UStaticMesh*> GrassMeshes;
};
