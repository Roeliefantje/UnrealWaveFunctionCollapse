// Fill out your copyright notice in the Description page of Project Settings.
#include "WFCAlgorithm.h"
#include <algorithm>
#include <cmath>
#include <bitset>

WFCAlgorithm::FTile WFCAlgorithm::FTile::GetCWRotatedTile(int Rotations) const
{
	std::vector<EPixelValues> rotated = Pixels;
	
	for (int i = 0; i < Rotations; i++)
	{
		std::vector<EPixelValues> start= rotated;
		
		for (int y = 0; y < Dim; y++)
		{
			for (int x = 0; x < Dim; x++)
			{
				int OldIndex = y * Dim + x;
				//transpose and flip the order
				int NewIndex = x * Dim + (Dim - 1 - y);
				rotated[NewIndex] = start[OldIndex];
			}
		}
	}
	
	
	
	return FTile{rotated, Dim};
}

WFCAlgorithm::WFCAlgorithm(const std::vector<FTile> &PossibleTiles, int width, int height, int TileDimensions) : Width(width), Height(height), Possible_tileset(PossibleTiles), TileDimensions(TileDimensions)
{
	Grid = std::make_unique<FGridTile[]>(width * height);
	constructRuleset();
	
	//Initialize all the grid values with the same entropy, meaning they all can be all possible states.
	//TODO!: It would be fun if we could set an initial state through some functions instead of doing it here.
	//TODO!: So we would init all this, and then in the editor allow certain tiles to be set to a type.
	check(!(PossibleTiles.size() > 32 || PossibleTiles.size() == 0))
	
	FGridTile BaseTile;
	BaseTile.Collapsed = false;
	//Set the first n bits of the bitmask to 1, allowing all the tiles.
	if (PossibleTiles.size() == 32)
	{
		BaseTile.PossibleOptions = ~0u;
	} else
	{
		BaseTile.PossibleOptions = (1u << PossibleTiles.size()) - 1;
	}
	
	
	//Use std::fill instead of a loop as memcpy is probably a bit more efficient.
	//Fill will deepcopy the struct, so the vectors are still seperate, we could look into using a pointer to a vec instead,
	//If we then have a shared_ptr for each different instance of possibilities, we could potentially save some memory.
	std::fill(Grid.get(), Grid.get() + Width * Height, BaseTile);
	
	//Init the entropy buckets and add all cells to highest bucket
	for (int i = 0; i <= Possible_tileset.size(); i++)
	{
		if (i == Possible_tileset.size())
		{
			std::vector<FBucketItem> final_bucket = std::vector<FBucketItem>();
			final_bucket.reserve(width * height);
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					final_bucket.emplace_back(x, y);
				}
			}
			EntropyBuckets.push_back(final_bucket);
		} else
		{
			EntropyBuckets.emplace_back(0);
		}
		
	}
}

FString UInt32ToBinary(uint32 Value)
{
	auto BitSet = std::bitset<32>(Value);
	// BitSet.to_string();
	
	return FString(BitSet.to_string().c_str());
	
	FString Result;
	for (int i = 31; i >= 0; --i)
	{
		Result += (Value & (1u << i)) ? TEXT("1") : TEXT("0");

		// Optional: spacing every 4 bits for readability
		if (i % 4 == 0) Result += TEXT(" ");
	}
	return Result;
}

void WFCAlgorithm::LogNeighbourBitmasks(FTileNeighbours NBInfo)
{
	UE_LOG(LogTemp, Log, TEXT("Value (bin): %s"), *UInt32ToBinary(NBInfo.North));
	UE_LOG(LogTemp, Log, TEXT("Value (bin): %s"), *UInt32ToBinary(NBInfo.East));
	UE_LOG(LogTemp, Log, TEXT("Value (bin): %s"), *UInt32ToBinary(NBInfo.South));
	UE_LOG(LogTemp, Log, TEXT("Value (bin): %s"), *UInt32ToBinary(NBInfo.West));
}

