// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "EnginePrivate.h"
#include "PhysDerivedData.h"
#include "TargetPlatform.h"

#if WITH_PHYSX && (WITH_RUNTIME_PHYSICS_COOKING || WITH_EDITOR)

#include "IPhysXFormatModule.h"

FDerivedDataPhysXCooker::FDerivedDataPhysXCooker( FName InFormat, UBodySetup* InBodySetup )
	: BodySetup( InBodySetup )
	, CollisionDataProvider( NULL )
	, Format( InFormat )
	, Cooker( NULL )
{
	check( BodySetup != NULL );
	CollisionDataProvider = BodySetup->GetOuter();
	DataGuid = BodySetup->BodySetupGuid;
	bGenerateNormalMesh = BodySetup->bGenerateNonMirroredCollision;
	bGenerateMirroredMesh = BodySetup->bGenerateMirroredCollision;
	IInterface_CollisionDataProvider* CDP = Cast<IInterface_CollisionDataProvider>(CollisionDataProvider);
	if (CDP)
	{
		CDP->GetMeshId(MeshId);
	}
	InitCooker();
}

void FDerivedDataPhysXCooker::InitCooker()
{
#if WITH_EDITOR
	// static here as an optimization
	static ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		Cooker = TPM->FindPhysXFormat( Format );
	}
#elif WITH_RUNTIME_PHYSICS_COOKING
	if (IPhysXFormatModule* Module = FModuleManager::LoadModulePtr<IPhysXFormatModule>("PhysXFormats"))
	{
		Cooker = Module->GetPhysXFormat();
	}
#endif
}

DECLARE_CYCLE_STAT(TEXT("PhysX Cooking"), STAT_PhysXCooking, STATGROUP_Physics);


bool FDerivedDataPhysXCooker::Build( TArray<uint8>& OutData )
{
	SCOPE_CYCLE_COUNTER(STAT_PhysXCooking);

	check(Cooker != NULL);

	FMemoryWriter Ar( OutData );
	uint8 bLittleEndian = PLATFORM_LITTLE_ENDIAN;	//TODO: We should pass the target platform into this function and write it. Then swap the endian on the writer so the reader doesn't have to do it at runtime
	int32 NumConvexElementsCooked = 0;
	int32 NumMirroredElementsCooked = 0;
	int32 NumTriMeshesCooked = 0;
	Ar << bLittleEndian;
	int64 CookedMeshInfoOffset = Ar.Tell();
	Ar << NumConvexElementsCooked;	
	Ar << NumMirroredElementsCooked;
	Ar << NumTriMeshesCooked;

	//TODO: we must save an id with convex and tri meshes for serialization. We must save this here and patch it up at runtime somehow

	// Cook convex meshes, but only if we are not forcing complex collision to be used as simple collision as well
	if( BodySetup && BodySetup->GetCollisionTraceFlag() != CTF_UseComplexAsSimple && BodySetup->AggGeom.ConvexElems.Num() > 0 )
	{
		if( bGenerateNormalMesh )
		{
			NumConvexElementsCooked = BuildConvex( OutData, false );
		}
		if ( bGenerateMirroredMesh )
		{
			NumMirroredElementsCooked = BuildConvex( OutData, true );
		}
	}

	// Cook trimeshes, but only if we do not frce simple collision to be used as complex collision as well
	bool bUsingAllTriData = BodySetup != NULL ? BodySetup->bMeshCollideAll : false;
	if( (BodySetup == NULL || BodySetup->GetCollisionTraceFlag() != CTF_UseSimpleAsComplex) && ShouldGenerateTriMeshData(bUsingAllTriData) )
	{
		NumTriMeshesCooked = BuildTriMesh( OutData, bUsingAllTriData );
	}

	// Update info on what actually got cooked
	Ar.Seek( CookedMeshInfoOffset );
	Ar << NumConvexElementsCooked;	
	Ar << NumMirroredElementsCooked;
	Ar << NumTriMeshesCooked;

	// Whatever got cached return true. We want to cache 'failure' too.
	return true;
}

int32 FDerivedDataPhysXCooker::BuildConvex( TArray<uint8>& OutData, bool InMirrored )
{	
	check( BodySetup != NULL );
	for( int32 ElementIndex = 0; ElementIndex < BodySetup->AggGeom.ConvexElems.Num(); ElementIndex++ )
	{
		const TArray<FVector>* MeshVertices = NULL;
		TArray<FVector> MirroredVerts;
		const FKConvexElem& ConvexElem = BodySetup->AggGeom.ConvexElems[ElementIndex];

		if( InMirrored )
		{
			MirroredVerts.AddUninitialized(ConvexElem.VertexData.Num());
			for(int32 VertIdx=0; VertIdx<ConvexElem.VertexData.Num(); VertIdx++)
			{
				MirroredVerts[VertIdx] = ConvexElem.VertexData[VertIdx] * FVector(-1,1,1);
			}
			MeshVertices = &MirroredVerts;
		}
		else
		{
			MeshVertices = &ConvexElem.VertexData;
		}

		// Store info on the cooking result (1 byte)
		int32 ResultInfoOffset = OutData.Add( false );

		// Cook and store Result at ResultInfoOffset
		UE_LOG(LogPhysics, Log, TEXT("Cook Convex: %s %d (FlipX:%d)"), *BodySetup->GetOuter()->GetPathName(), ElementIndex, InMirrored);		
		const bool bDeformableMesh = CollisionDataProvider->IsA(USplineMeshComponent::StaticClass());
		const bool Result = Cooker->CookConvex(Format, *MeshVertices, OutData, bDeformableMesh);
		if( !Result )
		{
			UE_LOG(LogPhysics, Warning, TEXT("Failed to cook convex: %s %d (FlipX:%d). The remaining elements will not get cooked."), *BodySetup->GetOuter()->GetPathName(), ElementIndex, InMirrored);
		}
		OutData[ ResultInfoOffset ] = Result;
	}

	return BodySetup->AggGeom.ConvexElems.Num();
}

