// Fill out your copyright notice in the Description page of Project Settings.


#include "CityGenerator.h"

#include <cmath>

#include "WFCAlgorithm.h"

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
	WFCAlgorithm::FTile Grass = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile StraightRoadGrass = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
		}};
	WFCAlgorithm::FTile StraightRoadRotatedGrass = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile PlusGrass = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn2 = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn3 = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
		}};
	
	WFCAlgorithm::FTile Turn4 = {
		{
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Grass,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Road,
			WFCAlgorithm::EPixelValues::Grass, WFCAlgorithm::EPixelValues::Road, WFCAlgorithm::EPixelValues::Grass,
		}};
	
	std::vector<WFCAlgorithm::FTile> TileSet = {Grass, StraightRoadGrass, StraightRoadRotatedGrass, PlusGrass, Turn, Turn2, Turn3, Turn4};
	// std::vector<WFCAlgorithm::FTile> TileSet = {Grass, StraightRoad, Turn};
	int Height = 40;
	int Width = 40;
	
	auto Wfc = WFCAlgorithm(TileSet, Width, Height);
	UE_LOG(LogTemp, Log, TEXT("Solving WFC algo"));
	auto Pixels = Wfc.Solve();
	
	int ResultHeight = Height * std::sqrt(WFCAlgorithm::TileSize);
	int ResultWidth = Width * std::sqrt(WFCAlgorithm::TileSize);
	
	for (int32 y = 0; y < ResultHeight; y++)
	{
		FString RowString;
		for (int32 x = 0; x < ResultWidth; x++)
		{
			int32 Index = y * ResultWidth + x;
			// UE_LOG(LogTemp, Log, TEXT("%d "), Index);
			// Cast enum class to int32 for printing
			RowString += FString::Printf(TEXT("%d "), static_cast<int32>(Pixels[Index]));
		}
		UE_LOG(LogTemp, Log, TEXT("%s"), *RowString);
	}
	
	
}

// Called every frame
void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

