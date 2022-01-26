// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MechanicalActor.h"
#include "PhysXTest.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API APhysXTest : public AMechanicalActor
{
	GENERATED_BODY()

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	FCollisionFilterData EthanCreateObjectQueryFilterData(bool bTraceComplex, int32 MultiTrace,
	                                                      const FCollisionObjectQueryParams& ObjectParam);
	void CreateNewPhysActor(bool EnableGravity, FVector position, float Radius = 10.0f);
	

	// Dead code?
	//FPhysicsActorHandle CachedHandle;
	//FPhysicsActorHandle* CachedHandlePointer;

public:
	UFUNCTION(BlueprintCallable)
	void SpawnStationary();

	UFUNCTION(BlueprintCallable)
	void SpawnGravityAffected();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SpawnWidth = 10.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SpawnRadius = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool shouldRayTrace = true;

	//shouldRayTrace


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FCollisionResponseContainer testContainer;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int KeypressSpawnCount = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int StartingSpawnCount = 5000;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool ToggleDebugDraw = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool ToggleDebugPrint = true;

	


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int SphereDrawSegments = 16;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector SpawnOffset;

	
};
