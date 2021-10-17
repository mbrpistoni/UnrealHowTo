#include "MyNavBoundsVolumeManagerSubsystem.h"
#include "Algo/Transform.h"

void UMyNavBoundsVolumeManagerSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	AvailableNavBounds.Reserve(InitalArraySize);

	// The engine does not remove the nav volumes when removing a level from the world, so we need to listen to this to
	// clean the array
	FWorldDelegates::LevelRemovedFromWorld.AddUObject(this,
		&UMyNavBoundsVolumeManagerSubsystem::OnLevelRemovedFromWorld);
}

void UMyNavBoundsVolumeManagerSubsystem::Deinitialize()
{
	FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
	Super::Deinitialize();
}

void UMyNavBoundsVolumeManagerSubsystem::RegisterNavBoundsVolume(ANavMeshBoundsVolume* navBoundsToRegister)
{
	FLevelNavBoundsContainer* foundContainer = FindContainerByNavVolume(navBoundsToRegister);

	if (foundContainer != nullptr)
	{
		foundContainer->OwnedNavVolumes.AddUnique(navBoundsToRegister);
	}
	else
	{
		foundContainer = &AvailableNavBounds[AvailableNavBounds.Emplace(navBoundsToRegister)];
	}
}

void UMyNavBoundsVolumeManagerSubsystem::UnRegisterNavBoundsVolume(ANavMeshBoundsVolume* navBoundsToRegister)
{
	FLevelNavBoundsContainer* foundContainer = FindContainerByNavVolume(navBoundsToRegister);
	if (foundContainer != nullptr)
	{
		foundContainer->OwnedNavVolumes.RemoveSwap(navBoundsToRegister);
	}
}

void UMyNavBoundsVolumeManagerSubsystem::GetVolumesFromLevel(ULevel* levelToSearch,
	TArray<ANavMeshBoundsVolume*>& outNavVolumesFromLevel) const
{
	if (const FLevelNavBoundsContainer* foundContainer = FindContainerByLevel(levelToSearch))
	{
		auto transformLambda = [](ANavMeshBoundsVolume* navBound)
		{
			return navBound;
		};
		Algo::Transform(foundContainer->OwnedNavVolumes, outNavVolumesFromLevel, transformLambda);
	}
}

const FLevelNavBoundsContainer* UMyNavBoundsVolumeManagerSubsystem::FindContainerByNavVolume(ANavMeshBoundsVolume* navBoundsToQuery) const
{
	ULevel* owningLevel = ::IsValid(navBoundsToQuery) ? navBoundsToQuery->GetLevel() : nullptr;
	return FindContainerByLevel(owningLevel);
}

FLevelNavBoundsContainer* UMyNavBoundsVolumeManagerSubsystem::FindContainerByNavVolume(ANavMeshBoundsVolume* navBoundsToQuery)
{
	ULevel* owningLevel = ::IsValid(navBoundsToQuery) ? navBoundsToQuery->GetLevel() : nullptr;
	return FindContainerByLevel(owningLevel);
}

FLevelNavBoundsContainer* UMyNavBoundsVolumeManagerSubsystem::FindContainerByLevel(ULevel* levelToQuery)
{
	FLevelNavBoundsContainer* foundContainer = nullptr;
	if (::IsValid(levelToQuery))
	{
		auto searchLambda = [&levelToQuery](const FLevelNavBoundsContainer& container)
		{
			return container.IsValid() && (container.OwningLevel == levelToQuery);
		};
		foundContainer = AvailableNavBounds.FindByPredicate(searchLambda);
	}

	return foundContainer;
}

const FLevelNavBoundsContainer* UMyNavBoundsVolumeManagerSubsystem::FindContainerByLevel(ULevel* levelToQuery) const
{
	if (::IsValid(levelToQuery))
	{
		auto searchLambda = [&levelToQuery](const FLevelNavBoundsContainer& container)
		{
			return container.IsValid() && (container.OwningLevel == levelToQuery);
		};
		const FLevelNavBoundsContainer* foundContainer = AvailableNavBounds.FindByPredicate(searchLambda);
		return foundContainer;
	}
	return nullptr;
}

bool UMyNavBoundsVolumeManagerSubsystem::FindRefContainerByLevel(ULevel* levelToQuery, FLevelNavBoundsContainer& outContainer)
{
	FLevelNavBoundsContainer* foundContainer = nullptr;
	if (::IsValid(levelToQuery))
	{
		auto searchLambda = [&levelToQuery](const FLevelNavBoundsContainer& container)
		{
			return container.IsValid() && (container.OwningLevel == levelToQuery);
		};
		foundContainer = AvailableNavBounds.FindByPredicate(searchLambda);
	}

	outContainer = foundContainer != nullptr ? *foundContainer : FLevelNavBoundsContainer();
	return outContainer.IsValid();
}

void UMyNavBoundsVolumeManagerSubsystem::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	// take the chance to clear any invalid elements belonging to a level that might have been GC'ed
	auto removeLambda = [](const FLevelNavBoundsContainer& navContainer)
	{
		return !navContainer.IsValid();
	};
	AvailableNavBounds.RemoveAll(removeLambda);

	// a NULL object means the LoadMap case, because all levels will be
	// removed from the world without a RemoveFromWorld call for each
	if (InLevel != nullptr)
	{
		FLevelNavBoundsContainer foundContainer;
		if (FindRefContainerByLevel(InLevel, foundContainer))
		{
			AvailableNavBounds.RemoveSwap(foundContainer);
		}
	}
}