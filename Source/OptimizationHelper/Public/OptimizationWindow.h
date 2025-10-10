#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "OptimizationAnalyzer.h"
#include "Widgets/Views/SListView.h"

class SOptimizationWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SOptimizationWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnAnalyzeClicked();
    TSharedRef<ITableRow> OnGenerateIssueRow(
        TSharedPtr<FOptimizationIssue> Issue, 
        const TSharedRef<STableViewBase>& OwnerTable
    );
    
    TArray<TSharedPtr<FOptimizationIssue>> Issues;
    TSharedPtr<SListView<TSharedPtr<FOptimizationIssue>>> IssueListView;
    TSharedPtr<STextBlock> StatusText;
    
    UOptimizationAnalyzer* Analyzer;
};