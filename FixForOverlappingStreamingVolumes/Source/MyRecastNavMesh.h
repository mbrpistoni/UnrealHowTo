#pragma once
#include "Navmesh/RecastNavMesh.h"
#include "MyRecastNavMesh.generated.h"

/** Custom version of RecastNavMesh so we can have a say in how nav is handled*/
UCLASS()
class MY_API AMyRecastNavMesh : public ARecastNavMesh
{
	GENERATED_BODY()

public:
	void DetachNavMeshDataChunk(URecastNavMeshDataChunk& navDataChunk) override;
	void OnNavMeshGenerationFinished() override;
};