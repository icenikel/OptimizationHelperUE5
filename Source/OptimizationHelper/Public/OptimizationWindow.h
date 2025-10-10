#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "OptimizationAnalyzer.h"
#include "Widgets/Views/SListView.h"

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
    FReply OnExportClicked();

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

    // UI Elements
    TArray<TSharedPtr<FOptimizationIssue>> Issues;
    TSharedPtr<SListView<TSharedPtr<FOptimizationIssue>>> IssueListView;
    TSharedPtr<STextBlock> StatusText;
    TSharedPtr<SSpinBox<float>> MaxTrianglesSpinBox;
    TSharedPtr<SSpinBox<float>> MaxTextureSizeSpinBox;

    // Logic
    UOptimizationAnalyzer* Analyzer;
};

class FOptimizationWindow
{
public:
    static void OpenWindow();
};