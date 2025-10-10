#include "OptimizationWindow.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ContentBrowserModule.h"  // ← НОВОЕ!
#include "IContentBrowserSingleton.h"  // ← НОВОЕ!

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

                // Settings panel
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.0f)
                [
                    SNew(SBorder)
                        .BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f))
                        .Padding(10.0f)
                        [
                            SNew(SVerticalBox)

                                // Settings title
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("SettingsTitle", "Analysis Settings"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                                ]

                                // Settings row
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 5.0f)
                                [
                                    SNew(SHorizontalBox)

                                        // Max Triangles
                                        + SHorizontalBox::Slot()
                                        .FillWidth(1.0f)
                                        .Padding(5.0f, 0.0f)
                                        [
                                            SNew(SVerticalBox)

                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                [
                                                    SNew(STextBlock)
                                                        .Text(LOCTEXT("MaxTriangles", "Max Triangles per Mesh"))
                                                ]

                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                [
                                                    SAssignNew(MaxTrianglesSpinBox, SSpinBox<float>)
                                                        .MinValue(10000.0f)
                                                        .MaxValue(1000000.0f)
                                                        .Value(100000.0f)
                                                        .Delta(10000.0f)
                                                        .OnValueChanged(this, &SOptimizationWindow::OnMaxTrianglesChanged)
                                                ]
                                        ]

                                    // Max Texture Size
                                    + SHorizontalBox::Slot()
                                        .FillWidth(1.0f)
                                        .Padding(5.0f, 0.0f)
                                        [
                                            SNew(SVerticalBox)

                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                [
                                                    SNew(STextBlock)
                                                        .Text(LOCTEXT("MaxTextureSize", "Max Texture Size"))
                                                ]

                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                [
                                                    SAssignNew(MaxTextureSizeSpinBox, SSpinBox<float>)
                                                        .MinValue(512.0f)
                                                        .MaxValue(8192.0f)
                                                        .Delta(512.0f)
                                                        .Value(2048.0f)
                                                        .OnValueChanged(this, &SOptimizationWindow::OnMaxTextureSizeChanged)
                                                ]
                                        ]
                                ]
                        ]
                ]

            // Buttons row
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.0f)
                [
                    SNew(SHorizontalBox)

                        // Analyze button
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(5.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("AnalyzeButton", "Analyze Project"))
                                .OnClicked(this, &SOptimizationWindow::OnAnalyzeClicked)
                                .HAlign(HAlign_Center)
                        ]

                        // Export button
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(5.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("ExportButton", "Export to CSV"))
                                .OnClicked(this, &SOptimizationWindow::OnExportClicked)
                                .HAlign(HAlign_Center)
                        ]
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

FReply SOptimizationWindow::OnExportClicked()
{
    if (Issues.Num() == 0)
    {
        StatusText->SetText(LOCTEXT("NoIssues", "No issues to export. Run analysis first."));
        return FReply::Handled();
    }

    // Create filename with current date and time
    FDateTime Now = FDateTime::Now();
    FString FileName = FString::Printf(
        TEXT("OptimizationReport_%04d-%02d-%02d_%02d-%02d-%02d.csv"),
        Now.GetYear(), Now.GetMonth(), Now.GetDay(),
        Now.GetHour(), Now.GetMinute(), Now.GetSecond()
    );

    // Save to project Saved folder
    FString SavePath = FPaths::ProjectSavedDir() / TEXT("OptimizationReports") / FileName;

    // Create directory if doesn't exist
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath), true);

    ExportToCSV(SavePath);

    FText Message = FText::Format(
        LOCTEXT("ExportSuccess", "Report exported to: {0}"),
        FText::FromString(SavePath)
    );
    StatusText->SetText(Message);

    UE_LOG(LogTemp, Log, TEXT("Exported report to: %s"), *SavePath);

    return FReply::Handled();
}

void SOptimizationWindow::ExportToCSV(const FString& FilePath)
{
    FString CSVContent;

    // Header
    CSVContent += TEXT("Severity,Title,Description,Impact (%),Asset Path,Suggested Fix\n");

    // Data rows
    for (const TSharedPtr<FOptimizationIssue>& Issue : Issues)
    {
        FString SeverityStr;
        switch (Issue->Severity)
        {
        case EOptimizationSeverity::Critical: SeverityStr = TEXT("Critical"); break;
        case EOptimizationSeverity::Warning: SeverityStr = TEXT("Warning"); break;
        case EOptimizationSeverity::Info: SeverityStr = TEXT("Info"); break;
        }

        // Escape commas and quotes in text
        FString Title = Issue->Title.Replace(TEXT(","), TEXT(";"));
        FString Description = Issue->Description.Replace(TEXT(","), TEXT(";"));
        FString SuggestedFix = Issue->SuggestedFix.Replace(TEXT(","), TEXT(";"));
        FString AssetPath = Issue->AssetPath.Replace(TEXT(","), TEXT(";"));

        CSVContent += FString::Printf(
            TEXT("%s,%s,%s,%.1f,%s,%s\n"),
            *SeverityStr,
            *Title,
            *Description,
            Issue->EstimatedImpact,
            *AssetPath,
            *SuggestedFix
        );
    }

    // Save to file
    FFileHelper::SaveStringToFile(CSVContent, *FilePath);
}

void SOptimizationWindow::OnMaxTrianglesChanged(float NewValue)
{
    if (Analyzer)
    {
        Analyzer->MaxTrianglesPerMesh = FMath::RoundToInt(NewValue);
        UE_LOG(LogTemp, Log, TEXT("Max triangles changed to: %d"), FMath::RoundToInt(NewValue));
    }
}

void SOptimizationWindow::OnMaxTextureSizeChanged(float NewValue)
{
    if (Analyzer)
    {
        Analyzer->MaxTextureSize = FMath::RoundToInt(NewValue);
        UE_LOG(LogTemp, Log, TEXT("Max texture size changed to: %d"), FMath::RoundToInt(NewValue));
    }
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

    // Создаем виджет с кнопкой для открытия ассета
    return SNew(STableRow<TSharedPtr<FOptimizationIssue>>, OwnerTable)
        .Padding(5.0f)
        [
            SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .OnClicked_Lambda([Issue]() -> FReply
                    {
                        // Open asset on click
                        if (!Issue->AssetPath.IsEmpty())
                        {
                            // Метод 1: Попробовать открыть редактор
                            UObject* Asset = LoadObject<UObject>(nullptr, *Issue->AssetPath);
                            if (Asset)
                            {
                                UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
                                if (AssetEditorSubsystem)
                                {
                                    AssetEditorSubsystem->OpenEditorForAsset(Asset);
                                    UE_LOG(LogTemp, Log, TEXT("Opened asset editor: %s"), *Issue->AssetPath);
                                }
                            }
                            else
                            {
                                // Метод 2: Если не удалось загрузить, попробовать синхронизировать в Content Browser
                                TArray<FString> AssetPaths;
                                AssetPaths.Add(Issue->AssetPath);

                                FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                                ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>(), true);

                                UE_LOG(LogTemp, Warning, TEXT("Could not load asset, synced in Content Browser: %s"), *Issue->AssetPath);
                            }
                        }
                        return FReply::Handled();
                    })
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

                                        // Asset path + hint
                                        + SVerticalBox::Slot()
                                        .AutoHeight()
                                        .Padding(0.0f, 2.0f)
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(FString::Printf(TEXT("📁 %s (Click to open)"), *Issue->AssetPath)))
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
                ]
        ];
}
#undef LOCTEXT_NAMESPACE