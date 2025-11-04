#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPerformanceMonitorWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPerformanceMonitorWidget) {}
        SLATE_ARGUMENT(class UOptimizationAnalyzer*, Analyzer)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Tick function to update stats
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    // UI elements
    TSharedPtr<STextBlock> FPSText;
    TSharedPtr<STextBlock> FrameTimeText;
    TSharedPtr<STextBlock> DrawCallsText;
    TSharedPtr<STextBlock> TrianglesText;
    TSharedPtr<STextBlock> MemoryText;
    TSharedPtr<STextBlock> TextureStreamingText;

    // Stats tracking
    float UpdateInterval = 0.5f;  // Update every 0.5 seconds
    float TimeSinceLastUpdate = 0.0f;

    // Cached stats
    float CurrentFPS = 0.0f;
    float CurrentFrameTime = 0.0f;
    int32 CurrentDrawCalls = 0;
    int32 CurrentTriangles = 0;
    float CurrentMemoryMB = 0.0f;
    int32 CurrentStreamingTextures = 0;

    // Reference to analyzer
    class UOptimizationAnalyzer* Analyzer = nullptr;

    // Helper functions
    void UpdateStats();
    FLinearColor GetFPSColor(float FPS) const;
    FLinearColor GetFrameTimeColor(float MS) const;
};