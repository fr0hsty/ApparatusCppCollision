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
#include "Physics/PhysicsInterfaceUtils.h"
//#include "Physics/PhysicsInterfaceUtils.cpp"

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


void APhysXTest::CreateNewPhysActor(bool EnableGravity, FVector position, float Radius)
{
	// FPhysicsInterface
	
	// Define the params of a new actor
	FActorCreationParams newActorParams;
	//FTransform newTransform = FTransform::Identity;
	newActorParams.InitialTM = FTransform(position);
	newActorParams.bStatic = false;
	newActorParams.bStartAwake = true;
	newActorParams.bEnableGravity = EnableGravity;
	newActorParams.bSimulatePhysics = true; // Doesnt do anything?
	newActorParams.bQueryOnly = false;
	newActorParams.DebugName = "CPPSpawnedPhysActor"; //("CPPSpawnedPhysXActor" + FMath::Rand());
	newActorParams.Scene = GetWorld()->GetPhysicsScene(); // THE MOST IMPORTANT FUCKING MEMBER OF THIS STRUCT
	// .scene litterally does nothing - its not actually handled in the ue4 physics API.
	// you MUST later call GetPxScene()->addActor etc etc ...
	
	// Create sphere geometry
	PxSphereGeometry SphereGeometry; // Could be an issue here, Should probably be replaced with FPhysicsGemoetry equivilent.
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
		// newContainer.SetResponse(ECC_WorldDynamic, ECR_Block);
		//newContainer.SetResponse(ECC_WorldStatic, ECR_Block);
		newContainer.SetAllChannels(ECR_Overlap);
		newContainer.SetResponse(ECC_Visibility, ECR_Block);
		newContainer.SetResponse(ECC_WorldStatic, ECR_Block);
		newContainer.SetAllChannels(ECR_Block);
		// newContainer.
		
		
		
		//newContainer.SetResponse(ECC_Pawn, ECR_Block);
		//newContainer.SetResponse(ECC_WorldStatic, ECR_Block); // Only block world static
		//newContainer.SetAllChannels(ECR_Block);
		//newContainer.SetAllChannels(ECR_Overlap);
		
		// Fill in the extra meta data the function wants
		FMaskFilter ExtraFilterData = {}; // I have no idea if this is OK to be empty initialized or not.

		// WARNING - I think this line here can crash the engine later when other ue4 actors... 
		// ... end up sharing a name with this collider. It might be worth it to FMath::Rand just to be sure its random.
		// int32 actorID = -1;
		//int32 actorID = FMath::Rand();
		int32 actorID = 1;
		
		uint32 ComponentID = 0;
		uint16 BodyIndex = 0; // I beleive these are ok to be "zero" beacuse only one shape is being added. // These correspond to hit indexes/SK indexes depending on how its implimented ...

		// Create the final filter data contailers
		FCollisionFilterData SimData={};

		// Collision acting funky, trying to experiment to sortt this out
		FCollisionFilterData QueryData={}; // I think the key thing with Querydata

		/*DebugPrint("Query Word0: " + FString::FromInt(QueryData.Word0), 10.0f, FColor::Red);
		DebugPrint("Query Word1: " + FString::FromInt(QueryData.Word1), 10.0f, FColor::Red);
		DebugPrint("Query Word2: " + FString::FromInt(QueryData.Word2), 10.0f, FColor::Red);
		DebugPrint("Query Word3: " + FString::FromInt(QueryData.Word3), 10.0f, FColor::Red);*/


			
		// AS OF CURRENT - THE QUERY DATA IS NOT CORRECT AND CANNOT BE TRACED AGAINST - WORKING TO FIX THAT

		// I dont think this fills out query data correctly.
		CreateShapeFilterData(ECollisionChannel::ECC_WorldDynamic, ExtraFilterData, actorID, newContainer, ComponentID,
		                      BodyIndex, QueryData, SimData, false, true, false, false);

		/*DebugPrint("Query Word0: " + FString::FromInt(QueryData.Word0), 10.0f, FColor::Red);
		DebugPrint("Query Word1: " + FString::FromInt(QueryData.Word1), 10.0f, FColor::Red);
		DebugPrint("Query Word2: " + FString::FromInt(QueryData.Word2), 10.0f, FColor::Red);
		DebugPrint("Query Word3: " + FString::FromInt(QueryData.Word3), 10.0f, FColor::Red);*/


		// This is doing the above thing manually in a second pass just for the QueryData
		FPhysicsFilterBuilder Builder(ECC_WorldDynamic, ExtraFilterData, newContainer);
		Builder.ConditionalSetFlags(EPDF_ComplexCollision, false);
		// Builder.ConditionalSetFlags(EPDF_KinematicKinematicPairs, false);
		Builder.ConditionalSetFlags(EPDF_CCD, true);
		Builder.ConditionalSetFlags(EPDF_ContactNotify, true);
		Builder.ConditionalSetFlags(EPDF_StaticShape, false);
		Builder.ConditionalSetFlags(EPDF_ModifyContacts, true);

		Builder.GetQueryData(actorID, QueryData.Word0, QueryData.Word1, QueryData.Word2, QueryData.Word3);


		/*DebugPrint("Query Word0: " + FString::FromInt(QueryData.Word0), 10.0f, FColor::Red);
		DebugPrint("Query Word1: " + FString::FromInt(QueryData.Word1), 10.0f, FColor::Red);
		DebugPrint("Query Word2: " + FString::FromInt(QueryData.Word2), 10.0f, FColor::Red);
		DebugPrint("Query Word3: " + FString::FromInt(QueryData.Word3), 10.0f, FColor::Red);*/
		
		
		/*
		 *CreateShapeFilterData(ECollisionChannel::ECC_WorldDynamic, ExtraFilterData, actorID, newContainer, ComponentID,
									  BodyIndex, QueryData, SimData, true, true, true, true);
									  */


		/*CreateShapeFilterData(ECollisionChannel::ECC_WorldDynamic, ExtraFilterData, actorID, newContainer, ComponentID,
							  BodyIndex, QueryData, SimData, true, true, false, false);*/

		// PhysicsInterfaceUtils.h contains CreateQueryFilterData() <- could be useful aswell.
		FCollisionQueryParams collisionQueryParams;
		
		FCollisionObjectQueryParams objectqueryparams = {}; // if these are valid down below, the function changes.
		objectqueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
		objectqueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_Visibility);
		objectqueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_Camera);
		//objectqueryparams.DefaultObjectQueryParam();
		
		if (objectqueryparams.IsValid())
		{
			DebugPrint("Object query is valid!!!", 10.0f, FColor::Red);
		}
		else
		{
			DebugPrint("Object query params are not valid!!!", 10.0f, FColor::Red);
		}

		
		//FCollisionFilterData
		//FCollisionFilterData CreateObjectQueryFilterData();
		
		
		// If The passed in ObjectQueryParams .IsVAlid() is true - it behaves differently. 
		// QueryData = CreateQueryFilterData(ECollisionChannel::ECC_WorldDynamic, false, newContainer, objectparams, objectqueryparams, false);
		// QueryData = CreateQueryFilterData(ECollisionChannel::ECC_WorldDynamic, false, newContainer, objectparams, objectqueryparams, false);
		
		

		// Apply the filter data.
		//FPhysicsInterface::SetQueryFilter(SphereShapeHandle, QueryData);

		FPhysicsInterface::SetIsQueryShape(SphereShapeHandle, true);
		FCollisionFilterData filterData ={};

		
		//CreateQueryFilterData();
		
		//filterData = CreateQueryFilterData(ECollisionChannel::ECC_WorldDynamic, false, newContainer, collisionQueryParams, objectqueryparams, false);
		//

		//filterData = EthanCreateObjectQueryFilterData(ECollisionChannel::ECC_WorldDynamic, false, newContainer, collisionQueryParams, objectqueryparams, false);
		filterData = EthanCreateObjectQueryFilterData(false, false, objectqueryparams);
		
		//EthanCreateObjectQueryFilterData
		//CreateQueryFilterData(ECollisionChannel::ECC_WorldDynamic, false, newContainer, collisionQueryParams, objectqueryparams, false);

		
		if (SphereShapeHandle.IsValid())
		{
			DebugPrint("Shape is valid, setting query filter!!!", 10.0f, FColor::Red);
			FPhysicsInterface::SetQueryFilter(SphereShapeHandle, filterData);
		}

		if(SphereShapeHandle.IsValid())
		{
			DebugPrint("THE SHAPE IS VALID!!!", 10.0f, FColor::Red);
		}
		else
		{
			DebugPrint("THE SHAPE IS NOOOOOT VALID!!!", 10.0f, FColor::Red);
		}

		// I think between here and the tick function - somehow - the query filter isn't being set properly
		
		DebugPrint("Name: " + FString(SphereActorHandle.SyncActor->getName()), 20.0f, FColor::Yellow);
		DebugPrint("Query before being applied: ", 20.0f, FColor::Yellow);
		DebugPrint("Query Word0: " + FString::FromInt(QueryData.Word0), 20.0f, FColor::Yellow);
		DebugPrint("Query Word1: " + FString::FromInt(QueryData.Word1), 20.0f, FColor::Yellow);
		DebugPrint("Query Word2: " + FString::FromInt(QueryData.Word2), 20.0f, FColor::Yellow);
		DebugPrint("Query Word3: " + FString::FromInt(QueryData.Word3), 20.0f, FColor::Yellow);
		
		//FPhysicsInterface::SetQueryFilter(SphereShapeHandle, QueryData);
		FPhysicsInterface::SetSimulationFilter(SphereShapeHandle, SimData);

		PxFilterData postApplyFilterData = SphereShapeHandle.Shape->getQueryFilterData();

		DebugPrint("Query AFTER being applied: ", 20.0f, FColor::Yellow);
		DebugPrint("Query Word0: " + FString::FromInt(postApplyFilterData.word0), 20.0f, FColor::Yellow);
		DebugPrint("Query Word1: " + FString::FromInt(postApplyFilterData.word1), 20.0f, FColor::Yellow);
		DebugPrint("Query Word2: " + FString::FromInt(postApplyFilterData.word2), 20.0f, FColor::Yellow);
		DebugPrint("Query Word3: " + FString::FromInt(postApplyFilterData.word3), 20.0f, FColor::Yellow);

		FCollisionFilterData getData = FPhysicsInterface::GetQueryFilter(SphereShapeHandle);
		
		DebugPrint("End applied: ", 20.0f, FColor::Yellow);
		DebugPrint("Last Word0: " + FString::FromInt(getData.Word0), 20.0f, FColor::Yellow);
		DebugPrint("Last Word1: " + FString::FromInt(getData.Word1), 20.0f, FColor::Yellow);
		DebugPrint("Last Word2: " + FString::FromInt(getData.Word2), 20.0f, FColor::Yellow);
		DebugPrint("Last Word3: " + FString::FromInt(getData.Word3), 20.0f, FColor::Yellow);

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

void APhysXTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Quickly perform a trace check to see if we can query against our colliders
	{
		// FGenericPhysicsInterface::RaycastTest()
		
		// Out hit 
		FHitResult outHit;

		// The types of params
		FCollisionQueryParams QueryParams;
		const FName TraceTag("MyTraceTag");
		GetWorld()->DebugDrawTraceTag = TraceTag;
		QueryParams.TraceTag = TraceTag;
		QueryParams.bTraceComplex = false;
		QueryParams.IgnoreMask = {};
		QueryParams.bIgnoreBlocks = false;
		QueryParams.bIgnoreTouches = false;
		
		
		// Block everything?
		FCollisionResponseParams ResponseParams;
		ResponseParams.CollisionResponse.SetAllChannels(ECR_Block);
		
		
		FCollisionObjectQueryParams ObjectQueryparams;
		ObjectQueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_Visibility);
		ObjectQueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
		ObjectQueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
		ObjectQueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);
		ObjectQueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_Camera);
		//ObjectQueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_Visibility);
		//ObjectQueryparams.AddObjectTypesToQuery(ECollisionChannel::ECC_Visibility);
		
		
		//ObjectQueryparams.
		
		
		// Perform the trace
		bool hit = FGenericPhysicsInterface::RaycastSingle(
			GetWorld(),
			outHit,
			GetActorLocation(),
			(GetActorForwardVector() * 3000) + GetActorLocation(),
			ECollisionChannel::ECC_WorldDynamic,
			QueryParams,
			ResponseParams,
			ObjectQueryparams);

		// Successfully hit something
		if (hit)
		{
			//outHit.GetActor()
			DebugPrint("WE HIT SOMETHIGN!!!!!!", 10.0f, FColor::Red);
		}
		
	}

	
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

			
			// DebugPrint("Shapes Array Query Word0: " + CastedRigid->getName(), 10.0f, FColor::Red);
			//const char* name = CastedRigid->getName();
			FString myString (CastedRigid->getName());

			
			DebugPrint("Shapes array actor Name: " + myString, DeltaSeconds, FColor::Red);
			for(int j = 0; j < FinalShapesBufferSize; j++)
			{
				if (ShapesArray[j] != nullptr)
				{
					PxFilterData filterData = ShapesArray[j]->getQueryFilterData();
					// filterData.word0
					DebugPrint("Shapes Array Query Word0: " + FString::FromInt(filterData.word0), DeltaSeconds, FColor::Red);
					DebugPrint("Shapes Array Query Word1: " + FString::FromInt(filterData.word1), DeltaSeconds, FColor::Red);
					DebugPrint("Shapes Array Query Word2: " + FString::FromInt(filterData.word2), DeltaSeconds, FColor::Red);
					DebugPrint("Shapes Array Query Word3: " + FString::FromInt(filterData.word3), DeltaSeconds, FColor::Red);
					DebugPrint("Shape is not null", DeltaSeconds, FColor::Green);

					if (myString == "CPPSpawnedPhysActor")
					{
						DebugPrint("Strings are equal", DeltaSeconds, FColor::Green);

						/*
						filterData.word0 = 1;
						filterData.word1 = -65281;
						filterData.word2 = 65280;
						filterData.word3 = 2097196;*/

						filterData.word0 = 99;
						filterData.word1 = 99;
						filterData.word2 = 99;
						filterData.word3 = 223236;
						
						// ShapesArray[j]->setQueryFilterData(filterData);
					}
					
				}
				else
				{
					DebugPrint("SHAPE is null", 0.5f, FColor::Green);
				}
			}
		}
	}
}

