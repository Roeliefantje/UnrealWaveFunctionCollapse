// Fill out your copyright notice in the Description page of Project Settings.


#include "CityGenerator.h"

#include <cmath>

#include "ParticleEmitterInstances.h"
#include "WFCAlgorithm.h"
#include "Engine/StaticMeshActor.h"

// Sets default values
ACityGenerator::ACityGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();
	const auto Tiles = FTilesFromTileSetData();
	const int TileDim = TilesetData->TileDimensions;
	
	const int Height = GridHeight / TileDim;
	const int Width = GridWidth / TileDim;
	
	//Make sure the GridHeight and GridWith also get adjusted back so the resulting spawned grid is not wrong.
	GridHeight = Height * TileDim;
	GridWidth = Width * TileDim;
	
	auto Wfc = WFCAlgorithm(Tiles, Width, Height, TileDim);
	UE_LOG(LogTemp, Log, TEXT("Solving WFC algo"));
	auto Pixels = Wfc.Solve();
	
	// int ResultHeight = Height * TileDim;
	// int ResultWidth = Width * TileDim;
	//
	// for (int32 y = 0; y < ResultHeight; y++)
	// {
	// 	FString RowString;
	// 	for (int32 x = 0; x < ResultWidth; x++)
	// 	{
	// 		int32 Index = y * ResultWidth + x;
	// 		// UE_LOG(LogTemp, Log, TEXT("%d "), Index);
	// 		// Cast enum class to int32 for printing
	// 		RowString += FString::Printf(TEXT("%d "), static_cast<int32>(Pixels[Index]));
	// 	}
	// 	UE_LOG(LogTemp, Log, TEXT("%s"), *RowString);
	// }
	
	const std::vector<GridGroup> Groups = CreateGridGroups(Pixels);
	
	SpawnMeshes(Pixels);
}

void AddIfNotDuplicate(std::vector<WFCAlgorithm::FTile>& Vec, WFCAlgorithm::FTile Obj)
{
	for (const auto& ExistingTile : Vec)
	{
		if (ExistingTile.Pixels == Obj.Pixels)
		{
			return;
		}
	}
	
	Vec.push_back(Obj);
}

std::vector<WFCAlgorithm::FTile> ACityGenerator::FTilesFromTileSetData() const
{	
	auto FinalTiles = std::vector<WFCAlgorithm::FTile>();
	
	if (TilesetData == nullptr)
	{
		return FinalTiles;
	}
	
	for (auto TileTexture : TilesetData->Tiles) {
		if (TileTexture == nullptr)
		{
			continue;
		}
		UE_LOG(LogTemp, Log, TEXT("Reading tile texture."));
		auto &MipMap = TileTexture->GetPlatformMips()[0];
		
		if (TileTexture->GetPlatformData()->PixelFormat != PF_B8G8R8A8)
		{
			//Oodle texture plugin overrides the compression settings because that is lovely.
			//So make sure to disable the plugin in case changing the settings doesn't change anything.
			UE_LOG(LogTemp, Log, TEXT("Unsupported Pixel format!, make sure to change the compression settings."));
			const EPixelFormat& PixelFormat = TileTexture->GetPlatformData()->PixelFormat;
			static const UEnum* PixelFormatEnum = StaticEnum<EPixelFormat>();
			const FString PixelFormatName = PixelFormatEnum ? PixelFormatEnum->GetNameStringByValue((int64)PixelFormat) : TEXT("Unknown");
			UE_LOG(LogTemp, Log, TEXT("Pixel Format: %s (%d)"), *PixelFormatName, (int32)PixelFormat);
		}
		
		
		std::vector<EPixelValues> TilePixels = std::vector<EPixelValues>();
		TilePixels.reserve(TilesetData->TileDimensions * TilesetData->TileDimensions);
		
		const uint8* RawImageData = static_cast<const uint8*>(MipMap.BulkData.LockReadOnly());
		
		for (int y = 0; y < TilesetData->TileDimensions; y++)
		{
			for (int x = 0; x < TilesetData->TileDimensions; x++)
			{
				const size_t PixelCoord = (y * MipMap.SizeX + x) * 4;
				const FColor PixelColor = FColor(RawImageData[PixelCoord + 2],
										RawImageData[PixelCoord + 1],
										RawImageData[PixelCoord],
										RawImageData[PixelCoord + 3]);
				//TODO!: We should have a func that gets the closest color just in case.
				if (TilesetData->ColorToCellType.Contains(PixelColor))
				{
					TilePixels.push_back(TilesetData->ColorToCellType[PixelColor]);
					// UE_LOG(LogTemp, Log, TEXT("%d"), static_cast<int32>(TilesetData->ColorToCellType[PixelColor]));
				} else {
					UE_LOG(LogTemp, Log, TEXT("Texture not found, Color: %s"), *PixelColor.ToString());
					TilePixels.push_back(EPixelValues::Invalid);
				}
			}
		}
		MipMap.BulkData.Unlock();
		
		//Add it and its rotations to the set
		WFCAlgorithm::FTile Tile =  WFCAlgorithm::FTile{TilePixels, TilesetData->TileDimensions};
		AddIfNotDuplicate(FinalTiles, Tile);
		AddIfNotDuplicate(FinalTiles, Tile.GetCWRotatedTile(1));
		AddIfNotDuplicate(FinalTiles, Tile.GetCWRotatedTile(2));
		AddIfNotDuplicate(FinalTiles, Tile.GetCWRotatedTile(3));
		
	}
	
	UE_LOG(LogTemp, Log, TEXT("Tiles loaded: %llu"), FinalTiles.size());
	return FinalTiles;
}


