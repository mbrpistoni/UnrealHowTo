#pragma once

#include "NavigationSystem/Public/NavigationSystem.h"
#include "MyNavigationSystem.generated.h"

class ANavMeshBoundsVolume;
class UMyNavBoundsVolumeManagerSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogMyNavigationSystem, Log, All);

UCLASS(config = Engine, defaultconfig)
class MY_API UMyNavigationSystem : public UNavigationSystemV1
{
	GENERATED_BODY()

public:
	void OnNavigationBoundsAdded(ANavMeshBoundsVolume* navVolume) override;
	void OnNavigationBoundsRemoved(ANavMeshBoundsVolume* navVolume) override;

private:
	/** Try to set the local member NavSubsystem*/
	UMyNavBoundsVolumeManagerSubsystem* TryCacheNavBoundsSubsystem();
	/** Custom function to check if we are running the game (not working in editor or cooking)*/
	const bool IsRunningGame() const;

private:
	// NavSubsystem is where the nav volumes are registered
	UPROPERTY(Transient)
		UMyNavBoundsVolumeManagerSubsystem* NavSubsystem;
};