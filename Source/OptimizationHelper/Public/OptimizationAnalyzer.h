#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OptimizationAnalyzer.generated.h"

// Forward declarations
class UEdGraphNode;

UENUM(BlueprintType)
enum class EOptimizationSeverity : uint8
{
    Info,
    Warning,
    Critical
};

UENUM(BlueprintType)
enum class EOptimizationCategory : uint8
{
    Mesh,
    Texture,
    Material,
    Blueprint,
    Audio,
    Particle,
    Other
};

USTRUCT(BlueprintType)
struct FOptimizationIssue
{
    GENERATED_BODY()

    FOptimizationIssue()
        : Severity(EOptimizationSeverity::Info)
        , Category(EOptimizationCategory::Other)
        , EstimatedImpact(0.0f)
    {
    }

    UPROPERTY(BlueprintReadWrite)
    FString Title;

    UPROPERTY(BlueprintReadWrite)
    FString Description;

    UPROPERTY(BlueprintReadWrite)
    EOptimizationSeverity Severity;

    UPROPERTY(BlueprintReadWrite)
    EOptimizationCategory Category;

    UPROPERTY(BlueprintReadWrite)
    FString AssetPath;

    UPROPERTY(BlueprintReadWrite)
    float EstimatedImpact; // 0-100 scale

    UPROPERTY(BlueprintReadWrite)
    FString SuggestedFix;
};

UCLASS()
class UOptimizationAnalyzer : public UObject
{
    GENERATED_BODY()

public:
    // Analysis functions
    TArray<FOptimizationIssue> AnalyzeCurrentLevel();
    TArray<FOptimizationIssue> AnalyzeProject();

    // Specific checks
    TArray<FOptimizationIssue> CheckMeshes();
    TArray<FOptimizationIssue> CheckTextures();
    TArray<FOptimizationIssue> CheckMaterials();
    TArray<FOptimizationIssue> CheckBlueprints();
    TArray<FOptimizationIssue> CheckAudio();
    TArray<FOptimizationIssue> CheckParticleSystems();

    // Configuration
    UPROPERTY()
    int32 MaxTrianglesPerMesh = 100000;

    UPROPERTY()
    int32 MaxTextureSize = 2048;

    UPROPERTY()
    int32 MaxBlueprintNodes = 200;


};