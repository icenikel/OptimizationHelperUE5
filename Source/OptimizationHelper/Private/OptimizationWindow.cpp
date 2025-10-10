#include "OptimizationWindow.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "OptimizationWindow"

void SOptimizationWindow::Construct(const FArguments& InArgs)
{
    Analyzer = NewObject<UOptimizationAnalyzer>();

    ChildSlot
    [
        SNew(SVerticalBox)
        
        // Title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WindowTitle", "Optimization Helper"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
        ]
        
        // Analyze button
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("AnalyzeButton", "Analyze Project"))
            .OnClicked(this, &SOptimizationWindow::OnAnalyzeClicked)
            .HAlign(HAlign_Center)
        ]
        
        // Status text
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SAssignNew(StatusText, STextBlock)
            .Text(LOCTEXT("StatusReady", "Ready to analyze. Click the button above."))
        ]
        
        // Issues list
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(10.0f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SAssignNew(IssueListView, SListView<TSharedPtr<FOptimizationIssue>>)
                .ListItemsSource(&Issues)
                .OnGenerateRow(this, &SOptimizationWindow::OnGenerateIssueRow)
            ]
        ]
    ];
}

FReply SOptimizationWindow::OnAnalyzeClicked()
{
    if (!Analyzer)
    {
        StatusText->SetText(LOCTEXT("AnalyzerError", "Error: Analyzer not initialized"));
        return FReply::Handled();
    }

    // Clear previous results
    Issues.Empty();
    StatusText->SetText(LOCTEXT("Analyzing", "Analyzing project..."));

    // Run analysis
    TArray<FOptimizationIssue> MeshIssues = Analyzer->CheckMeshes();
    TArray<FOptimizationIssue> TextureIssues = Analyzer->CheckTextures();

    // Combine all issues
    TArray<FOptimizationIssue> AllIssues;
    AllIssues.Append(MeshIssues);
    AllIssues.Append(TextureIssues);

    // Sort by severity and impact
    AllIssues.Sort([](const FOptimizationIssue& A, const FOptimizationIssue& B)
    {
        if (A.Severity != B.Severity)
        {
            return A.Severity > B.Severity;
        }
        return A.EstimatedImpact > B.EstimatedImpact;
    });

    // Convert to shared pointers
    for (const FOptimizationIssue& Issue : AllIssues)
    {
        Issues.Add(MakeShared<FOptimizationIssue>(Issue));
    }

    // Refresh list
    if (IssueListView.IsValid())
    {
        IssueListView->RequestListRefresh();
    }

    // Update status
    FText StatusMessage = FText::Format(
        LOCTEXT("AnalysisComplete", "Analysis complete! Found {0} issues."),
        FText::AsNumber(Issues.Num())
    );
    StatusText->SetText(StatusMessage);

    UE_LOG(LogTemp, Warning, TEXT("OptimizationHelper: Found %d issues"), Issues.Num());

    return FReply::Handled();
}

TSharedRef<ITableRow> SOptimizationWindow::OnGenerateIssueRow(
    TSharedPtr<FOptimizationIssue> Issue,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    FLinearColor SeverityColor = FLinearColor::White;
    FString SeverityText;

    switch (Issue->Severity)
    {
        case EOptimizationSeverity::Critical:
            SeverityColor = FLinearColor::Red;
            SeverityText = TEXT("CRITICAL");
            break;
        case EOptimizationSeverity::Warning:
            SeverityColor = FLinearColor::Yellow;
            SeverityText = TEXT("WARNING");
            break;
        case EOptimizationSeverity::Info:
            SeverityColor = FLinearColor::Green;
            SeverityText = TEXT("INFO");
            break;
    }

    return SNew(STableRow<TSharedPtr<FOptimizationIssue>>, OwnerTable)
    .Padding(5.0f)
    [
        SNew(SBox)
        .Padding(5.0f)
        [
            SNew(SHorizontalBox)
            
            // Severity badge
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5.0f)
            .VAlign(VAlign_Top)
            [
                SNew(SBox)
                .WidthOverride(80.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(SeverityText))
                    .ColorAndOpacity(SeverityColor)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                ]
            ]
            
            // Issue details
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(5.0f)
            [
                SNew(SVerticalBox)
                
                // Title
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Issue->Title))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                ]
                
                // Description
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 2.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Issue->Description))
                    .AutoWrapText(true)
                ]
                
                // Suggested fix
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 2.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("💡 Fix: %s"), *Issue->SuggestedFix)))
                    .ColorAndOpacity(FLinearColor(0.6f, 0.8f, 1.0f))
                    .AutoWrapText(true)
                ]
                
                // Asset path
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 2.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("📁 %s"), *Issue->AssetPath)))
                    .ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                ]
            ]
            
            // Impact percentage
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(80.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("Impact:\n%.0f%%"), Issue->EstimatedImpact)))
                    .Justification(ETextJustify::Center)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                ]
            ]
        ]
    ];
}

#undef LOCTEXT_NAMESPACE