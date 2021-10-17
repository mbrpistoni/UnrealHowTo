#include "MyRecastNavMeshDataChunk.h"

#include "Algo/Transform.h"
#include "Engine/LevelStreaming.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/PImplRecastNavMesh.h"
#include "Navmesh/Public/Detour/DetourNavMesh.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavMesh/RecastNavMeshDataChunk.h"
#include "Game/MyGameInstance.h"
#include "MyNavBoundsVolumeManagerSubsystem.h"
#include "DrawDebugHelpers.h"

TAutoConsoleVariable<bool> CVarMyProjectEnableNavChunkDebug(
	TEXT("MyProject.EnableNavChunkDebug"),
	false,
	TEXT("Enables visual debug for Nav Chunk operations"),
	ECVF_Cheat
);

TArray<uint32> UMyRecastNavMeshDataChunk::DetachTiles(FPImplRecastNavMesh& navMeshImpl,
	TArray<URecastNavMeshDataChunk*>& outChunksToRebuild)
{
	check(navMeshImpl.NavMeshOwner && navMeshImpl.NavMeshOwner->GetWorld());
	const bool bIsGameWorld = navMeshImpl.NavMeshOwner->GetWorld()->IsGameWorld();

	// Keep data in game worlds (in editor we have a copy of the data so we don't keep it).
	const bool bTakeDataOwnership = bIsGameWorld;
	const bool bTakeCacheDataOwnership = bIsGameWorld;

	return DetachTiles(navMeshImpl, bTakeDataOwnership, bTakeCacheDataOwnership, outChunksToRebuild);
}

TArray<uint32> UMyRecastNavMeshDataChunk::DetachTiles(FPImplRecastNavMesh& navMeshImpl, const bool bTakeDataOwnership,
	const bool bTakeCacheDataOwnership, TArray<URecastNavMeshDataChunk*>& outChunksToRebuild)
{
	TArray<uint32> result;
	TArray<FRecastTileData> tiles = GetMutableTiles();
	result.Reserve(tiles.Num());

#if WITH_RECAST
	dtNavMesh* navMesh = navMeshImpl.DetourNavMesh;
	ARecastNavMesh* recastNav = navMeshImpl.NavMeshOwner;
	if (navMesh != nullptr)
	{
		TArray<ANavMeshBoundsVolume*> otherLevelsNavVolumesBounds;
		GetAvailableLevelsNavBoundsVolumes(recastNav, otherLevelsNavVolumesBounds);

		outChunksToRebuild.Reserve(tiles.Num());

		for (FRecastTileData& tileData : tiles)
		{
			if (tileData.bAttached)
			{
				// Detach tile cache layer and take ownership over compressed data
				dtTileRef tileRef = 0;
				const dtMeshTile* meshTile = navMesh->getTileAt(tileData.X, tileData.Y, tileData.Layer);
				if (meshTile)
				{
					tileRef = navMesh->getTileRef(meshTile);

					if (bTakeCacheDataOwnership)
					{
						FNavMeshTileData tileCacheData = navMeshImpl.GetTileCacheLayer(tileData.X, tileData.Y, tileData.Layer);
						if (tileCacheData.IsValid())
						{
							tileData.TileCacheDataSize = tileCacheData.DataSize;
							tileData.TileCacheRawData->RawData = tileCacheData.Release();
						}
					}

					navMeshImpl.RemoveTileCacheLayer(tileData.X, tileData.Y, tileData.Layer);

					if (bTakeDataOwnership)
					{
						// Remove tile from navmesh and take ownership of tile raw data
						navMesh->removeTile(tileRef, &tileData.TileRawData->RawData, &tileData.TileDataSize);
					}
					else
					{
						// In the editor we have a copy of tile data so just release tile in navmesh
						navMesh->removeTile(tileRef, nullptr, nullptr);
					}

					// gather any nav chunk in the game that will need this tile re-attached
					ProcessOverlappingTileAndStoreAffectedNavChunk(recastNav, navMesh->getTileIndex(meshTile),
						otherLevelsNavVolumesBounds, outChunksToRebuild);

					result.Add(navMesh->decodePolyIdTile(tileRef));
				}
			}

			tileData.bAttached = false;
			tileData.X = 0;
			tileData.Y = 0;
			tileData.Layer = 0;
		}
	}
#endif// WITH_RECAST

	UE_LOG(LogNavigation, Log, TEXT("Detached %d tiles from NavMesh - %s"), result.Num(), *NavigationDataName.ToString());
	return result;
}

