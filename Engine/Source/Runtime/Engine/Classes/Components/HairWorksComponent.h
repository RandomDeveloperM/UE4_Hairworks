// @third party code - BEGIN HairWorks
#pragma once

#include "Engine/HairWorksInstance.h"
#include "HairWorksComponent.generated.h"

enum GFSDK_HairInstanceID;

/**
* HairWorksComponent manages and renders a hair asset.
*/
UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent), HideCategories = (Collision, Base, Object, PhysicsVolume))
class ENGINE_API UHairWorksComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	~UHairWorksComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hair, meta = (ShowOnlyInnerProperties))
	FHairWorksInstance HairInstance;

	//Begin UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void OnAttachmentChanged() override;
	//End UPrimitiveComponent interface

	//Begin UActorComponent interface
	virtual void SendRenderDynamicData_Concurrent() override; 
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void OnComponentCreated();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy);
	//End UActorComponent interface

	//Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//End USceneComponent interface.

	//Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)override;
#endif
	//End UObject interface.

protected:
	/** Send data for rendering */
	void SendHairDynamicData()const;

	/** Bone mapping */
	void SetupBoneMapping();

	/** Create HairWorks instance if not created yet*/
	void PrepareHairInstance();

	/** Parent skeleton */
	UPROPERTY()
	USkinnedMeshComponent* ParentSkeleton;

	/** Bone remapping */
	TArray<uint16> BoneIndices;

	/** The hair */
	GFSDK_HairInstanceID HairInstanceId;
};
// @third party code - END HairWorks