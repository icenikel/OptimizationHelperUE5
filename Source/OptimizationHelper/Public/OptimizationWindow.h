#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "OptimizationAnalyzer.h"
#include "Widgets/Views/SListView.h"
#include <Widgets/Notifications/SProgressBar.h>

// Forward declarations
template <typename NumericType>
class SSpinBox;

class SOptimizationWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SOptimizationWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // Button handlers
    FReply OnAnalyzeClicked();
    FReply OnAnalyzeCurrentLevelClicked();
    FReply OnExportClicked();

    // Filter handlers
    FReply OnFilterAll();
    FReply OnFilterCritical();
    FReply OnFilterWarning();
    FReply OnFilterInfo();
    FReply OnFilterMeshes();
    FReply OnFilterTextures();
    FReply OnFilterBlueprints();
    FReply OnFilterMaterials();

    void ApplyFilter();

    // List generation
    TSharedRef<ITableRow> OnGenerateIssueRow(
        TSharedPtr<FOptimizationIssue> Issue,
        const TSharedRef<STableViewBase>& OwnerTable
    );

    // Export functionality
    void ExportToCSV(const FString& FilePath);

    // Settings handlers
    void OnMaxTrianglesChanged(float NewValue);
    void OnMaxTextureSizeChanged(float NewValue);
    void OnMaxBlueprintNodesChanged(float NewValue);
    void OnMaxTextureSamplesChanged(float NewValue);

    void UpdateProgress(const FText& CurrentTask, float Progress);

    // UI Elements
    TArray<TSharedPtr<FOptimizationIssue>> Issues;
    TArray<TSharedPtr<FOptimizationIssue>> AllIssues;
    TArray<TSharedPtr<FOptimizationIssue>> FilteredIssues;
    TSharedPtr<SListView<TSharedPtr<FOptimizationIssue>>> IssueListView;
    TSharedPtr<STextBlock> StatusText;
    TSharedPtr<STextBlock> ProgressText; 
    TSharedPtr<SProgressBar> ProgressBar;
    TSharedPtr<SSpinBox<float>> MaxTrianglesSpinBox;
    TSharedPtr<SSpinBox<float>> MaxTextureSizeSpinBox;
    TSharedPtr<SSpinBox<float>> MaxBlueprintNodesSpinBox;
    TSharedPtr<SSpinBox<float>> MaxTextureSamplesSpinBox;

    // Filter state
    enum class EFilterType
    {
        All,
        Critical,
        Warning,
        Info,
        Meshes,
        Textures,
        Blueprints,
        Materials
    };
    EFilterType CurrentFilter;

    // Logic
    UOptimizationAnalyzer* Analyzer;
};

class FOptimizationWindow
{
public:
    static void OpenWindow();
};