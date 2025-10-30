#include "OptimizationAnalyzer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
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

            // ← НОВАЯ ФОРМУЛА IMPACT
            // Calculate how much the mesh exceeds the threshold
            float ExcessRatio = (float)TriangleCount / MaxTrianglesPerMesh;
            // Impact scales with excess: 10% over = ~16%, 100% over = ~70%, 200% over = 100%
            float BaseImpact = FMath::Clamp((ExcessRatio - 1.0f) * 60.0f + 10.0f, 10.0f, 100.0f);

            Issue.EstimatedImpact = BaseImpact;

            // Determine severity based on calculated impact
            if (BaseImpact > 80.0f)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
            }
            else if (BaseImpact > 50.0f)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Info;
            }

            Issue.Description = FString::Printf(
                TEXT("Mesh has %d triangles (threshold: %d, %.1fx over limit)"),
                TriangleCount,
                MaxTrianglesPerMesh,
                ExcessRatio
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
            Issue.Description = FString::Printf(
                TEXT("High-poly mesh (%d triangles) has no LOD chain"),
                TriangleCount
            );
            Issue.Severity = EOptimizationSeverity::Warning;
            Issue.AssetPath = AssetData.GetObjectPathString();

            // ← НОВАЯ ФОРМУЛА: Impact based on triangle count
            // More triangles = more important to have LODs
            float TriangleRatio = (float)TriangleCount / 50000.0f;
            Issue.EstimatedImpact = FMath::Clamp(TriangleRatio * 40.0f + 20.0f, 20.0f, 70.0f);

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

            // ← НОВАЯ ФОРМУЛА IMPACT
            // Calculate excess ratio
            float ExcessRatio = (float)MaxDimension / MaxTextureSize;

            // Estimate memory usage (RGBA format)
            int32 EstimatedMemoryMB = (MaxDimension * MaxDimension * 4) / (1024 * 1024);

            // Impact based on both size excess and memory cost
            float SizeImpact = (ExcessRatio - 1.0f) * 45.0f;
            float MemoryImpact = FMath::Min(EstimatedMemoryMB / 8.0f, 40.0f);  // Up to 40 points for memory
            float BaseImpact = FMath::Clamp(SizeImpact + MemoryImpact + 10.0f, 10.0f, 100.0f);

            Issue.EstimatedImpact = BaseImpact;

            // Determine severity based on impact
            if (BaseImpact > 75.0f)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
            }
            else if (BaseImpact > 45.0f)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Info;
            }

            Issue.Description = FString::Printf(
                TEXT("Texture size: %dx%d (threshold: %d, %.1fx over limit, ~%d MB)"),
                Texture->GetSizeX(),
                Texture->GetSizeY(),
                MaxTextureSize,
                ExcessRatio,
                EstimatedMemoryMB
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

    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> MaterialAssets;
    AssetRegistryModule.Get().GetAssetsByClass(
        UMaterial::StaticClass()->GetClassPathName(),
        MaterialAssets
    );

    UE_LOG(LogTemp, Log, TEXT("Checking %d materials..."), MaterialAssets.Num());

    for (const FAssetData& AssetData : MaterialAssets)
    {
        UMaterial* Material = Cast<UMaterial>(AssetData.GetAsset());
        if (!Material) continue;

        // Skip engine materials
        FString PackagePath = AssetData.PackageName.ToString();
        if (PackagePath.StartsWith(TEXT("/Engine/")))
        {
            continue;
        }

        // Issue 1: Count texture samples
        TArray<UTexture*> UsedTextures;
        Material->GetUsedTextures(UsedTextures, EMaterialQualityLevel::High, true, ERHIFeatureLevel::SM5, true);

        int32 TextureSampleCount = UsedTextures.Num();

        // ← ИСПОЛЬЗОВАНИЕ ПЕРЕМЕННОЙ
        if (TextureSampleCount > MaxTextureSamplesPerMaterial)
        {
            FOptimizationIssue Issue;
            Issue.Category = EOptimizationCategory::Material;
            Issue.Title = FString::Printf(TEXT("Too Many Textures: %s"), *Material->GetName());

            // Calculate impact based on texture count
            float ExcessRatio = (float)TextureSampleCount / MaxTextureSamplesPerMaterial;  // ← ПЕРЕМЕННАЯ
            float BaseImpact = FMath::Clamp((ExcessRatio - 1.0f) * 50.0f + 20.0f, 20.0f, 95.0f);
            Issue.EstimatedImpact = BaseImpact;

            if (BaseImpact > 70.0f)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
            }
            else if (BaseImpact > 45.0f)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Info;
            }

            Issue.Description = FString::Printf(
                TEXT("Material uses %d texture samples (recommended: ≤%d). Each texture sample impacts GPU performance."),
                TextureSampleCount,
                MaxTextureSamplesPerMaterial  // ← ПЕРЕМЕННАЯ
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Reduce texture count, combine textures into atlases, or use texture packing (RGB channels)");
            Issues.Add(Issue);
        }

        // Issue 2: Check if Two Sided is enabled
        if (Material->IsTwoSided())
        {
            FOptimizationIssue Issue;
            Issue.Category = EOptimizationCategory::Material;
            Issue.Title = FString::Printf(TEXT("Two-Sided Material: %s"), *Material->GetName());
            Issue.Severity = EOptimizationSeverity::Warning;
            Issue.EstimatedImpact = 35.0f;
            Issue.Description = TEXT("Material is set to Two-Sided, which doubles rendering cost. Only use when absolutely necessary (foliage, cloth).");
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Disable Two-Sided if back faces are never visible, or use proper two-sided geometry");
            Issues.Add(Issue);
        }

        // Issue 3: Check for expensive blend modes
        if (Material->GetBlendMode() == BLEND_Translucent ||
            Material->GetBlendMode() == BLEND_Additive ||
            Material->GetBlendMode() == BLEND_Modulate)
        {
            // Only flag if it also has many textures or is complex
            if (TextureSampleCount > 5)
            {
                FOptimizationIssue Issue;
                Issue.Category = EOptimizationCategory::Material;
                Issue.Title = FString::Printf(TEXT("Complex Translucent Material: %s"), *Material->GetName());
                Issue.Severity = EOptimizationSeverity::Warning;

                float BaseImpact = FMath::Clamp(TextureSampleCount * 8.0f, 30.0f, 80.0f);
                Issue.EstimatedImpact = BaseImpact;

                Issue.Description = FString::Printf(
                    TEXT("Translucent material with %d textures. Translucency is expensive and doesn't support many optimizations."),
                    TextureSampleCount
                );
                Issue.AssetPath = AssetData.GetObjectPathString();
                Issue.SuggestedFix = TEXT("Use Masked blend mode if possible, reduce texture samples, or use simpler shader");
                Issues.Add(Issue);
            }
        }

        // Issue 4: Check shader complexity
        int32 EstimatedInstructions = 0;
        EstimatedInstructions += 50;
        EstimatedInstructions += TextureSampleCount * 15;

        if (Material->IsTwoSided())
        {
            EstimatedInstructions *= 2;
        }

        if (Material->GetBlendMode() == BLEND_Translucent)
        {
            EstimatedInstructions += 30;
        }

        if (EstimatedInstructions > 300)
        {
            FOptimizationIssue Issue;
            Issue.Category = EOptimizationCategory::Material;
            Issue.Title = FString::Printf(TEXT("Complex Shader: %s"), *Material->GetName());

            float ComplexityRatio = (float)EstimatedInstructions / 300.0f;
            float BaseImpact = FMath::Clamp((ComplexityRatio - 1.0f) * 60.0f + 25.0f, 25.0f, 90.0f);
            Issue.EstimatedImpact = BaseImpact;

            if (BaseImpact > 70.0f)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
            }
            else if (BaseImpact > 45.0f)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Info;
            }

            Issue.Description = FString::Printf(
                TEXT("Material has approximately %d shader instructions (threshold: 300). Complex shaders impact GPU performance."),
                EstimatedInstructions
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Simplify shader logic, use Material Instances, or create LOD materials");
            Issues.Add(Issue);
        }
    }

    // Check Material Instances usage
    TArray<FAssetData> MaterialInstanceAssets;
    AssetRegistryModule.Get().GetAssetsByClass(
        UMaterialInstance::StaticClass()->GetClassPathName(),
        MaterialInstanceAssets
    );

    UE_LOG(LogTemp, Log, TEXT("Checking %d material instances..."), MaterialInstanceAssets.Num());

    int32 BaseMatCount = MaterialAssets.Num();
    int32 InstanceCount = MaterialInstanceAssets.Num();

    if (BaseMatCount > 10 && InstanceCount < BaseMatCount * 2)
    {
        FOptimizationIssue Issue;
        Issue.Category = EOptimizationCategory::Material;
        Issue.Title = TEXT("Project: Underusing Material Instances");
        Issue.Severity = EOptimizationSeverity::Warning;

        float InstanceRatio = (float)InstanceCount / BaseMatCount;
        float BaseImpact = FMath::Clamp((3.0f - InstanceRatio) * 20.0f, 25.0f, 60.0f);
        Issue.EstimatedImpact = BaseImpact;

        Issue.Description = FString::Printf(
            TEXT("Project has %d base materials but only %d instances (ratio: %.1f:1). Recommended ratio: >3:1"),
            BaseMatCount,
            InstanceCount,
            InstanceRatio
        );
        Issue.AssetPath = TEXT("Project-wide");
        Issue.SuggestedFix = TEXT("Create Material Instances instead of new base materials. Use parameter-driven master materials.");
        Issues.Add(Issue);
    }

    UE_LOG(LogTemp, Log, TEXT("Material check complete: %d issues found"), Issues.Num());
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

            // ← НОВАЯ ФОРМУЛА IMPACT
            // Calculate complexity ratio
            float ExcessRatio = (float)TotalNodes / MaxBlueprintNodes;

            // Base impact from node count excess
            float BaseImpact = FMath::Clamp((ExcessRatio - 1.0f) * 55.0f + 15.0f, 15.0f, 100.0f);

            // Blueprint complexity affects both compile time and runtime
            // Large blueprints also harder to maintain
            Issue.EstimatedImpact = BaseImpact;

            // Determine severity
            if (BaseImpact > 75.0f)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
            }
            else if (BaseImpact > 45.0f)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Info;
            }

            Issue.Description = FString::Printf(
                TEXT("Blueprint has %d nodes (threshold: %d, %.1fx over limit). Complex blueprints cause compilation and performance issues."),
                TotalNodes,
                MaxBlueprintNodes,
                ExcessRatio
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Refactor into smaller blueprints or move logic to C++");
            Issues.Add(Issue);
        }

        // Issue 2: Has Event Tick
        if (bHasEventTick && TotalNodes > 100)
        {
            FOptimizationIssue Issue;
            Issue.Category = EOptimizationCategory::Blueprint;
            Issue.Title = FString::Printf(TEXT("Blueprint with Event Tick: %s"), *Blueprint->GetName());

            // ← НОВАЯ ФОРМУЛА IMPACT
            // Event Tick is critical - runs every frame!
            // Impact scales with total blueprint complexity
            float ComplexityRatio = (float)TotalNodes / 200.0f;
            float BaseImpact = FMath::Clamp(ComplexityRatio * 60.0f + 25.0f, 25.0f, 95.0f);

            // Tick makes everything worse - multiply by severity
            Issue.EstimatedImpact = BaseImpact;

            if (BaseImpact > 70.0f)
            {
                Issue.Severity = EOptimizationSeverity::Critical;
            }
            else if (BaseImpact > 40.0f)
            {
                Issue.Severity = EOptimizationSeverity::Warning;
            }
            else
            {
                Issue.Severity = EOptimizationSeverity::Info;
            }

            Issue.Description = FString::Printf(
                TEXT("Blueprint contains Event Tick with %d total nodes. Event Tick runs every frame and significantly impacts performance."),
                TotalNodes
            );
            Issue.AssetPath = AssetData.GetObjectPathString();
            Issue.SuggestedFix = TEXT("Use Timers instead of Tick, or reduce tick frequency with 'Set Actor Tick Interval'");
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