#include "OptimizationWindow.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Notifications/SProgressBar.h"  // ← НОВОЕ!
#include "Subsystems/AssetEditorSubsystem.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "OptimizationWindow"

void SOptimizationWindow::Construct(const FArguments& InArgs)
{
    Analyzer = NewObject<UOptimizationAnalyzer>();
    Analyzer->MaxBlueprintNodes = 200;
    CurrentFilter = EFilterType::All;

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

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.0f, 2.0f)
                [
                    SNew(STextBlock)
                        .Text(LOCTEXT("ImpactLegend",
                            "Impact: estimated performance cost (0-100%). Higher = more urgent to fix. "
                            "Calculated based on how much threshold is exceeded."))
                        .ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
                        .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
                        .AutoWrapText(true)
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

                                    // Max Blueprint Nodes
                                    + SHorizontalBox::Slot()
                                        .FillWidth(1.0f)
                                        .Padding(5.0f, 0.0f)
                                        [
                                            SNew(SVerticalBox)
                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                [
                                                    SNew(STextBlock)
                                                        .Text(LOCTEXT("MaxBlueprintNodes", "Max Blueprint Nodes"))
                                                ]
                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                [
                                                    SAssignNew(MaxBlueprintNodesSpinBox, SSpinBox<float>)
                                                        .MinValue(100.0f)
                                                        .MaxValue(2000.0f)
                                                        .Delta(50.0f)
                                                        .Value(200.0f)  // ← Значение по умолчанию
                                                        .OnValueChanged(this, &SOptimizationWindow::OnMaxBlueprintNodesChanged)
                                                ]
                                        ]
                                ]
                        ]
                ]

            // Filter buttons
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.0f, 5.0f)
                [
                    SNew(SHorizontalBox)

                        // Label
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(5.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("FilterLabel", "Filter:"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                        ]

                        // All button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("FilterAll", "All"))
                                .OnClicked(this, &SOptimizationWindow::OnFilterAll)
                                .ToolTipText(LOCTEXT("FilterAllTooltip", "Show all issues"))
                        ]

                        // Critical button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("FilterCritical", "Critical"))
                                .OnClicked(this, &SOptimizationWindow::OnFilterCritical)
                                .ButtonColorAndOpacity(FLinearColor(0.8f, 0.2f, 0.2f, 1.0f))
                                .ToolTipText(LOCTEXT("FilterCriticalTooltip", "Show only critical issues"))
                        ]

                    // Warning button
                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("FilterWarning", "Warning"))
                                .OnClicked(this, &SOptimizationWindow::OnFilterWarning)
                                .ButtonColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.2f, 1.0f))
                                .ToolTipText(LOCTEXT("FilterWarningTooltip", "Show only warnings"))
                        ]

                    // Info button
                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("FilterInfo", "Info"))
                                .OnClicked(this, &SOptimizationWindow::OnFilterInfo)
                                .ButtonColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f, 1.0f))
                                .ToolTipText(LOCTEXT("FilterInfoTooltip", "Show only info messages"))
                        ]

                    // Meshes button
                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("FilterMeshes", "Meshes"))
                                .OnClicked(this, &SOptimizationWindow::OnFilterMeshes)
                                .ToolTipText(LOCTEXT("FilterMeshesTooltip", "Show only mesh issues"))
                        ]

                        // Textures button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("FilterTextures", "Textures"))
                                .OnClicked(this, &SOptimizationWindow::OnFilterTextures)
                                .ToolTipText(LOCTEXT("FilterTexturesTooltip", "Show only texture issues"))
                        ]

                        // Blueprints button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("FilterBlueprints", "Blueprints"))
                                .OnClicked(this, &SOptimizationWindow::OnFilterBlueprints)
                                .ToolTipText(LOCTEXT("FilterBlueprintsTooltip", "Show only blueprint issues"))
                        ]
                ]

            // Buttons row
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.0f)
                [
                    SNew(SHorizontalBox)

                        
                                // AnalyzeCurrentLevel
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                .Padding(5.0f, 0.0f)
                                [
                                    SNew(SButton)
                                        .Text(LOCTEXT("AnalyzeCurrentLevelButton", "Analyze Current Level"))
                                        .ToolTipText(LOCTEXT("AnalyzeCurrentLevelTooltip", "Quick analysis of the currently opened level"))
                                        .OnClicked(this, &SOptimizationWindow::OnAnalyzeCurrentLevelClicked)
                                        .HAlign(HAlign_Center)
                                ]
            
           
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


            // Status and Progress section  ← ОБНОВЛЕНО!
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.0f)
                [
                    SNew(SVerticalBox)

                        // Status text
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SAssignNew(StatusText, STextBlock)
                                .Text(LOCTEXT("StatusReady", "Ready to analyze. Click the button above."))
                        ]

                        // Progress text
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SAssignNew(ProgressText, STextBlock)
                                .Text(FText::GetEmpty())
                                .Visibility(EVisibility::Collapsed)  // Hidden by default
                        ]

                        // Progress bar
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(SBox)
                                .HeightOverride(20.0f)  // Явная высота
                                [
                                    SAssignNew(ProgressBar, SProgressBar)
                                        .Percent(0.0f)  // ← Убрали TOptional
                                        .FillColorAndOpacity(FLinearColor(0.0f, 0.5f, 1.0f))  // Синий цвет заполнения
                                        .Visibility(EVisibility::Collapsed)
                                ]
                        ]
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
    AllIssues.Empty();
    Issues.Empty();

    // Show progress widgets
    if (ProgressBar.IsValid())
    {
        ProgressBar->SetVisibility(EVisibility::Visible);
        ProgressBar->SetPercent(0.0f);  // ← ИСПРАВЛЕНО
    }

    if (ProgressText.IsValid())
    {
        ProgressText->SetVisibility(EVisibility::Visible);
        ProgressText->SetText(LOCTEXT("ProgressStarting", "Starting analysis... (0%)"));
    }

    StatusText->SetText(LOCTEXT("Analyzing", "Analyzing project..."));

    // Force UI update
    FSlateApplication::Get().PumpMessages();
    FSlateApplication::Get().Tick();
    FPlatformProcess::Sleep(0.1f);

    // Step 1: Analyze Meshes (0-40%)
    UpdateProgress(LOCTEXT("ProgressMeshes", "Analyzing meshes..."), 0.1f);
    FPlatformProcess::Sleep(0.1f);

    TArray<FOptimizationIssue> MeshIssues = Analyzer->CheckMeshes();
    UpdateProgress(LOCTEXT("ProgressMeshesDone", "Meshes analyzed"), 0.4f);
    FPlatformProcess::Sleep(0.1f);

    // Step 2: Analyze Textures (40-70%)
    UpdateProgress(LOCTEXT("ProgressTextures", "Analyzing textures..."), 0.5f);
    FPlatformProcess::Sleep(0.05f);

    TArray<FOptimizationIssue> TextureIssues = Analyzer->CheckTextures();
    UpdateProgress(LOCTEXT("ProgressTexturesDone", "Textures analyzed"), 0.7f);
    FPlatformProcess::Sleep(0.05f);

    // Step 3: Analyze Materials (70-80%)
    UpdateProgress(LOCTEXT("ProgressMaterials", "Analyzing materials..."), 0.75f);
    FPlatformProcess::Sleep(0.05f);

    TArray<FOptimizationIssue> MaterialIssues = Analyzer->CheckMaterials();
    UpdateProgress(LOCTEXT("ProgressMaterialsDone", "Materials analyzed"), 0.8f);
    FPlatformProcess::Sleep(0.05f);

    // Step 4: Analyze Blueprints (80-90%)
    UpdateProgress(LOCTEXT("ProgressBlueprints", "Analyzing blueprints..."), 0.85f);
    FPlatformProcess::Sleep(0.05f);

    TArray<FOptimizationIssue> BlueprintIssues = Analyzer->CheckBlueprints();
    UpdateProgress(LOCTEXT("ProgressBlueprintsDone", "Blueprints analyzed"), 0.9f);
    FPlatformProcess::Sleep(0.05f);

    // Step 5: Finalize (90-100%)
    UpdateProgress(LOCTEXT("ProgressFinalizing", "Finalizing results..."), 0.95f);
    FPlatformProcess::Sleep(0.05f);

    // Combine all issues
    TArray<FOptimizationIssue> AllIssuesArray;
    AllIssuesArray.Append(MeshIssues);
    AllIssuesArray.Append(TextureIssues);
    AllIssuesArray.Append(MaterialIssues);
    AllIssuesArray.Append(BlueprintIssues);

    // Sort by severity and impact
    AllIssuesArray.Sort([](const FOptimizationIssue& A, const FOptimizationIssue& B)
        {
            if (A.Severity != B.Severity)
            {
                return A.Severity > B.Severity;
            }
            return A.EstimatedImpact > B.EstimatedImpact;
        });

    // Convert to shared pointers
    for (const FOptimizationIssue& Issue : AllIssuesArray)
    {
        AllIssues.Add(MakeShared<FOptimizationIssue>(Issue));
    }

    // Apply current filter
    CurrentFilter = EFilterType::All;
    ApplyFilter();

    // Complete
    UpdateProgress(LOCTEXT("ProgressComplete", "Analysis complete!"), 1.0f);
    FPlatformProcess::Sleep(0.3f);  // Show 100% for a moment

    // Hide progress bar
    if (ProgressBar.IsValid())
    {
        ProgressBar->SetVisibility(EVisibility::Collapsed);
    }
    if (ProgressText.IsValid())
    {
        ProgressText->SetVisibility(EVisibility::Collapsed);
    }

    // Update status
    FText StatusMessage = FText::Format(
        LOCTEXT("AnalysisComplete", "Analysis complete! Found {0} issues."),
        FText::AsNumber(AllIssues.Num())
    );
    StatusText->SetText(StatusMessage);

    UE_LOG(LogTemp, Warning, TEXT("OptimizationHelper: Found %d issues"), AllIssues.Num());

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

        // Escape commas in text
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

