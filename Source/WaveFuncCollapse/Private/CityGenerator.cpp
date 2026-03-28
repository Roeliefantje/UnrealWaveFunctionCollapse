// Fill out your copyright notice in the Description page of Project Settings.


#include "CityGenerator.h"

#include <cmath>
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
	auto tiles = FTilesFromTileSetData();
	

	WFCAlgorithm::FTile Grass = {
		{
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile StraightRoadGrass = {
		{
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
		}};
	WFCAlgorithm::FTile StraightRoadRotatedGrass = {
		{
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
			EPixelValues::Road, EPixelValues::Road, EPixelValues::Road,
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile PlusGrass = {
		{
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
			EPixelValues::Road, EPixelValues::Road, EPixelValues::Road,
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn = {
		{
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Road,
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn2 = {
		{
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
			EPixelValues::Road, EPixelValues::Road, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn3 = {
		{
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
			EPixelValues::Road, EPixelValues::Road, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn4 = {
		{
			EPixelValues::Grass, EPixelValues::Grass, EPixelValues::Grass,
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Road,
			EPixelValues::Grass, EPixelValues::Road, EPixelValues::Grass,
		}};
	
	std::vector<WFCAlgorithm::FTile> TileSet = {Grass, StraightRoadGrass, StraightRoadRotatedGrass, PlusGrass, Turn, Turn2, Turn3, Turn4};
	// std::vector<WFCAlgorithm::FTile> TileSet = {Grass, StraightRoad, Turn};
	int TileDim = TilesetData->TileDimensions;
	
	int Height = GridHeight / TileDim;
	int Width = GridWidth / TileDim;
	
	//Make sure the GridHeight and GridWith also get adjusted back so the resulting spawned grid is not wrong.
	GridHeight = Height * TileDim;
	GridWidth = Width * TileDim;
	
	auto Wfc = WFCAlgorithm(TileSet, Width, Height, TileDim);
	UE_LOG(LogTemp, Log, TEXT("Solving WFC algo"));
	auto Pixels = Wfc.Solve();
	
	int ResultHeight = Height * TileDim;
	int ResultWidth = Width * TileDim;
	
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
	
	SpawnMeshes(Pixels);
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
				const FColor PixelColor = FColor(RawImageData[PixelCoord],
										RawImageData[PixelCoord + 1],
										RawImageData[PixelCoord + 2],
										RawImageData[PixelCoord + 3]);
				//TODO!: We should have a func that gets the closest color just in case.
				if (TilesetData->ColorToCellType.Contains(PixelColor))
				{
					TilePixels.push_back(TilesetData->ColorToCellType[PixelColor]);
					UE_LOG(LogTemp, Log, TEXT("%d"), static_cast<int32>(TilesetData->ColorToCellType[PixelColor]));
				} else {
					UE_LOG(LogTemp, Log, TEXT("Texture not found, Color: %s"), *PixelColor.ToString());
				}
			}
		}
		MipMap.BulkData.Unlock();
		
		// WFCAlgorithm::FTile FTile =  WFCAlgorithm::FTile{TilePixels};
		FinalTiles.emplace_back(TilePixels);
		
	}
	
	UE_LOG(LogTemp, Log, TEXT("Tiles loaded: %llu"), FinalTiles.size());
	return FinalTiles;
}


void ACityGenerator::SpawnMeshes(std::vector<EPixelValues> Pixels)
{
	FVector Origin = GetActorLocation();
	
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
				//Invalid mesh...
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