void UMyRecastNavMeshDataChunk::GetAavilableLevelsNavBoundsVolumes(ARecastNavMesh* navRecast,
	TArray<ANavMeshBoundsVolume*>& outNavBoundsVolumes)
{
	const UWorld* world = ::IsValid(navRecast) ? navRecast->GetWorld() : nullptr;
	if (::IsValid(world))
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("URecastNavMeshDataChunk::DetachTiles gatter nav bounds"));

		TArray<ULevelStreaming*> streamedLevels = world->GetStreamingLevels();

		UMyNavBoundsVolumeManagerSubsystem* navSubsystem = nullptr;

		if (streamedLevels.Num() > 0)
		{
			const UMyGameInstance* myGameInstance = ::IsValid(world) ? world->GetGameInstance<UMyGameInstance>() : nullptr;
			navSubsystem = ::IsValid(myGameInstance) ? myGameInstance->GetSubsystem<UMyNavBoundsVolumeManagerSubsystem>() : nullptr;
		}

		for (ULevelStreaming* streamedLevel : streamedLevels)
		{
			// Level not loaded? we don't care then. Only in-use sub levels matter
			ULevel* loadedLevel = streamedLevel->GetLoadedLevel();
			if (!::IsValid(loadedLevel))
			{
				continue;
			}

			// skip the about-to-be removed data chunk
			if (navRecast->GetNavigationDataChunk(loadedLevel) == this)
			{
				continue;
			}

			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("URecastNavMeshDataChunk::DetachTiles Get nav bound actors"));

				if (::IsValid(navSubsystem))
				{
					navSubsystem->GetVolumesFromLevel(loadedLevel, outNavBoundsVolumes);
				}
				else
				{
					UE_LOG(LogNavigation, Warning, TEXT("%s Expensive path! no navSubSystem!"), *FString(__FUNCTION__));

					// grab all the nav mesh bounds from the other sub-levels
					auto conditionLambda = [](AActor* actorInLevel)
					{
						return ::IsValid(actorInLevel) && actorInLevel->IsA<ANavMeshBoundsVolume>();
					};
					auto transformLambda = [](AActor* actorInLevel)
					{
						return Cast<ANavMeshBoundsVolume>(actorInLevel);
					};

					Algo::TransformIf(loadedLevel->Actors, outNavBoundsVolumes, conditionLambda, transformLambda);
				}
			}
		}
	}
}

void UMyRecastNavMeshDataChunk::ProcessOverlappingTileAndStoreAffectedNavChunk(ARecastNavMesh* navRecast, const int32 tileIndex,
	TArray<ANavMeshBoundsVolume*> navBoundsToCheckAgainst, TArray<URecastNavMeshDataChunk*>& outAffectedNavChunks)
{
	if (::IsValid(navRecast))
	{
		const FBox& tileBounds = navRecast->GetNavMeshTileBounds(tileIndex);
		for (ANavMeshBoundsVolume* otherLevelNavBound : navBoundsToCheckAgainst)
		{
			if (::IsValid(otherLevelNavBound))
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("URecastNavMeshDataChunk::DetachTiles Intersect test"));
				const FBox& navBoundBox = otherLevelNavBound->GetBounds().GetBox();
#if ENABLE_DRAW_DEBUG
				const UWorld* world = CVarMyProjectEnableNavChunkDebug.GetValueOnGameThread() ? navRecast->GetWorld() : nullptr;
				if (::IsValid(world))
				{
					DrawDebugBox(world, navBoundBox.GetCenter(), navBoundBox.GetExtent(), FQuat::Identity, FColor::Red, false, 10.f);
					const FVector customExtent{ tileBounds.GetExtent().X, tileBounds.GetExtent().Y, 50.f };
					DrawDebugBox(world, tileBounds.GetCenter(), customExtent, FQuat::Identity, FColor::Orange, false, 10.f);
				}
#endif // ENABLE_DRAW_DEBUG
				// !tileBounds.IsValid: sometimes the bounds of a tile are not valid, which makes the Intersect test to fail
				// no matter what. This causes issues so, to prevent problems (tiles we need available but that actually being
				// detached forever), we force this failing tiles to be present again.
				if (!tileBounds.IsValid || FBoxSphereBounds::BoxesIntersect(navBoundBox, tileBounds))
				{
					URecastNavMeshDataChunk* navChunk = navRecast->GetNavigationDataChunk(otherLevelNavBound->GetLevel());
					outAffectedNavChunks.Emplace(navChunk);
				}
			}
		}
	}
}