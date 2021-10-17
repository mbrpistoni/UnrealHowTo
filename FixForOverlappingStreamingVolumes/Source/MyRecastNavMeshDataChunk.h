#pragma once
#include "NavMesh/RecastNavMeshDataChunk.h"
#include "MyRecastNavMeshDataChunk.generated.h"

struct FNavMeshTileData;
class ANavMeshBoundsVolume;

// Extension of the engine class to correct a bug that happens while streaming in/out
// https://udn.unrealengine.com/s/question/0D74z00000AHv3p/detail?s1oid=00D1a000000L44R
UCLASS()
class MY_API UMyRecastNavMeshDataChunk : public URecastNavMeshDataChunk
{
	GENERATED_BODY()

public:
	/** Detaches tiles from specified navmesh, taking tile ownership and outputs a collection of
	 * Nav Chunks that are now missing tiles after detaching this nav data chunk (shared tiles)*/
	TArray<uint32> DetachTiles(FPImplRecastNavMesh& navMeshImpl, TArray<URecastNavMeshDataChunk*>& outChunksToRebuild);

private:
	/** Detaches tiles from specified navmesh and returns an array of URecastNavMeshDataChunk where we will need to re-add
	* tiles to the nav due some tiles being removed by another level during this detach process.
	* As in: you detached tiles from a sublevel, but those tiles are also needed in a different sublevel
	* @param NavMeshImpl
	* @param bTakeDataOwnership
	* @param bTakeCacheDataOwnership
	* @out outChunksToRebuild: the nav data chunks that belong to a different level and that were affected by this detach process*/
	TArray<uint32> DetachTiles(FPImplRecastNavMesh& navMeshImpl, const bool bTakeDataOwnership,
		const bool bTakeCacheDataOwnership, TArray<URecastNavMeshDataChunk*>& outChunksToRebuild);

	/** Iterates through all levels available(except the one owning this chunk) and returns all the nav bounds volumes available*/
	void GetAvailableLevelsNavBoundsVolumes(class ARecastNavMesh* navRecast, TArray<ANavMeshBoundsVolume*>& outNavBoundsVolumes);

	/** Checks if a tile just detached will affect other nav chunks in other levels by checking if its bounds intersect
	* any Nav Volume in any other sub-level. If it does, we store the nav chunk in the out array*/
	void ProcessOverlappingTileAndStoreAffectedNavChunk(class ARecastNavMesh* navRecast, const int32 tileIndex,
		TArray<ANavMeshBoundsVolume*> navBoundsToCheckAgainst, TArray<URecastNavMeshDataChunk*>& outAffectedNavChunks);
};