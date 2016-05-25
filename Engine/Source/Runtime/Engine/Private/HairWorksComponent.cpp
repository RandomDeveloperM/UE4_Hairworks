// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include "HairWorksSDK.h"
#include "Engine/HairWorksMaterial.h"
#include "Engine/HairWorksAsset.h"
#include "HairWorksSceneProxy.h"
#include "Components/HairWorksComponent.h"

UHairWorksComponent::UHairWorksComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HairInstanceId(GFSDK_HairInstanceID_NULL)
{
	// No need to select
	bSelectable = false;

	// Setup shadow
	CastShadow = true;
	bAffectDynamicIndirectLighting = false;
	bAffectDistanceFieldLighting = false;
	bCastInsetShadow = true;
	bCastStaticShadow = false;

	// Setup tick
	bAutoActivate = true;
	bTickInEditor = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;	// Just mark render data dirty every frame

	// Create hair material
	HairInstance.HairMaterial = ObjectInitializer.CreateDefaultSubobject<UHairWorksMaterial>(this, FName(*UHairWorksMaterial::StaticClass()->GetName()));
	HairInstance.HairMaterial->SetFlags(EObjectFlags::RF_Public);	// To avoid "Graph is linked to private object(s) in an external package." error in UPackage::SavePackage().
}

UHairWorksComponent::~UHairWorksComponent()
{
	check(HairInstanceId == GFSDK_HairInstanceID_NULL)
}

FPrimitiveSceneProxy* UHairWorksComponent::CreateSceneProxy()
{
	if(HairInstanceId == GFSDK_HairInstanceID_NULL || HairInstance.Hair == nullptr)
		return nullptr;

	return new FHairWorksSceneProxy(this, *HairInstance.Hair, HairInstanceId);
}

void UHairWorksComponent::OnAttachmentChanged()
{
	// Parent as skeleton
	ParentSkeleton = Cast<USkinnedMeshComponent>(AttachParent);

	// Setup bone mapping
	SetupBoneMapping();

	// Update proxy
	SendHairDynamicData();	// For correct initial visual effect
}

FBoxSphereBounds UHairWorksComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (HairInstanceId == GFSDK_HairAssetID_NULL)
		return FBoxSphereBounds(EForceInit::ForceInit);

	gfsdk_float3 HairBoundMin, HairBoundMax;
	GHairWorksSDK->GetBounds(HairInstanceId, &HairBoundMin, &HairBoundMax);

	FBoxSphereBounds Bounds(FBox(reinterpret_cast<FVector&>(HairBoundMin), reinterpret_cast<FVector&>(HairBoundMax)));

	return Bounds.TransformBy(LocalToWorld);
}

void UHairWorksComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();

	// Send data for rendering
	SendHairDynamicData();
}

void UHairWorksComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Send data every frame
	if (SceneProxy != nullptr)
	{
		MarkRenderDynamicDataDirty();
	}

#if WITH_EDITOR
	// Clear cached instances
	if(HairInstance.Hair != nullptr)
	{
		for(auto& HairInstanceId : HairInstance.Hair->CachedInstances)
		{
			GHairWorksSDK->FreeHairInstance(HairInstanceId);
		}

		HairInstance.Hair->CachedInstances.Empty();
	}
#endif
}

void UHairWorksComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	PrepareHairInstance();
}

void UHairWorksComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UHairWorksComponent::CreateRenderState_Concurrent()
{
	// Sometimes OnComponentCreated() is not called. So we call PrepareHairInstance() here
	PrepareHairInstance();

	Super::CreateRenderState_Concurrent();

	// Update proxy
	SendHairDynamicData();	// Ensure correct visual effect at first frame.
}

void UHairWorksComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();

	if(HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

#if WITH_EDITOR
	// To boost recreation in editor
	if(HairInstance.Hair != nullptr)
	{
		GFSDK_HairInstanceDescriptor HairDesc;
		GHairWorksSDK->CopyCurrentInstanceDescriptor(HairInstanceId, HairDesc);
		HairDesc.m_enable = false;
		GHairWorksSDK->UpdateInstanceDescriptor(HairInstanceId, HairDesc);

		HairInstance.Hair->CachedInstances.Push(HairInstanceId);
	}
	else
		GHairWorksSDK->FreeHairInstance(HairInstanceId);
#else
	// Delete this instance for next frame.
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		HairDisableInstance,
		GFSDK_HairInstanceID, HairInstanceId, HairInstanceId,
		{
			GHairWorksSDK->FreeHairInstance(HairInstanceId);
		}
	);
#endif

	HairInstanceId = GFSDK_HairInstanceID_NULL;
}