//This function constructs a map of the ruleset to ensure
void WFCAlgorithm::constructRuleset()
{
	for (const auto &possibleTile : Possible_tileset)
	{
		UE_LOG(LogTemp, Log, TEXT("Rule set for tile"));
		auto nbRuleSet = FTileNeighbours{};
		// LogNeighbourBitmasks(nbRuleSet);
		for (size_t i = 0; i < Possible_tileset.size(); i++)
		{
			const auto &nbTile = Possible_tileset[i];
			if (possibleNorth(possibleTile, nbTile))
			{
				//Set the bitmask to 1 for that neighbour.
				nbRuleSet.North |= (1u << i);
			}
			if (possibleEast(possibleTile, nbTile))
			{
				// nbRuleSet.East.push_back(nbTile);
				nbRuleSet.East |= (1u << i);
			}
			if (possibleWest(possibleTile, nbTile))
			{
				// nbRuleSet.South.push_back(nbTile);
				nbRuleSet.West |= (1u << i);
			}
			if (possibleSouth(possibleTile, nbTile))
			{
				// nbRuleSet.West.push_back(nbTile);
				nbRuleSet.South |= (1u << i);
			}
		}
		//LogNeighbourBitmasks(nbRuleSet);
		TileRuleset.push_back(nbRuleSet);
	}
}

inline bool WFCAlgorithm::possibleNorth(const FTile& Curr, const FTile& Nb) const
{
	//We need to compare the top row with the bottom row, which is done by calculating the dimensions of the tile and
	//then comparing them. The index for the bottom left value is the total size - the dimension, for example: 9 - 3 = 6
	int TileSize = TileDimensions * TileDimensions;
	for (int i = 0; i < TileDimensions; i++)
	{
		if (Curr.Pixels[i] != Nb.Pixels[TileSize - TileDimensions + i])
		{
			return false;
		}
	}
	
	return true;
}

inline bool WFCAlgorithm::possibleEast(const FTile& Curr, const FTile& Nb) const
{
	//We need to compare the right side values with the left side values.
	for (int i = 0; i < TileDimensions; i++)
	{
		if (Curr.Pixels[TileDimensions - 1 + TileDimensions * i] != Nb.Pixels[TileDimensions * i])
		{
			return false;
		}
	}
	
	return true;
}

inline bool WFCAlgorithm::possibleSouth(const FTile& Curr, const FTile& Nb) const
{
	//South is just north but flipped
	int TileSize = TileDimensions * TileDimensions;
	for (int i = 0; i < TileDimensions; i++)
	{
		if (Curr.Pixels[TileSize - TileDimensions + i] != Nb.Pixels[i])
		{
			return false;
		}
	}
	
	return true;
}

inline bool WFCAlgorithm::possibleWest(const FTile& Curr, const FTile& Nb) const
{
	//West is East but flipped
	for (int i = 0; i < TileDimensions; i++)
	{
		if (Curr.Pixels[TileDimensions * i] != Nb.Pixels[TileDimensions - 1 + TileDimensions * i])
		{
			return false;
		}
	}
	
	return true;
}

inline size_t RandomIndex(size_t vecSize) 
{
	return 0;
}

WFCAlgorithm::FBucketItem WFCAlgorithm::GetLowestEntropyGridTile()
{
	//Iterate over the buckets until an non-zero vector is found.
	//Fetch a random cell from it, check if the entropy is valid.
	//If entropy = 0, we are cooked!
	for (size_t i = 0; i < EntropyBuckets.size(); i++)
	{
		auto& Bucket = EntropyBuckets[i];
		if (Bucket.size() > 0)
		{
			while (!Bucket.empty())
			{
				auto index = RandomIndex(Bucket.size());
				auto BucketItem = Bucket[index];
				auto& GridTile = Grid[BucketItem.YIndex * Width + BucketItem.XIndex];
				//Check the tiles entropy and check to see if its still a valid bucket entry.
				int OptionsCount = __popcnt(GridTile.PossibleOptions);
				
				
				//Overwrite the current index value and pop the last entry in the vec
				//We do not care about the order and this allows us to remove an item in O(1)
				//We do this before checking if its valid as returning it here means it will also no longer be valid.
				Bucket[index] = std::move(Bucket.back());
				Bucket.pop_back();
				
				if (!GridTile.Collapsed && OptionsCount == i)
				{
					//Found valid lowest-entropy item
					return BucketItem;
				}
				
			}
		}
	}
	
	return FBucketItem{-1, -1};
}

inline int GetNthOneBitIdx(const uint32_t Bitmask, int N)
{
	int count = 0;
	for (int i = 0; i < 32; i++)
	{
		if (Bitmask & (1u << i))
		{
			if (count == N)
			{
				return i;
			}
			count++;
		}
	}
	
	return -1;
}