void ACityGenerator::CreateHouseGroups(std::vector<ACityGenerator::GridGroup>& Groups, TMap<int, bool>& HasGroup, const int SizeX, const int SizeY, const int FlattenedIndex) const
{
	//If both are divisible by 2, subdivide the groups into 2x2 houses
	if ( (SizeY & 1) == 0 && (SizeX & 1) == 0)
	{
		//This looks like a nested loop but in all cases either SizeY or SizeX should be 2
		for (int OffsetY = 0; OffsetY < SizeY; OffsetY +=2)
		{
			for (int OffsetX = 0; OffsetX < SizeX; OffsetX +=2)
			{
				int StartIndex = FlattenedIndex + OffsetY * GridWidth + OffsetX;
				Groups.emplace_back(StartIndex, 2, 2, EPixelValues::House);
				HasGroup.Emplace(StartIndex, true);
				HasGroup.Emplace(StartIndex + 1, true);
				HasGroup.Emplace(StartIndex + GridWidth, true);
				HasGroup.Emplace(StartIndex + GridWidth + 1, true);
			}
		}
	} else if ((SizeX == 3 && SizeY == 2) || (SizeY == 3 && SizeX == 2)) {
		//3x2 houses.
		Groups.emplace_back(ENABLE_TRAILS_START_END_INDEX_OPTIMIZATION, (SizeX == 3 ? 3 : 2), (SizeY == 3 ? 3 : 2), EPixelValues::House);
		for (int OffsetY = 0; OffsetY < SizeY; OffsetY++)
		{
			for (int OffsetX = 0; OffsetX < SizeX; OffsetX++)
			{
				HasGroup.Emplace(FlattenedIndex + OffsetY * GridWidth + OffsetX, true);
			}
		}
	} else {
		//Shouldn't be able to get here
		UE_LOG(LogTemp, Warning, TEXT("Flattened index of type House has illegal group size: %d"), FlattenedIndex);
	}
}
void ACityGenerator::CreateRoadGroups(std::vector<ACityGenerator::GridGroup>& Groups, TMap<int, bool>& HasGroup, const int SizeX, const int SizeY, const int FlattenedIndex, const std::vector<EPixelValues>& Pixels) const
{
	//Intersection, make a 2x2 group
	//Technically there could be a 2x2 road after an intersection as well, but I dont think its in our current tileset.
	if (SizeY > 2 && SizeX > 2)
	{
		Groups.emplace_back(FlattenedIndex, 2, 2, EPixelValues::Road);
		HasGroup.Emplace(FlattenedIndex, true);
		HasGroup.Emplace(FlattenedIndex + 1, true);
		HasGroup.Emplace(FlattenedIndex + GridWidth, true);
		HasGroup.Emplace(FlattenedIndex + GridWidth + 1, true);
	} else if ((SizeY == 2) || (SizeX == 2))
	{
		//In the case that Size X is 2, it is a road along the X direction, therefore we should check for intersections
		//in the X direction.
		int NeighbourOffset = (SizeX == 2) ? 1 : GridWidth;
		int StartIndexOffset = (SizeX == 2) ? GridWidth : 1;
		int LoopCount = (SizeX == 2) ? SizeY : SizeX;
		
		for (int i = 0; i < LoopCount; i++)
		{
			//Check if its not an intersection tile
			const int StartIndex = FlattenedIndex + StartIndexOffset * i;
			const int AboveNeighbourIndex = StartIndex - NeighbourOffset;
			const int BelowNeighbourIndex = StartIndex + NeighbourOffset * 2;
			
			if ((AboveNeighbourIndex >= 0 && Pixels[AboveNeighbourIndex] == EPixelValues::Road) ||
				(BelowNeighbourIndex < Pixels.size() && Pixels[BelowNeighbourIndex] == EPixelValues::Road))
			{
				break;
			}
			//Group size in X and Y are 1 or 2 depending on whether we are iterating over Y or X
			Groups.emplace_back(StartIndex, (SizeX == 2 ? 2 : 1), (SizeY == 2 ? 2 : 1), EPixelValues::Road);
			HasGroup.Emplace(StartIndex, true);
			HasGroup.Emplace(StartIndex + NeighbourOffset, true);
		}
	} else
	{
		//Shouldn't be able to get here
		UE_LOG(LogTemp, Warning, TEXT("Flattened index of type Road has illegal group size: %d"), FlattenedIndex);
	}
}