bool FDerivedDataPhysXCooker::ShouldGenerateTriMeshData(bool InUseAllTriData)
{
	check(Cooker != NULL);

	IInterface_CollisionDataProvider* CDP = Cast<IInterface_CollisionDataProvider>(CollisionDataProvider);
	const bool bPerformCook = ( CDP != NULL ) ? CDP->ContainsPhysicsTriMeshData(InUseAllTriData) : false;
	return bPerformCook;
}

int32 FDerivedDataPhysXCooker::BuildTriMesh( TArray<uint8>& OutData, bool InUseAllTriData )
{
	check(Cooker != NULL);

	bool bResult = false;
	FTriMeshCollisionData TriangleMeshDesc;
	IInterface_CollisionDataProvider* CDP = Cast<IInterface_CollisionDataProvider>(CollisionDataProvider);
	check(CDP != NULL); // It's all been checked before getting into this function
		
	bool bHaveTriMeshData = CDP->GetPhysicsTriMeshData(&TriangleMeshDesc, InUseAllTriData);
	if(bHaveTriMeshData)
	{
		// If any of the below checks gets hit this usually means 
		// IInterface_CollisionDataProvider::ContainsPhysicsTriMeshData did not work properly.
		const int32 NumIndices = TriangleMeshDesc.Indices.Num();
		const int32 NumVerts = TriangleMeshDesc.Vertices.Num();
		if(NumIndices == 0 || NumVerts == 0 || TriangleMeshDesc.MaterialIndices.Num() > NumIndices)
		{
			UE_LOG(LogPhysics, Warning, TEXT("FDerivedDataPhysXCooker::BuildTriMesh: Triangle data from '%s' invalid (%d verts, %d indices)."), *CollisionDataProvider->GetPathName(), NumVerts, NumIndices );
			return bResult;
		}

		TArray<FVector>* MeshVertices = NULL;
		MeshVertices = &TriangleMeshDesc.Vertices;
		
		UE_LOG(LogPhysics, Log, TEXT("Cook TriMesh: %s"), *CollisionDataProvider->GetPathName());
		bool bDeformableMesh = CollisionDataProvider->IsA(USplineMeshComponent::StaticClass());
		if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(CollisionDataProvider))
		{
			ensure(SkeletalMesh->bEnablePerPolyCollision);
			bDeformableMesh = true;
		}
		bResult = Cooker->CookTriMesh( Format, *MeshVertices, TriangleMeshDesc.Indices, TriangleMeshDesc.MaterialIndices, TriangleMeshDesc.bFlipNormals, OutData, bDeformableMesh );
		if( !bResult )
		{
			UE_LOG(LogPhysics, Warning, TEXT("Failed to cook TriMesh: %s."), *CollisionDataProvider->GetPathName());
		}
	}

	return bResult == true ? 1 : 0;	//the cooker only generates 1 or 0 trimeshes. We return an int because we support multiple trimeshes for welding and we might want to do this per static mesh in the future.
}

FDerivedDataPhysXBinarySerializer::FDerivedDataPhysXBinarySerializer(FName InFormat, const TArray<FBodyInstance*>& InBodies, const TArray<class UBodySetup*>& InBodySetups, const TArray<class UPhysicalMaterial*>& InPhysicalMaterials, const FGuid& InGuid)
: Bodies(InBodies)
, BodySetups(InBodySetups)
, PhysicalMaterials(InPhysicalMaterials)
, Format(InFormat)
, DataGuid(InGuid)
{
	InitSerializer();
}

bool FDerivedDataPhysXBinarySerializer::Build(TArray<uint8>& OutData)
{
	FMemoryWriter Ar(OutData);
	
	//do not serialize anything before the physx data. This is important because physx requires specific alignment. For that to work the physx data must come first in the archive
	SerializeRigidActors(OutData);
	
	// Whatever got cached return true. We want to cache 'failure' too.
	return true;
}

void FDerivedDataPhysXBinarySerializer::SerializeRigidActors(TArray<uint8>& OutData)
{
	Serializer->SerializeActors(Format, Bodies, BodySetups, PhysicalMaterials, OutData);
}

void FDerivedDataPhysXBinarySerializer::InitSerializer()
{
#if WITH_EDITOR
	// static here as an optimization
	static ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		Serializer = TPM->FindPhysXFormat(Format);
	}
#endif
}


#endif	//WITH_PHYSX && WITH_EDITOR
