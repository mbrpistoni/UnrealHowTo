#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "MyNavBoundsVolumeManagerSubsystem.generated.h"

/** Wrapper for the registered nav volumes data*/
USTRUCT()
struct FLevelNavBoundsContainer
{
	GENERATED_BODY()

		FLevelNavBoundsContainer() :
		OwningLevel(nullptr)
	{
		OwnedNavVolumes.Reserve(20);
	}

	// Helper constructor for easy use of Emplace
	FLevelNavBoundsContainer(ANavMeshBoundsVolume* boundInLevel)
	{
		OwningLevel = Cast<ULevel>(boundInLevel->GetLevel());
		if (::IsValid(OwningLevel))
		{
			OwnedNavVolumes.Reserve(20);
			if (::IsValid(boundInLevel))
			{
				OwnedNavVolumes.Add(boundInLevel);
			}
		}
	}

	bool IsValid() const
	{
		return ::IsValid(OwningLevel);
	}

	bool operator==(const FLevelNavBoundsContainer& other)
	{
		return OwningLevel == other.OwningLevel;
	}

	// The level that owns the nav volumes
	UPROPERTY(Transient)
		ULevel* OwningLevel = nullptr;

	// All the available nav volumes for the OwningLevel
	UPROPERTY(Transient)
		TArray<ANavMeshBoundsVolume*> OwnedNavVolumes;
};

/** Subsystem to easily get access to all nav bounds volumes available in the world*/
UCLASS()
class UMyNavBoundsVolumeManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** Add a new added volume to the level to the local collection*/
	void RegisterNavBoundsVolume(ANavMeshBoundsVolume* navBoundsToRegister);
	/** Remove a volume from the collection once it has been removed from its level*/
	void UnRegisterNavBoundsVolume(ANavMeshBoundsVolume* navBoundsToRegister);
	/** Get all nav volumes own by a level*/
	void GetVolumesFromLevel(ULevel* levelToSearch, TArray<ANavMeshBoundsVolume*>& outNavVolumesFromLevel) const;

private:
	/** Returns the container info wrapper of the passed volume (if any)*/
	FLevelNavBoundsContainer* FindContainerByNavVolume(ANavMeshBoundsVolume* navBoundsToQuery);
	/** const version of FindContainerByNavVolume*/
	const FLevelNavBoundsContainer* FindContainerByNavVolume(ANavMeshBoundsVolume* navBoundsToQuery) const;
	/** Returns the container info wrapper of the passed level (if any)*/
	FLevelNavBoundsContainer* FindContainerByLevel(ULevel* levelToQuery);
	/** const version of FindContainerByLevel*/
	const FLevelNavBoundsContainer* FindContainerByLevel(ULevel* levelToQuery) const;

	/** Outputs the container ref rather than return it as a pointer to ease removal of containers from collections
	 * @out outContainer: the container found or a default invalid one
	 * @return: true if a container was found*/
	bool FindRefContainerByLevel(ULevel* levelToQuery, FLevelNavBoundsContainer& outContainer);

	// We need this function because nav volumes are not removed from the world when the world is removed
	void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);

private:
	// The registered levels-navVolumes available in the game
	UPROPERTY(Transient)
		TArray<FLevelNavBoundsContainer> AvailableNavBounds;

	// const initializer for the collection
	const int32 InitalArraySize = 50;
};