void WFCAlgorithm::UpdateNeighbour(int x, int y, uint32_t bitMask)
{
	if (x < 0 || x >= Width || y < 0 || y >= Height)
	{
		return;
	}
	
	FGridTile& Tile = Grid[y * Width + x];
	int PriorCount = __popcnt(Tile.PossibleOptions);
	Tile.PossibleOptions &= bitMask;
	int OptionsCount = __popcnt(Tile.PossibleOptions);
	
	if (PriorCount == OptionsCount)
	{
		//No changes, do not need to update neighbours.
		return;
	}
	
	//Add itself to the buckets
	EntropyBuckets[OptionsCount].emplace_back(x, y);
	
	if (OptionsCount == 0)
	{
		//Oh boy
		//TODO!: Handle this case...
		return;
	}
	
	if (OptionsCount == 1)
	{
		Tile.Collapsed = true;
	}
	
	//Create a Neighbour bitmask that combines all the neighbours for all possible options in all directions
	int count = 0;
	FTileNeighbours CombinedNeighbours = {};
	for (int i = 0; i < Possible_tileset.size(); i++)
	{
		if (Tile.PossibleOptions & (1u << i))
		{
			CombinedNeighbours.AddPossibleNeighbours(TileRuleset[i]);
			
			if (++count == OptionsCount)
			{
				break;
			}
		}
	}
	
	UpdateNeighbour(x, y - 1, CombinedNeighbours.North);
	UpdateNeighbour(x, y + 1, CombinedNeighbours.South);
	UpdateNeighbour(x - 1, y, CombinedNeighbours.West);
	UpdateNeighbour(x + 1, y, CombinedNeighbours.East);
	
}

bool WFCAlgorithm::Step()
{
	//Get one of the lowest entropy grid tiles.
	FBucketItem GridCoords = GetLowestEntropyGridTile();
	if (GridCoords.XIndex == -1)
	{
		return true;
	}
	//Collapse the cell to a random possible cell
	FGridTile& Tile = Grid[GridCoords.YIndex * Width + GridCoords.XIndex];
	int OptionsCount = __popcnt(Tile.PossibleOptions);
	
	if (OptionsCount == 0)
	{
		//We have encountered an illegal state
		//do something
		//TODO!: Handle this.
		return true;
	}
	
	Tile.Collapsed = true;
	int VariantIndex = GetNthOneBitIdx(Tile.PossibleOptions, FMath::RandRange(0, OptionsCount - 1));
	Tile.PossibleOptions = (1u << VariantIndex);
	
	//Update neighbours, and their neighbours and so on.
	UpdateNeighbour(GridCoords.XIndex, GridCoords.YIndex - 1, TileRuleset[VariantIndex].North);
	UpdateNeighbour(GridCoords.XIndex, GridCoords.YIndex + 1, TileRuleset[VariantIndex].South);
	UpdateNeighbour(GridCoords.XIndex - 1, GridCoords.YIndex, TileRuleset[VariantIndex].West);
	UpdateNeighbour(GridCoords.XIndex + 1, GridCoords.YIndex, TileRuleset[VariantIndex].East);
	
	return false;
}

std::vector<EPixelValues> WFCAlgorithm::Solve()
{
	while (!Step())
	{
		//TODO!: Keep track of branches whenever we collapse something
		// break;
	}
	
	int ResultWidth = Width * TileDimensions;
	int ResultHeight = Height * TileDimensions;
	auto result = std::vector<EPixelValues>(ResultWidth * ResultHeight, EPixelValues::Invalid);
	// std::fill_n(result.begin(), ResultWidth * ResultHeight, EPixelValues::Invalid);
	
	
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			FGridTile& GridTile = Grid[y * Width + x];
			if (GridTile.Collapsed)
			{
				// UE_LOG(LogTemp, Log, TEXT("COLLAPSED GRID TILE"));
				int TileIndex = GetNthOneBitIdx(GridTile.PossibleOptions, 0);
				if (TileIndex >= Possible_tileset.size() || TileIndex == -1)
				{
					UE_LOG(LogTemp, Error, TEXT("TileIndex surprassed tileset size!"));
					UE_LOG(LogTemp, Error, TEXT("%d"), TileIndex);
					continue;
				}
				const FTile& ChosenTile = Possible_tileset[TileIndex];
				
				//Copy the pixel values into the resulting array.
				for (int TileY = 0; TileY < TileDimensions; TileY++)
				{
					for (int TileX = 0; TileX < TileDimensions; TileX++)
					{
						int ResultIndex = ((y * TileDimensions + TileY) * ResultWidth) + (x * TileDimensions + TileX);
						int InTileIndex   = TileY * TileDimensions + TileX;
						result[ResultIndex] = ChosenTile.Pixels[InTileIndex];
					}
				}
			} 
		}
	}
	
	return result;
}


WFCAlgorithm::~WFCAlgorithm()
{
}
