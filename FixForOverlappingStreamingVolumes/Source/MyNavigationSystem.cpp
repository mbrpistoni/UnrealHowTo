#include "MyNavigationSystem.h"

#include "NavMesh/NavMeshBoundsVolume.h"

#include "Game/MyGameInstance.h"
#include "MyNavBoundsVolumeManagerSubsystem.h"

DEFINE_LOG_CATEGORY(LogMyNavigationSystem);

void UMyNavigationSystem::OnNavigationBoundsAdded(ANavMeshBoundsVolume* navVolume)
{
	Super::OnNavigationBoundsAdded(navVolume);
	TryCacheNavBoundsSubsystem();
	if (::IsValid(NavSubsystem))
	{
		NavSubsystem->RegisterNavBoundsVolume(navVolume);
	}
	else if (IsRunningGame())
	{
		UE_LOG(LogMyNavigationSystem, Error, TEXT("%s NavVolume %s in level %s is not being registered because no subsystem was found!"),
			*FString(__FUNCTION__), *GetNameSafe(navVolume), *GetNameSafe(navVolume->GetLevel()));
	}
}

void UMyNavigationSystem::OnNavigationBoundsRemoved(ANavMeshBoundsVolume* navVolume)
{
	Super::OnNavigationBoundsRemoved(navVolume);
	TryCacheNavBoundsSubsystem();
	if (::IsValid(NavSubsystem))
	{
		NavSubsystem->UnRegisterNavBoundsVolume(navVolume);
	}
	else if (IsRunningGame())
	{
		UE_LOG(LogMyNavigationSystem, Error, TEXT("%s NavVolume %s in level %s is not being un-registered because no subsystem was found!"),
			*FString(__FUNCTION__), *GetNameSafe(navVolume), *GetNameSafe(navVolume->GetLevel()));
	}
}

UMyNavBoundsVolumeManagerSubsystem* UMyNavigationSystem::TryCacheNavBoundsSubsystem()
{
	if (!::IsValid(NavSubsystem))
	{
		const UWorld* world = GetWorld();
		const UMyGameInstance* myGameInstance = ::IsValid(world) ? world->GetGameInstance<UMyGameInstance>() : nullptr;
		NavSubsystem = ::IsValid(myGameInstance) ? myGameInstance->GetSubsystem<UMyNavBoundsVolumeManagerSubsystem>() : nullptr;
	}

	return NavSubsystem;
}

const bool UMyNavigationSystem::IsRunningGame() const
{
	const UWorld* world = GetWorld();
	const bool inGame = ::IsValid(world) ? ::IsValid(world->GetGameInstance<UMyGameInstance>()) : false;
	return inGame && !GIsCookerLoadingPackage;
}