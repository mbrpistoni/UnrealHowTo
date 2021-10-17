#include "MyRecastNavMesh.h"

#include "NavMesh/PImplRecastNavMesh.h"

#include "MyRecastNavMeshDataChunk.h"
#include "NavigationSystem.h"//UNavigationSystemV1

TAutoConsoleVariable<bool> CVarMyEnableCustomNavChunk(
	TEXT("my.EnableCustomNavChunk"),
	true,
	TEXT("Enables custom nav chunk that will enable all the optimizations for nav"),
	ECVF_Cheat
);

void AMyRecastNavMesh::DetachNavMeshDataChunk(URecastNavMeshDataChunk& navDataChunk)
{
	if (UMyRecastNavMeshDataChunk* myNavDataChunk = Cast<UMyRecastNavMeshDataChunk>(&navDataChunk))
	{
		FPImplRecastNavMesh* recastImpl = GetRecastNavMeshImpl();
		if (ensureMsgf(recastImpl != nullptr, TEXT("%s no valid RecastImpl!"), *FString(__FUNCTION__)))
		{
			TArray<URecastNavMeshDataChunk*> navDataChunksToRebuild;
			TArray<uint32> DetachedIndices = myNavDataChunk->DetachTiles(*recastImpl, navDataChunksToRebuild);
			if (DetachedIndices.Num() > 0)
			{
				// re-attach missing tiles back to the nav that the currently loaded nav chunks also need
				if (navDataChunksToRebuild.Num() > 0)
				{
					for (URecastNavMeshDataChunk* chunk : navDataChunksToRebuild)
					{
						if (::IsValid(chunk))
						{
							chunk->AttachTiles(*recastImpl);
						}
					}
				}

				InvalidateAffectedPaths(DetachedIndices);
				RequestDrawingUpdate();
			};
		}
	}
	else
	{
		Super::DetachNavMeshDataChunk(navDataChunk);
	}
}

void AMyRecastNavMesh::OnNavMeshGenerationFinished()
{
	// Copy/pasted from the engine original. Only change is that we create our own version of URecastNavMeshDataChunk (UMyRecastNavMeshDataChunk)
	UWorld* world = GetWorld();

	if (world != nullptr && world->IsPendingKill() == false)
	{
#if WITH_EDITOR
		// For navmeshes that support streaming create navigation data holders in each streaming level
		// so parts of navmesh can be streamed in/out with those levels
		if (!world->IsGameWorld())
		{
			const auto& levels = world->GetLevels();
			for (auto level : levels)
			{
				if (level->IsPersistentLevel())
				{
					continue;
				}

				URecastNavMeshDataChunk* navDataChunk = GetNavigationDataChunk(level);

				if (SupportsStreaming())
				{
					// We use navigation volumes that belongs to this streaming level to find tiles we want to save
					TArray<int32> levelTiles;
					TArray<FBox> levelNavBounds = GetNavigableBoundsInLevel(level);

					FPImplRecastNavMesh* recastNavMeshImpl = GetRecastNavMeshImpl();
					recastNavMeshImpl->GetNavMeshTilesIn(levelNavBounds, levelTiles);

					if (levelTiles.Num())
					{
						// Create new chunk only if we have something to save in it
						if (navDataChunk == nullptr)
						{
							if (CVarMyEnableCustomNavChunk.GetValueOnGameThread())
							{
								navDataChunk = NewObject<UMyRecastNavMeshDataChunk>(level);
							}
							else
							{
								navDataChunk = NewObject<URecastNavMeshDataChunk>(level);
							}
							navDataChunk->NavigationDataName = GetFName();
							level->NavDataChunks.Add(navDataChunk);
						}

						const EGatherTilesCopyMode CopyMode = recastNavMeshImpl->NavMeshOwner->SupportsRuntimeGeneration()
							? EGatherTilesCopyMode::CopyDataAndCacheData : EGatherTilesCopyMode::CopyData;
						navDataChunk->GetTiles(recastNavMeshImpl, levelTiles, CopyMode);
						navDataChunk->MarkPackageDirty();
						continue;
					}
				}

				// stale data that is left in the level
				if (navDataChunk)
				{
					// clear it
					navDataChunk->ReleaseTiles();
					navDataChunk->MarkPackageDirty();
					level->NavDataChunks.Remove(navDataChunk);
				}
			}
		}

		// force navmesh drawing update
		RequestDrawingUpdate(/*bForce=*/true);
#endif// WITH_EDITOR

		UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world);
		if (navSys)
		{
			navSys->OnNavigationGenerationFinished(*this);
		}
	}
}