std::vector<ACityGenerator::GridGroup> ACityGenerator::CreateGridGroups(const std::vector<EPixelValues>& Pixels) const
{
	std::vector<GridGroup> Groups;
	//Since most groups are either 2x1 or 2x2, we reserve accounting for that.
	Groups.reserve((GridWidth / 2) * (GridHeight / 2));
	TMap<int, bool> HasGroup;
	
	for (int y = 0; y < GridHeight; y++)
	{
		for (int x = 0; x < GridWidth; x++)
		{
			const int FlattenedIndex = y * GridWidth + x;
			//Check if Index is already ocntained in a group
			if (HasGroup.Contains(FlattenedIndex))
			{
				continue;
			}
			
			const EPixelValues& GroupType = Pixels[FlattenedIndex];
			
			if (GroupType == EPixelValues::Grass || GroupType == EPixelValues::Invalid)
			{
				//TODO!: Add bigger size groupings to grass as well to allow for trees and stuff
				Groups.emplace_back(FlattenedIndex, 1, 1, GroupType);
				break;
			}
			
			int SizeX = 1;
			int SizeY = 1;
			while (x + SizeX < GridWidth && Pixels[FlattenedIndex + SizeX] == GroupType)
			{
				SizeX++;
			}
			while (y + SizeY < GridHeight && Pixels[FlattenedIndex + GridWidth * SizeY] == GroupType)
			{
				SizeY++;
			}
			
			if (GroupType == EPixelValues::House)
			{
				CreateHouseGroups(Groups, HasGroup, SizeX, SizeY, FlattenedIndex);
			} else if (GroupType == EPixelValues::Road)
			{
				CreateRoadGroups(Groups, HasGroup, SizeX, SizeY, FlattenedIndex, Pixels);
			}
			
			
		}
	}
	
	return Groups;
}


void ACityGenerator::SpawnMeshes(const std::vector<EPixelValues>& Pixels)
{
	const FVector Origin = GetActorLocation();
	
	for (int y = 0; y < GridHeight; ++y)
	{
		for (int x = 0; x < GridWidth; ++x)
		{
			const FVector SpawnLocation = Origin + FVector(x * GridSpacing, y * GridSpacing, 0);
			
			UStaticMesh* SpawnMesh = nullptr;
			
			switch (const EPixelValues& Pixel = Pixels[y * GridWidth + x])
			{
			case EPixelValues::Grass:
				if (MeshsetData->GrassMeshes.Num() > 0) {
					SpawnMesh = MeshsetData->GrassMeshes[0];
				}
				break;
			case EPixelValues::Road:
				if (MeshsetData->RoadMeshes.Num() > 0) {
					SpawnMesh = MeshsetData->RoadMeshes[0];
				}
				break;
			case EPixelValues::House:
				if (MeshsetData->HouseMeshes.Num() > 0) {
					SpawnMesh = MeshsetData->HouseMeshes[0];
				}
				break;
			default:
				//Invalid mesh
				break;
			}
			
			if (!SpawnMesh)
			{
				continue;
			}
			
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			AStaticMeshActor* SpawnedActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), 
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);
			
			if (SpawnedActor && SpawnedActor->GetStaticMeshComponent())
			{
				SpawnedActor->GetStaticMeshComponent()->SetStaticMesh(SpawnMesh);
				SpawnedActor->SetActorLabel(FString::Printf(TEXT("GridMesh_%d_%d"), x, y));
				// Optional: Attach spawned actor for hierarchy organization
				// SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			} else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to spawn mesh actor at grid [%d,%d]."), x, y);
			}
		}
	}
}

// Called every frame
void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

