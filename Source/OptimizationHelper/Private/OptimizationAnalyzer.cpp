#include "OptimizationAnalyzer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Engine/Blueprint.h"
#include "Sound/SoundWave.h"
#include "Particles/ParticleSystem.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"


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

    // Get current world
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("No level is currently opened"));
        return Issues;
    }

    UE_LOG(LogTemp, Log, TEXT("Analyzing current level: %s"), *World->GetName());

    // Track processed assets to avoid duplicates
    TSet<UStaticMesh*> ProcessedMeshes;
    TSet<UTexture*> ProcessedTextures;

    int32 ActorCount = 0;
    int32 MeshCount = 0;
    int32 TextureCount = 0;

    // Iterate through all actors in the level
    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        ActorCount++;

        // Get all static mesh components
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);

        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp && MeshComp->GetStaticMesh())
            {
                UStaticMesh* Mesh = MeshComp->GetStaticMesh();

                // Avoid analyzing same mesh multiple times
                if (!ProcessedMeshes.Contains(Mesh))
                {
                    ProcessedMeshes.Add(Mesh);
                    MeshCount++;

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
                        Issue.Category = EOptimizationCategory::Mesh;
                        Issue.Title = FString::Printf(TEXT("High Poly Count: %s"), *Mesh->GetName());

                        if (TriangleCount > MaxTrianglesPerMesh * 3)
                        {
                            Issue.Severity = EOptimizationSeverity::Critical;
                            Issue.EstimatedImpact = 90.0f;
                        }
                        else
                        {
                            Issue.Severity = EOptimizationSeverity::Warning;
                            Issue.EstimatedImpact = 60.0f;
                        }

                        Issue.Description = FString::Printf(
                            TEXT("Mesh has %d triangles (threshold: %d). Used in level '%s'"),
                            TriangleCount,
                            MaxTrianglesPerMesh,
                            *World->GetName()
                        );
                        Issue.AssetPath = Mesh->GetPathName();
                        Issue.SuggestedFix = TEXT("Reduce polygon count or create LODs");
                        Issues.Add(Issue);
                    }

                    // Check for missing LODs
                    if (Mesh->GetNumLODs() <= 1 && TriangleCount > 10000)
                    {
                        FOptimizationIssue Issue;
                        Issue.Category = EOptimizationCategory::Mesh;
                        Issue.Title = FString::Printf(TEXT("Missing LODs: %s"), *Mesh->GetName());
                        Issue.Description = TEXT("High-poly mesh has no LOD chain in current level");
                        Issue.Severity = EOptimizationSeverity::Warning;
                        Issue.AssetPath = Mesh->GetPathName();
                        Issue.EstimatedImpact = 50.0f;
                        Issue.SuggestedFix = TEXT("Generate LOD chain");
                        Issues.Add(Issue);
                    }
                }

                // Analyze materials and textures
                TArray<UMaterialInterface*> Materials = MeshComp->GetMaterials();
                for (UMaterialInterface* Material : Materials)
                {
                    if (Material)
                    {
                        TArray<UTexture*> Textures;
                        Material->GetUsedTextures(Textures, EMaterialQualityLevel::High, true, ERHIFeatureLevel::SM5, true);

                        for (UTexture* Texture : Textures)
                        {
                            UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
                            if (Texture2D && !ProcessedTextures.Contains(Texture2D))
                            {
                                ProcessedTextures.Add(Texture2D);
                                TextureCount++;

                                int32 MaxDimension = FMath::Max(Texture2D->GetSizeX(), Texture2D->GetSizeY());

                                if (MaxDimension > MaxTextureSize)
                                {
                                    FOptimizationIssue Issue;
                                    Issue.Category = EOptimizationCategory::Texture;
                                    Issue.Title = FString::Printf(TEXT("Large Texture: %s"), *Texture2D->GetName());

                                    if (MaxDimension > 8192)
                                    {
                                        Issue.Severity = EOptimizationSeverity::Critical;
                                        Issue.EstimatedImpact = 85.0f;
                                    }
                                    else
                                    {
                                        Issue.Severity = EOptimizationSeverity::Warning;
                                        Issue.EstimatedImpact = 55.0f;
                                    }

                                    Issue.Description = FString::Printf(
                                        TEXT("Texture size: %dx%d (threshold: %d). Used in level '%s'"),
                                        Texture2D->GetSizeX(),
                                        Texture2D->GetSizeY(),
                                        MaxTextureSize,
                                        *World->GetName()
                                    );
                                    Issue.AssetPath = Texture2D->GetPathName();
                                    Issue.SuggestedFix = TEXT("Resize texture or enable virtual texturing");
                                    Issues.Add(Issue);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Level analysis complete: %d actors, %d unique meshes, %d unique textures, %d issues found"),
        ActorCount, MeshCount, TextureCount, Issues.Num());

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
            Issue.Category = EOptimizationCategory::Mesh;
            Issue.Title = FString::Printf(TEXT("High Poly Count: %s"), *Mesh->GetName());

            // Improved severity logic
            if (TriangleCount > MaxTrianglesPerMesh * 3)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
                Issue.EstimatedImpact = 90.0f;
            }
            else if (TriangleCount > MaxTrianglesPerMesh * 1.5)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
                Issue.EstimatedImpact = 65.0f;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Warning;
                Issue.EstimatedImpact = 60.0f;
            }

            Issue.Description = FString::Printf(
                TEXT("Mesh has %d triangles (threshold: %d)"),
                TriangleCount,
                MaxTrianglesPerMesh
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Reduce polygon count or create LODs");
            Issues.Add(Issue);
        }

        // Check for missing LODs
        if (Mesh->GetNumLODs() <= 1 && TriangleCount > 10000)
        {
            FOptimizationIssue Issue;
            Issue.Category = EOptimizationCategory::Mesh;
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
            Issue.Category = EOptimizationCategory::Texture;
            Issue.Title = FString::Printf(TEXT("Large Texture: %s"), *Texture->GetName());

            // Improved severity logic
            if (MaxDimension > 8192)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
                Issue.EstimatedImpact = 85.0f;
            }
            else if (MaxDimension > 4096)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
                Issue.EstimatedImpact = 55.0f;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Warning;
                Issue.EstimatedImpact = 45.0f;
            }

            Issue.Description = FString::Printf(
                TEXT("Texture size: %dx%d (threshold: %d)"),
                Texture->GetSizeX(),
                Texture->GetSizeY(),
                MaxTextureSize
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Resize texture or enable virtual texturing");
            Issues.Add(Issue);
        }
    }

    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckMaterials()
{
    TArray<FOptimizationIssue> Issues;
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckBlueprints()
{
    TArray<FOptimizationIssue> Issues;

    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> BlueprintAssets;
    AssetRegistryModule.Get().GetAssetsByClass(
        UBlueprint::StaticClass()->GetClassPathName(),
        BlueprintAssets
    );

    UE_LOG(LogTemp, Log, TEXT("Checking %d blueprints..."), BlueprintAssets.Num());

    for (const FAssetData& AssetData : BlueprintAssets)
    {
        UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
        if (!Blueprint) continue;

        // Skip engine content
        FString PackagePath = AssetData.PackageName.ToString();
        if (PackagePath.StartsWith(TEXT("/Engine/")))
        {
            continue;
        }

        int32 TotalNodes = 0;
        bool bHasEventTick = false;

        // Count nodes in all graphs
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (!Graph) continue;

            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (!Node) continue;

                TotalNodes++;

                // Check for Event Tick
                FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                if (NodeTitle.Contains(TEXT("Event Tick")))
                {
                    bHasEventTick = true;
                }
            }
        }

        // Check function graphs as well
        for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
        {
            if (!FunctionGraph) continue;

            for (UEdGraphNode* Node : FunctionGraph->Nodes)
            {
                if (Node) TotalNodes++;
            }
        }

        // Issue 1: Too many nodes
        if (TotalNodes > MaxBlueprintNodes)
        {
            FOptimizationIssue Issue;
            Issue.Category = EOptimizationCategory::Blueprint;
            Issue.Title = FString::Printf(TEXT("Complex Blueprint: %s"), *Blueprint->GetName());

            if (TotalNodes > MaxBlueprintNodes * 2)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
                Issue.EstimatedImpact = 80.0f;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Warning;
                Issue.EstimatedImpact = 60.0f;
            }

            Issue.Description = FString::Printf(
                TEXT("Blueprint has %d nodes (threshold: %d). Complex blueprints can cause compilation and performance issues."),
                TotalNodes,
                MaxBlueprintNodes
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Refactor into smaller blueprints or move logic to C++");
            Issues.Add(Issue);
        }

        // Issue 2: Has Event Tick (any Event Tick is potentially problematic)
        if (bHasEventTick && TotalNodes > 100)
        {
            FOptimizationIssue Issue;
            Issue.Category = EOptimizationCategory::Blueprint;
            Issue.Title = FString::Printf(TEXT("Blueprint with Event Tick: %s"), *Blueprint->GetName());
            Issue.Severity = EOptimizationSeverity::Warning;
            Issue.EstimatedImpact = 65.0f;
            Issue.Description = FString::Printf(
                TEXT("Blueprint contains Event Tick with %d total nodes. Event Tick runs every frame."),
                TotalNodes
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Consider using Timers instead of Tick, or reduce tick frequency");
            Issues.Add(Issue);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Blueprint check complete: %d issues found"), Issues.Num());
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckAudio()
{
    TArray<FOptimizationIssue> Issues;
    return Issues;
}

TArray<FOptimizationIssue> UOptimizationAnalyzer::CheckParticleSystems()
{
    TArray<FOptimizationIssue> Issues;
    return Issues;
}