// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include "HairWorksSDK.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/HairWorksAsset.h"

UHairWorksAsset::UHairWorksAsset(const class FObjectInitializer& ObjectInitializer):
	Super(ObjectInitializer),
	AssetId(GFSDK_HairAssetID_NULL)
{
}

UHairWorksAsset::~UHairWorksAsset()
{
#if WITH_EDITOR
	for(auto& HairInstanceId : CachedInstances)
	{
		GHairWorksSDK->FreeHairInstance(HairInstanceId);
	}
#endif

	if(AssetId != GFSDK_HairAssetID_NULL)
		GHairWorksSDK->FreeHairAsset(AssetId);
}

void UHairWorksAsset::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);
}

void UHairWorksAsset::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif

	Super::PostInitProperties();
}
// @third party code - END HairWorks
