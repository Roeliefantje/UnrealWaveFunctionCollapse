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
private:
	struct GridGroup
	{
		int StartX;
		int StartY;
		int SizeX;
		int SizeY;
		EPixelValues GroupType;
	};
	
	std::vector<WFCAlgorithm::FTile> FTilesFromTileSetData() const;
	
	UStaticMesh* GetSpawnMesh(EPixelValues PixelType, int SizeX, int SizeY) const;
	void SpawnMeshes(const std::vector<GridGroup>& Groups);
	std::vector<GridGroup> CreateGridGroups(const std::vector<EPixelValues>& Pixels) const;
	void CreateHouseGroups(std::vector<GridGroup>& Groups, TMap<int, bool>& HasGroup, const int StartX, const int StartY, const int SizeX, const int SizeY) const;
	void CreateRoadGroups(std::vector<GridGroup>& Groups, TMap<int, bool>& HasGroup, const int StartX, const int StartY, const int SizeX, const int SizeY, const std::vector<EPixelValues>& Pixels) const;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
