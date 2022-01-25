// Fill out your copyright notice in the Description page of Project Settings.
#include "PhysXTest.h"
#include "PxScene.h"
#include "Physics/PhysicsFiltering.h"
#include "PxSphereGeometry.h"
#include "Physics/PhysScene_PhysX.h"
#include "DrawDebugHelpers.h"
#include "MyMacros.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "Physics/Experimental/ChaosInterfaceWrapper.h"

void APhysXTest::BeginPlay()
{
	Super::BeginPlay();
	
	// Spawn a bunch of dudes initially 
	for(int i =0; i < StartingSpawnCount; i++)
	{
		FVector SpawnLoc = SpawnOffset + FVector(FMath::RandRange(-SpawnWidth, SpawnWidth),
												 FMath::RandRange(-SpawnWidth, SpawnWidth),
												 FMath::RandRange(-SpawnWidth, SpawnWidth));
		                                         
		CreateNewPhysActor(true, SpawnLoc, SpawnRadius);
	}
}

void APhysXTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ToggleDebugPrint)
	{
		// Cache the scene initially
		FPhysScene_PhysX* newScene = GetWorld()->GetPhysicsScene();
		
		// static dudes
		int32 staticCount = newScene->GetPxScene()->getNbActors(PxActorTypeFlag::eRIGID_STATIC);
		DebugPrint("Num Static actors: " + FString::FromInt(staticCount), DeltaSeconds, FColor::Green);

		// dynamic dudes
		int32 dynamicInt = newScene->GetPxScene()->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
		DebugPrint("Num dynamic actors: " + FString::FromInt(dynamicInt), DeltaSeconds, FColor::Green);
	}

	// Debug draw lines around each collider for visualization
	if (ToggleDebugDraw)
	{
		// Cache the scene initially
		// FPhysScene_PhysX* newScene = GetWorld()->GetPhysicsScene();

		const int MaxBufferSize = 150;
		int DynBufferSize = GetWorld()->GetPhysicsScene()->GetPxScene()->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
		int FinalBufferSize = FMath::Min(DynBufferSize, MaxBufferSize);

		DebugPrint("Final buffer size: " + FString::FromInt(FinalBufferSize), DeltaSeconds, FColor::Green);
		
		// Setup a new array for our actors
		PxActor* MyArray[MaxBufferSize] = {};

		// Save all actors to array
		GetWorld()
		->GetPhysicsScene()
		->GetPxScene()
		->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, MyArray, FinalBufferSize, 0);

		// Iterate over every actor
		for(int i = 0; i < FinalBufferSize; i++)
		{
			// This Might fail? I'm only asking for eRIGID_DYNAMICs from physX, but who knows if thats going to do what it says on the tin
			PxRigidDynamic* CastedRigid = (PxRigidDynamic*)MyArray[i];

			// Grab the transform of the actor - It might actually be smarter to draw the SHAPES not the actor.
			FTransform newTransform = P2UTransform(CastedRigid->getGlobalPose());

			// Draw a sphere around the actor
			DrawDebugSphere(GetWorld(), newTransform.GetLocation(), SpawnRadius, SphereDrawSegments, FColor::Green);

			// Declare max buffer size
			const int MaxShapesBufferCount = 10;

			// Declare min buffer size
			int minShapesNum = CastedRigid->getNbShapes();

			// Declare final buffer size
			int FinalShapesBufferSize = FMath::Min(minShapesNum, MaxShapesBufferCount);

			// Allocate array for all our shapes to end up in
			PxShape* ShapesArray[MaxBufferSize] = {};
			CastedRigid->getShapes(ShapesArray, FinalShapesBufferSize,0);

			for(int j = 0; j < FinalShapesBufferSize; j++)
			{
			}
		}
	}
}

