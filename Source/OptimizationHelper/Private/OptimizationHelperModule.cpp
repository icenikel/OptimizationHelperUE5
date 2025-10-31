#include "OptimizationHelperModule.h"
#include "OptimizationWindow.h"  
#include "PerformanceMonitorWidget.h"
#include "ToolMenus.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FOptimizationHelperModule"

void FOptimizationHelperModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("OptimizationHelper: Plugin Started!"));
    
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(
            this, &FOptimizationHelperModule::RegisterMenus
        )
    );
}

void FOptimizationHelperModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

void FOptimizationHelperModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    if (Menu)
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

        // Existing entry
        Section.AddMenuEntry(
            "OptimizationHelper",
            LOCTEXT("MenuEntry", "Optimization Helper"),
            LOCTEXT("MenuTooltip", "Opens Optimization Helper window"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(
                this, &FOptimizationHelperModule::OnOpenOptimizationWindow
            ))
        );

    }
}

void FOptimizationHelperModule::OnOpenOptimizationWindow()
{
    UE_LOG(LogTemp, Error, TEXT("=== OPENING NEW WINDOW VERSION 2.0 ===")); // ← Добавьте эту строку
    
    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("WindowTitle", "Optimization Helper"))
        .ClientSize(FVector2D(1200, 700))
        [
            SNew(SOptimizationWindow)  // ← НЕ STextBlock!
        ];

    FSlateApplication::Get().AddWindow(Window);
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOptimizationHelperModule, OptimizationHelper)