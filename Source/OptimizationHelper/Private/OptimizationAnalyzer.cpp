#include "OptimizationAnalyzer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Engine/Blueprint.h"
#include "Sound/SoundWave.h"
#include "Particles/ParticleSystem.h"

TArray<FOptimizationIssue> UOptimizationAnalyzer::AnalyzeProject()
{
    TArray<FOptimizationIssue> AllIssues;
    
    AllIssues.Append(CheckMeshes());
    AllIssues.Append(CheckTextures());
    AllIssues.Append(CheckMaterials());
    AllIssues.Append(CheckBlueprints());
    AllIssues.Append(CheckAudio());
    AllIssues.Append(CheckParticleSystems());
    
    return AllIssues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::AnalyzeCurrentLevel()
{
    TArray<FOptimizationIssue> Issues;
    // Implement later
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckMeshes()
{
    TArray<FOptimizationIssue> Issues;
    
    FAssetRegistryModule& AssetRegistryModule = 
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    
    TArray<FAssetData> MeshAssets;
    AssetRegistryModule.Get().GetAssetsByClass(
        UStaticMesh::StaticClass()->GetClassPathName(), 
        MeshAssets
    );
    
    for (const FAssetData& AssetData : MeshAssets)
    {
        UStaticMesh* Mesh = Cast<UStaticMesh>(AssetData.GetAsset());
        if (!Mesh) continue;
        
        // Check triangle count
        int32 TriangleCount = 0;
        if (Mesh->GetRenderData())
        {
            const FStaticMeshLODResources& LOD = Mesh->GetRenderData()->LODResources[0];
            TriangleCount = LOD.GetNumTriangles();
        }
        
        if (TriangleCount > MaxTrianglesPerMesh)
        {
            FOptimizationIssue Issue;
            Issue.Title = FString::Printf(TEXT("High Poly Count: %s"), *Mesh->GetName());
            Issue.Description = FString::Printf(
                TEXT("Mesh has %d triangles (threshold: %d)"),
                TriangleCount,
                MaxTrianglesPerMesh
            );
            Issue.Severity = EOptimizationSeverity::Warning;
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.EstimatedImpact = 60.0f;
            Issue.SuggestedFix = TEXT("Reduce polygon count or create LODs");
            Issues.Add(Issue);
        }
        
        // Check for missing LODs
        if (Mesh->GetNumLODs() <= 1 && TriangleCount > 10000)
        {
            FOptimizationIssue Issue;
            Issue.Title = FString::Printf(TEXT("Missing LODs: %s"), *Mesh->GetName());
            Issue.Description = TEXT("High-poly mesh has no LOD chain");
            Issue.Severity = EOptimizationSeverity::Warning;
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.EstimatedImpact = 50.0f;
            Issue.SuggestedFix = TEXT("Generate LOD chain");
            Issues.Add(Issue);
        }
    }
    
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckTextures()
{
    TArray<FOptimizationIssue> Issues;
    
    FAssetRegistryModule& AssetRegistryModule = 
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    
    TArray<FAssetData> TextureAssets;
    AssetRegistryModule.Get().GetAssetsByClass(
        UTexture2D::StaticClass()->GetClassPathName(), 
        TextureAssets
    );
    
    for (const FAssetData& AssetData : TextureAssets)
    {
        UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset());
        if (!Texture) continue;
        
        int32 MaxDimension = FMath::Max(Texture->GetSizeX(), Texture->GetSizeY());
        
        if (MaxDimension > MaxTextureSize)
        {
            FOptimizationIssue Issue;
            Issue.Title = FString::Printf(TEXT("Large Texture: %s"), *Texture->GetName());
            Issue.Description = FString::Printf(
                TEXT("Texture size: %dx%d (threshold: %d)"),
                Texture->GetSizeX(),
                Texture->GetSizeY(),
                MaxTextureSize
            );
            Issue.Severity = EOptimizationSeverity::Warning;
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.EstimatedImpact = 45.0f;
            Issue.SuggestedFix = TEXT("Resize texture or enable virtual texturing");
            Issues.Add(Issue);
        }
    }
    
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckMaterials()
{
    TArray<FOptimizationIssue> Issues;
    // Simplified for now
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckBlueprints()
{
    TArray<FOptimizationIssue> Issues;
    // Simplified for now
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckAudio()
{
    TArray<FOptimizationIssue> Issues;
    // Simplified for now
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckParticleSystems()
{
    TArray<FOptimizationIssue> Issues;
    // Simplified for now
    return Issues;
}