void APhysXTest::CreateNewPhysActor(bool EnableGravity, FVector position, float Radius)
{
	// FPhysicsInterface
	
	// Define the params of a new actor
	FActorCreationParams newActorParams;
	FTransform newTransform = FTransform::Identity;
	newActorParams.InitialTM = FTransform(position);
	newActorParams.bStatic = false;
	newActorParams.bStartAwake = true;
	newActorParams.bEnableGravity = EnableGravity;
	newActorParams.bSimulatePhysics = true;
	newActorParams.bQueryOnly = false;
	newActorParams.DebugName = "CPPSpawnedPhysActor"; //("CPPSpawnedPhysXActor" + FMath::Rand());
	newActorParams.Scene = GetWorld()->GetPhysicsScene(); // THE MOST IMPORTANT FUCKING MEMBER OF THIS STRUCT
	// .scene litterally does nothing - its not actually handled in the ue4 physics API.
	// you MUST later call GetPxScene()->addActor etc etc ...
	
	// Create sphere geometry
	PxSphereGeometry SphereGeometry; // Could be an issue here
	SphereGeometry.radius = Radius;

	// Create actor handle
	FPhysicsActorHandle SphereActorHandle;

	// Create the sphere shape handle
	FPhysicsShapeHandle SphereShapeHandle;
	
	// Create the "actor"
	FPhysicsInterface::CreateActor(newActorParams, SphereActorHandle);

	// Add it to the scene?
	// Actor creation params dont actually add it to a scene so we need to do it manually here i think ...
	GetWorld()->GetPhysicsScene()->GetPxScene()->addActor(*SphereActorHandle.SyncActor);
	
	// Its highly reccomended by epic to use "ExecuteWrite" rather then writing changes directly
	bool wrote = FPhysicsCommand::ExecuteWrite(SphereActorHandle, [&](FPhysicsActorHandle & Actor)
	{
		// Setting cached handle
		// CachedHandle = SphereActorHandle;

		// Must pass at least a simple material in or Physics API will fail during create shape and not add shape
		// I think this is a silent failure - just results in no shape being created, check the CreateShape() implimentation for more info
		SphereShapeHandle = FPhysicsInterface::CreateShape(&SphereGeometry, true, true, GEngine->DefaultPhysMaterial);
		
		// Attach the shape to the actor
		// Kind of implied, but attach shape cannot be called before CreateShape above.
		FPhysicsInterface::AttachShape(SphereActorHandle, SphereShapeHandle);
		
		// Setup new collision data
		// Its easiest to use the helper function from Epic CreateShapeFilterData, but we need a lot of metadata for that.
		// Create a new response container to specify which channels we block
		FCollisionResponseContainer newContainer;
		newContainer.SetAllChannels(ECR_Ignore);
		newContainer.SetResponse(ECC_Pawn, ECR_Block);
		newContainer.SetResponse(ECC_WorldStatic, ECR_Block); // Only block world static
		
		// Fill in the extra meta data the function wants
		FMaskFilter ExtraFilterData = {};
		int32 actorID = -1;
		uint32 ComponentID = 0;
		uint16 BodyIndex = 0;

		// Create the final filter data contailers
		FCollisionFilterData QueryData;
		FCollisionFilterData SimData;
		
		CreateShapeFilterData(ECollisionChannel::ECC_WorldDynamic, ExtraFilterData, actorID, newContainer, ComponentID,
		                      BodyIndex, QueryData, SimData, true, true, false, false);
		
		// Apply the filter data.
		FPhysicsInterface::SetQueryFilter(SphereShapeHandle, QueryData);
		FPhysicsInterface::SetSimulationFilter(SphereShapeHandle, SimData);
	});
}

void APhysXTest::SpawnStationary()
{
	DebugPrint("Spawning stationary called", 10.0f, FColor::Red);
	CreateNewPhysActor(false, FVector(0,0,150));
}

void APhysXTest::SpawnGravityAffected()
{
	DebugPrint("Spawning gravity affected", 1.0f, FColor::Red);
	for(int i =0; i < SpawnCount; i++)
	{
		FVector SpawnLoc = SpawnOffset + FVector(FMath::RandRange(-SpawnWidth, SpawnWidth),
		                                         FMath::RandRange(-SpawnWidth, SpawnWidth),
		                                         FMath::RandRange(-SpawnWidth, SpawnWidth));
		                                         
		CreateNewPhysActor(true, SpawnLoc, SpawnRadius);
	}
}