// Copy pasted this out of engines PhysicsInterfaceUtils.h - code base wouldn't compile without it
// Original name is CreateObjectQueryFilterData
// HACK - This should probably be put somewhere better then this OR someone shoudl figure out how to get CreateObjectQueryFilterData to compile
FCollisionFilterData APhysXTest::EthanCreateObjectQueryFilterData(const bool bTraceComplex, const int32 MultiTrace/*=1 if multi. 0 otherwise*/, const struct FCollisionObjectQueryParams & ObjectParam)
{
	/**
	* Format for QueryData :
	*		word0 (meta data - ECollisionQuery. Extendable)
	*
	*		For object queries
	*
	*		word1 (object type queries)
	*		word2 (unused)
	*		word3 (Multi (1) or single (0) (top 8) + Flags (lower 24))
	*/

	FCollisionFilterData NewData;

	NewData.Word0 = (uint32)ECollisionQuery::ObjectQuery;

	if (bTraceComplex)
	{
		NewData.Word3 |= EPDF_ComplexCollision;
	}
	else
	{
		NewData.Word3 |= EPDF_SimpleCollision;
	}

	// get object param bits
	NewData.Word1 = ObjectParam.GetQueryBitfield();

	// if 'nothing', then set no bits
	NewData.Word3 |= CreateChannelAndFilter((ECollisionChannel)MultiTrace, ObjectParam.IgnoreMask);

	return NewData;
}

