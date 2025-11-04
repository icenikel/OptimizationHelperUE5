#include "PerformanceMonitorWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Engine/Engine.h"
#include "RenderingThread.h"
#include "HAL/PlatformMemory.h"
#include "Styling/CoreStyle.h"
#include "OptimizationAnalyzer.h"

#define LOCTEXT_NAMESPACE "PerformanceMonitorWidget"

void SPerformanceMonitorWidget::Construct(const FArguments& InArgs)
{
    Analyzer = InArgs._Analyzer; 

    ChildSlot
        [
            SNew(SBorder)
                .BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.9f))
                .Padding(10.0f)
                [
                    SNew(SVerticalBox)

                        // Title
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("Title", "Performance Monitor"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                .ColorAndOpacity(FLinearColor::White)
                        ]

                        // FPS
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 2.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("FPSLabel", "FPS: "))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SAssignNew(FPSText, STextBlock)
                                        .Text(LOCTEXT("FPSValue", "0"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                                ]
                        ]

                    // Frame Time
                    + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 2.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("FrameTimeLabel", "Frame Time: "))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SAssignNew(FrameTimeText, STextBlock)
                                        .Text(LOCTEXT("FrameTimeValue", "0 ms"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                                ]
                        ]

                    // Draw Calls
                    + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 2.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("DrawCallsLabel", "Draw Calls: "))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SAssignNew(DrawCallsText, STextBlock)
                                        .Text(LOCTEXT("DrawCallsValue", "0"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                                ]
                        ]

                    // Triangles
                    + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 2.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("TrianglesLabel", "Triangles: "))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SAssignNew(TrianglesText, STextBlock)
                                        .Text(LOCTEXT("TrianglesValue", "0"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                                ]
                        ]

                    // Memory
                    + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 2.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("MemoryLabel", "Memory: "))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SAssignNew(MemoryText, STextBlock)
                                        .Text(LOCTEXT("MemoryValue", "0 MB"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                                ]
                        ]

                    // Texture Streaming
                    + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 2.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("StreamingLabel", "Streaming Textures: "))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SAssignNew(TextureStreamingText, STextBlock)
                                        .Text(LOCTEXT("StreamingValue", "0"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                                ]
                        ]
                ]
        ];
}

void SPerformanceMonitorWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    TimeSinceLastUpdate += InDeltaTime;

    if (TimeSinceLastUpdate >= UpdateInterval)
    {
        UpdateStats();
        TimeSinceLastUpdate = 0.0f;
    }
}

void SPerformanceMonitorWidget::UpdateStats()
{
    // DEBUG: Check if Analyzer exists
    if (!Analyzer)
    {
        UE_LOG(LogTemp, Error, TEXT("PerformanceMonitorWidget: Analyzer is NULL!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("PerformanceMonitorWidget: UpdateStats called, Analyzer is valid"));

    // Get FPS
    CurrentFPS = 1.0f / FApp::GetDeltaTime();
    CurrentFrameTime = FApp::GetDeltaTime() * 1000.0f;  // Convert to milliseconds

    // Get rendering stats from Analyzer
    if (Analyzer)
    {
        FPerformanceStats Stats = Analyzer->GetCurrentPerformanceStats();
        CurrentDrawCalls = Stats.DrawCalls;
        CurrentTriangles = Stats.Triangles;
        UE_LOG(LogTemp, Warning, TEXT("PerformanceMonitorWidget: Got stats - DrawCalls=%d, Triangles=%d"),
            CurrentDrawCalls, CurrentTriangles);
    }
    else
    {
        CurrentDrawCalls = 0;
        CurrentTriangles = 0;
    }

    // Get memory usage
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    CurrentMemoryMB = MemStats.UsedPhysical / (1024.0f * 1024.0f);

    // Get texture streaming count
    CurrentStreamingTextures = 0;  // Would need TextureStreamingManager access

    // Update UI
    if (FPSText.IsValid())
    {
        FPSText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), CurrentFPS)));
        FPSText->SetColorAndOpacity(GetFPSColor(CurrentFPS));
    }

    if (FrameTimeText.IsValid())
    {
        FrameTimeText->SetText(FText::FromString(FString::Printf(TEXT("%.2f ms"), CurrentFrameTime)));
        FrameTimeText->SetColorAndOpacity(GetFrameTimeColor(CurrentFrameTime));
    }

    if (DrawCallsText.IsValid())
    {
        // Color coding for Draw Calls
        FLinearColor DrawCallsColor = FLinearColor::Green;
        if (CurrentDrawCalls > 5000)
            DrawCallsColor = FLinearColor::Red;
        else if (CurrentDrawCalls > 2000)
            DrawCallsColor = FLinearColor::Yellow;

        DrawCallsText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CurrentDrawCalls)));
        DrawCallsText->SetColorAndOpacity(DrawCallsColor);
        // DEBUG
        UE_LOG(LogTemp, Warning, TEXT("UI UPDATE: DrawCalls text set to %d"), CurrentDrawCalls);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UI UPDATE: DrawCallsText is INVALID!"));
    
    }

    if (TrianglesText.IsValid())
    {
        // Format triangles (1.5M, 250.5K, or raw number)
        FString TrianglesStr;
        if (CurrentTriangles > 1000000)
            TrianglesStr = FString::Printf(TEXT("%.2fM"), CurrentTriangles / 1000000.0f);
        else if (CurrentTriangles > 1000)
            TrianglesStr = FString::Printf(TEXT("%.1fK"), CurrentTriangles / 1000.0f);
        else
            TrianglesStr = FString::Printf(TEXT("%d"), CurrentTriangles);

        TrianglesText->SetText(FText::FromString(TrianglesStr));
        // DEBUG
        UE_LOG(LogTemp, Warning, TEXT("UI UPDATE: Triangles text set to %s"), *TrianglesStr);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UI UPDATE: TrianglesText is INVALID!"));
    
    }

    if (MemoryText.IsValid())
    {
        MemoryText->SetText(FText::FromString(FString::Printf(TEXT("%.0f MB"), CurrentMemoryMB)));
    }

    if (TextureStreamingText.IsValid())
    {
        TextureStreamingText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CurrentStreamingTextures)));
    }
}

FLinearColor SPerformanceMonitorWidget::GetFPSColor(float FPS) const
{
    if (FPS >= 60.0f)
        return FLinearColor::Green;
    else if (FPS >= 30.0f)
        return FLinearColor::Yellow;
    else
        return FLinearColor::Red;
}

FLinearColor SPerformanceMonitorWidget::GetFrameTimeColor(float MS) const
{
    if (MS <= 16.67f)  // 60 FPS
        return FLinearColor::Green;
    else if (MS <= 33.33f)  // 30 FPS
        return FLinearColor::Yellow;
    else
        return FLinearColor::Red;
}

#undef LOCTEXT_NAMESPACE