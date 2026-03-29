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
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Houses")
	TArray<UStaticMesh*> House1x1Meshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Houses")
	TArray<UStaticMesh*> House2x2Meshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Houses")
	TArray<UStaticMesh*> House3x2Meshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Roads")
	TArray<UStaticMesh*> Road2x1Meshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Roads")
	TArray<UStaticMesh*> Road2x2Meshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Grass")
	TArray<UStaticMesh*> Grass1x1Meshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Pavement")
	TArray<UStaticMesh*> Pavement1x1Meshes;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Invalid")
	TArray<UStaticMesh*> Invalid1x1Meshes;
};
