// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PDA_Tileset.h"
#include "PDA_MeshSet.h"
#include "CityGenerator.generated.h"

UCLASS()
class WAVEFUNCCOLLAPSE_API ACityGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACityGenerator();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tileset")
	UPDA_Tileset* TilesetData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	UPDA_MeshSet* MeshsetData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int GridWidth;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int GridHeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int GridSpacing = 100.0f;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	std::vector<WFCAlgorithm::FTile> FTilesFromTileSetData() const;
	
	void SpawnMeshes(const std::vector<EPixelValues>& Pixels);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