void UHairWorksComponent::SendHairDynamicData()const
{
	// Setup bone matrices
	if (SceneProxy == nullptr)
		return;

	TSharedRef<FHairWorksSceneProxy::FDynamicRenderData> DynamicData(new FHairWorksSceneProxy::FDynamicRenderData);

	if(ParentSkeleton != nullptr && ParentSkeleton->SkeletalMesh != nullptr)
	{
		DynamicData->BoneMatrices.Init(FMatrix::Identity, BoneIndices.Num());

		for(auto Idx = 0; Idx < BoneIndices.Num(); ++Idx)
		{
			auto& IdxInParent = BoneIndices[Idx];
			if(IdxInParent >= ParentSkeleton->GetSpaceBases().Num())
				continue;

			auto Matrix = ParentSkeleton->GetSpaceBases()[IdxInParent].ToMatrixWithScale();

			DynamicData->BoneMatrices[Idx] = ParentSkeleton->SkeletalMesh->RefBasesInvMatrix[IdxInParent] * Matrix;
		}
	}

	// Setup material
	DynamicData->Textures.SetNumZeroed(GFSDK_HAIR_NUM_TEXTURES);

	if(HairInstance.Hair->HairMaterial != nullptr)	// Always load from asset to propagate visualization flags.
	{
		HairInstance.Hair->HairMaterial->SyncHairDescriptor(DynamicData->HairInstanceDesc, DynamicData->Textures, false);
		DynamicData->NormalCenterBoneName = HairInstance.Hair->HairMaterial->HairNormalCenter;
	}

	if(HairInstance.HairMaterial != nullptr && HairInstance.bOverride)
	{
		HairInstance.HairMaterial->SyncHairDescriptor(DynamicData->HairInstanceDesc, DynamicData->Textures, false);
		DynamicData->NormalCenterBoneName = HairInstance.HairMaterial->HairNormalCenter;
	}


	// Send to proxy
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		HairUpdateDynamicData,
		FHairWorksSceneProxy&, ThisProxy, static_cast<FHairWorksSceneProxy&>(*SceneProxy),
		TSharedRef<FHairWorksSceneProxy::FDynamicRenderData>, DynamicData, DynamicData,
		{
			ThisProxy.UpdateDynamicData_RenderThread(*DynamicData);
		}
	);
}

void UHairWorksComponent::SetupBoneMapping()
{
	if(HairInstance.Hair == nullptr || ParentSkeleton == nullptr || ParentSkeleton->SkeletalMesh == nullptr)
		return;

	auto& Bones = ParentSkeleton->SkeletalMesh->RefSkeleton.GetRefBoneInfo();
	BoneIndices.SetNumUninitialized(HairInstance.Hair->BoneNames.Num());

	for(auto Idx = 0; Idx < BoneIndices.Num(); ++Idx)
	{
		BoneIndices[Idx] = Bones.IndexOfByPredicate(
			[&](const FMeshBoneInfo& BoneInfo){return BoneInfo.Name == HairInstance.Hair->BoneNames[Idx]; }
		);
	}
}

void UHairWorksComponent::PrepareHairInstance()
{
	if(HairInstanceId != GFSDK_HairInstanceID_NULL || GHairWorksSDK == nullptr || HairInstance.Hair == nullptr)
		return;

	// Initialize hair asset
	if(HairInstance.Hair->AssetId == GFSDK_HairAssetID_NULL)
	{
		// Create hair asset
		GHairWorksSDK->LoadHairAssetFromMemory(HairInstance.Hair->AssetData.GetData(), HairInstance.Hair->AssetData.Num(), &HairInstance.Hair->AssetId, nullptr, &GHairWorksConversionSettings);
	}

	// Initialize hair instance
#if WITH_EDITOR
	// To boost recreation in editor
	if(HairInstance.Hair->CachedInstances.Num() > 0)
	{
		HairInstanceId = HairInstance.Hair->CachedInstances.Pop(false);
	}
	else
	{
		GHairWorksSDK->CreateHairInstance(HairInstance.Hair->AssetId, &HairInstanceId);
	}
#else
	GHairWorksSDK->CreateHairInstance(HairInstance.Hair->AssetId, &HairInstanceId);
#endif

	// Disable this instance at first.
	GFSDK_HairInstanceDescriptor HairDesc;
	GHairWorksSDK->CopyCurrentInstanceDescriptor(HairInstanceId, HairDesc);
	HairDesc.m_enable = false;
	GHairWorksSDK->UpdateInstanceDescriptor(HairInstanceId, HairDesc);

	// Setup bone mapping
	SetupBoneMapping();
}

#if WITH_EDITOR
void UHairWorksComponent::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Initialize hair material when a hair asset is assigned
	if(HairInstance.Hair == nullptr || HairInstance.Hair->HairMaterial == nullptr)
		return;

	auto* PropertyNode = PropertyChangedEvent.PropertyChain.GetActiveMemberNode();

	if(
		PropertyNode->GetValue()->GetName() != "HairInstance"
		|| PropertyNode->GetNextNode() == nullptr
		|| PropertyNode->GetNextNode()->GetValue()->GetName() != "Hair"
		|| PropertyNode->GetNextNode()->GetNextNode() != nullptr
		)
		return;

	for(TFieldIterator<UProperty> PropIt(UHairWorksMaterial::StaticClass()); PropIt; ++PropIt)
	{
		auto* Property = *PropIt;
		Property->CopyCompleteValue_InContainer(HairInstance.HairMaterial, HairInstance.Hair->HairMaterial);
	}
}
#endif
// @third party code - END HairWorks