FReply SOptimizationWindow::OnAnalyzeCurrentLevelClicked()
{
    if (!Analyzer)
    {
        StatusText->SetText(LOCTEXT("AnalyzerError", "Error: Analyzer not initialized"));
        return FReply::Handled();
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Starting Current Level Analysis ==="));

    // Clear previous results
    AllIssues.Empty();
    Issues.Empty();

    // Show progress bar - ВАЖНО: сначала показать виджеты
    if (ProgressBar.IsValid())
    {
        ProgressBar->SetVisibility(EVisibility::Visible);
        ProgressBar->SetPercent(0.0f);
    }

    if (ProgressText.IsValid())
    {
        ProgressText->SetVisibility(EVisibility::Visible);
        ProgressText->SetText(LOCTEXT("ProgressStartingLevel", "Starting level analysis... (0%)"));
    }

    StatusText->SetText(LOCTEXT("AnalyzingLevel", "Analyzing current level..."));

    // Force UI update ПЕРЕД началом анализа
    FSlateApplication::Get().PumpMessages();
    FSlateApplication::Get().Tick();
    FPlatformProcess::Sleep(0.15f);  // Увеличил задержку

    // Step 1: Initialize (0-20%)
    UpdateProgress(LOCTEXT("ProgressLevelInit", "Initializing level scan..."), 0.1f);
    FPlatformProcess::Sleep(0.2f);

    UpdateProgress(LOCTEXT("ProgressLevelScan", "Scanning level actors..."), 0.2f);
    FPlatformProcess::Sleep(0.2f);

    // Step 2: Analyze (20-80%)
    UpdateProgress(LOCTEXT("ProgressLevelAnalyzing", "Analyzing meshes and textures..."), 0.4f);
    FPlatformProcess::Sleep(0.2f);

    // ОСНОВНОЙ АНАЛИЗ
    TArray<FOptimizationIssue> LevelIssues = Analyzer->AnalyzeCurrentLevel();

    UpdateProgress(LOCTEXT("ProgressLevelProcessing", "Processing results..."), 0.8f);
    FPlatformProcess::Sleep(0.2f);

    // Step 3: Finalize (80-100%)
    UpdateProgress(LOCTEXT("ProgressLevelFinalizing", "Finalizing..."), 0.9f);
    FPlatformProcess::Sleep(0.2f);

    // Convert to shared pointers
    for (const FOptimizationIssue& Issue : LevelIssues)
    {
        AllIssues.Add(MakeShared<FOptimizationIssue>(Issue));
    }

    // Apply filter
    CurrentFilter = EFilterType::All;
    ApplyFilter();

    // Complete
    UpdateProgress(LOCTEXT("ProgressLevelComplete", "Level analysis complete!"), 1.0f);
    FPlatformProcess::Sleep(0.5f);  // Показать 100% подольше

    // Hide progress bar
    if (ProgressBar.IsValid())
    {
        ProgressBar->SetVisibility(EVisibility::Collapsed);
    }
    if (ProgressText.IsValid())
    {
        ProgressText->SetVisibility(EVisibility::Collapsed);
    }

    // Update status
    FText StatusMessage = FText::Format(
        LOCTEXT("LevelAnalysisComplete", "Level analysis complete! Found {0} issues."),
        FText::AsNumber(AllIssues.Num())
    );
    StatusText->SetText(StatusMessage);

    UE_LOG(LogTemp, Warning, TEXT("=== Level Analysis Complete: %d issues found ==="), AllIssues.Num());

    return FReply::Handled();
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

void SOptimizationWindow::OnMaxBlueprintNodesChanged(float NewValue)
{
    if (Analyzer)
    {
        Analyzer->MaxBlueprintNodes = FMath::RoundToInt(NewValue);
        UE_LOG(LogTemp, Log, TEXT("Max Blueprint nodes changed to: %d"), FMath::RoundToInt(NewValue));
    }
}

FReply SOptimizationWindow::OnFilterAll()
{
    CurrentFilter = EFilterType::All;
    ApplyFilter();
    return FReply::Handled();
}

FReply SOptimizationWindow::OnFilterCritical()
{
    CurrentFilter = EFilterType::Critical;
    ApplyFilter();
    return FReply::Handled();
}

FReply SOptimizationWindow::OnFilterWarning()
{
    CurrentFilter = EFilterType::Warning;
    ApplyFilter();
    return FReply::Handled();
}

FReply SOptimizationWindow::OnFilterInfo()
{
    CurrentFilter = EFilterType::Info;
    ApplyFilter();
    return FReply::Handled();
}

FReply SOptimizationWindow::OnFilterMeshes()
{
    CurrentFilter = EFilterType::Meshes;
    ApplyFilter();
    return FReply::Handled();
}

FReply SOptimizationWindow::OnFilterBlueprints()
{
    CurrentFilter = EFilterType::Blueprints;
    ApplyFilter();
    return FReply::Handled();
}

FReply SOptimizationWindow::OnFilterTextures()
{
    CurrentFilter = EFilterType::Textures;
    ApplyFilter();
    return FReply::Handled();
}

void SOptimizationWindow::ApplyFilter()
{
    FilteredIssues.Empty();

    switch (CurrentFilter)
    {
    case EFilterType::All:
        FilteredIssues = AllIssues;
        break;

    case EFilterType::Critical:
        for (const auto& Issue : AllIssues)
        {
            if (Issue->Severity == EOptimizationSeverity::Critical)
            {
                FilteredIssues.Add(Issue);
            }
        }
        break;

    case EFilterType::Warning:
        for (const auto& Issue : AllIssues)
        {
            if (Issue->Severity == EOptimizationSeverity::Warning)
            {
                FilteredIssues.Add(Issue);
            }
        }
        break;

    case EFilterType::Info:
        for (const auto& Issue : AllIssues)
        {
            if (Issue->Severity == EOptimizationSeverity::Info)
            {
                FilteredIssues.Add(Issue);
            }
        }
        break;

    case EFilterType::Meshes:
        for (const auto& Issue : AllIssues)
        {
            if (Issue->Category == EOptimizationCategory::Mesh)
            {
                FilteredIssues.Add(Issue);
            }
        }
        break;

    case EFilterType::Textures:
        for (const auto& Issue : AllIssues)
        {
            if (Issue->Category == EOptimizationCategory::Texture)
            {
                FilteredIssues.Add(Issue);
            }
        }
        break;

    case EFilterType::Blueprints:
        for (const auto& Issue : AllIssues)
        {
            if (Issue->Category == EOptimizationCategory::Blueprint)
            {
                FilteredIssues.Add(Issue);
            }
        }
        break;
    }

    // Update list view to use filtered issues
    Issues = FilteredIssues;

    if (IssueListView.IsValid())
    {
        IssueListView->RequestListRefresh();
    }

    // Update status
    FText FilterMessage = FText::Format(
        LOCTEXT("FilterApplied", "Showing {0} of {1} issues"),
        FText::AsNumber(FilteredIssues.Num()),
        FText::AsNumber(AllIssues.Num())
    );
    StatusText->SetText(FilterMessage);

    UE_LOG(LogTemp, Log, TEXT("Filter applied: %d/%d issues shown"), FilteredIssues.Num(), AllIssues.Num());
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
            SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .OnClicked_Lambda([Issue]() -> FReply
                    {
                        // Open asset on click
                        if (!Issue->AssetPath.IsEmpty())
                        {
                            // Find asset in Asset Registry
                            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                            FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(Issue->AssetPath));

                            if (AssetData.IsValid())
                            {
                                // Show in Content Browser
                                TArray<FAssetData> AssetsToSync;
                                AssetsToSync.Add(AssetData);

                                FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                                ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);

                                UE_LOG(LogTemp, Log, TEXT("Highlighted asset in Content Browser: %s"), *Issue->AssetPath);
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

void SOptimizationWindow::UpdateProgress(const FText& CurrentTask, float Progress)
{
    if (ProgressBar.IsValid())
    {
        ProgressBar->SetPercent(Progress);
        ProgressBar->SetVisibility(EVisibility::Visible);
    }

    if (ProgressText.IsValid())
    {
        FText ProgressMessage = FText::Format(
            LOCTEXT("ProgressFormat", "{0} ({1}%)"),
            CurrentTask,
            FText::AsNumber(FMath::RoundToInt(Progress * 100))
        );
        ProgressText->SetText(ProgressMessage);
        ProgressText->SetVisibility(EVisibility::Visible);
    }

    if (StatusText.IsValid())
    {
        StatusText->SetText(CurrentTask);
    }

    // КРИТИЧНО: Принудительное обновление UI
    FSlateApplication::Get().PumpMessages();
    FSlateApplication::Get().Tick();  // ← Добавьте эту строку!

    UE_LOG(LogTemp, Warning, TEXT("Progress updated: %.1f%% - %s"), Progress * 100, *CurrentTask.ToString());
}

#undef LOCTEXT_NAMESPACE