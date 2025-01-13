#include "AssetDependencyViewerWidget.h"

#include "AssetManagerEditorModule.h"
#include "Framework/Commands/GenericCommands.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
	const FName DependencyViewer_ColumnID_Name{TEXT("Name")};
	const FName DependencyViewer_ColumnID_MemorySize{TEXT("Memory Size")};
	const FName DependencyViewer_ColumnID_DiskSize{TEXT("Disk Size")};
	const FName DependencyViewer_ColumnID_Type{TEXT("Type")};
	const FName DependencyViewer_ColumnID_Path{TEXT("Path")};
	const FName DependencyViewer_ColumnID_DependencyCount{TEXT("Dependency Count")};
	const FName DependencyViewer_ColumnID_DependencyDepth{TEXT("Dependency Depth")};
}

void SAssetDependencyViewerRowWidget::Construct(const FArguments& InArgs)
{
	AuditViewer = InArgs._AuditViewer;
	Item = InArgs._Item;
	
	SMultiColumnTableRow::Construct(FSuperRowType::FArguments().Padding(0.0f), AuditViewer->ListView.ToSharedRef());
}

TSharedRef<SWidget> SAssetDependencyViewerRowWidget::GenerateWidgetForColumn(const FName& ColumnName)
{
	using namespace UE::AssetValidation;

	FString Text;
	if (ColumnName == DependencyViewer_ColumnID_Name)
	{
		Text = Item->AssetData.AssetName.ToString();
	}
	else if (ColumnName == DependencyViewer_ColumnID_Type)
	{
		Text = Item->AssetData.AssetClassPath.GetAssetName().ToString();
	}
	else if (ColumnName == DependencyViewer_ColumnID_Path)
	{
		Text = Item->AssetData.GetSoftObjectPath().ToString();
	}
	else if (ColumnName == DependencyViewer_ColumnID_MemorySize)
	{
		
	}
	else if (ColumnName == DependencyViewer_ColumnID_DiskSize)
	{
		
	}
	else if (ColumnName == DependencyViewer_ColumnID_DependencyCount)
	{
	
	}
	else if (ColumnName == DependencyViewer_ColumnID_DependencyDepth)
	{
		
	}

	return SNew(SBorder)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Text))
	]
	.OnMouseButtonDown(this, &ThisClass::HandleClicked)
	.OnMouseDoubleClick(this, &ThisClass::HandleDoubleClicked);
}

FReply SAssetDependencyViewerRowWidget::HandleClicked(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	if (PointerEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		AuditViewer->CreateContextMenu(Item, PointerEvent);
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

FReply SAssetDependencyViewerRowWidget::HandleDoubleClicked(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	AuditViewer->OpenSizeMap(Item);

	return FReply::Handled();
}

void SAssetDependencyViewerWidget::Construct(const FArguments& InArgs)
{
	using namespace UE::AssetValidation;
	
	CommandList = MakeShared<FUICommandList>();
	CommandList->MapAction(FGenericCommands::Get().Delete, FExecuteAction::CreateSP(this, &ThisClass::RemoveSelectedEntries));
	CommandList->MapAction(FGenericCommands::Get().Copy, FExecuteAction::CreateSP(this, &ThisClass::CopySelectedEntries));

	// header widget
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow)
	+ SHeaderRow::Column(DependencyViewer_ColumnID_Name)
	.DefaultLabel(FText::FromName(DependencyViewer_ColumnID_Name))
	.OnSort(FOnSortModeChanged::CreateSP(this, &ThisClass::SortColumn))
	+ SHeaderRow::Column(DependencyViewer_ColumnID_MemorySize)
	.DefaultLabel(FText::FromName(DependencyViewer_ColumnID_MemorySize))
	.OnSort(FOnSortModeChanged::CreateSP(this, &ThisClass::SortColumn))
	+ SHeaderRow::Column(DependencyViewer_ColumnID_DiskSize)
	.DefaultLabel(FText::FromName(DependencyViewer_ColumnID_DiskSize))
	.OnSort(FOnSortModeChanged::CreateSP(this, &ThisClass::SortColumn))
	+ SHeaderRow::Column(DependencyViewer_ColumnID_Type)
	.DefaultLabel(FText::FromName(DependencyViewer_ColumnID_Type))
	.OnSort(FOnSortModeChanged::CreateSP(this, &ThisClass::SortColumn))
	+ SHeaderRow::Column(DependencyViewer_ColumnID_Path)
	.DefaultLabel(FText::FromName(DependencyViewer_ColumnID_Path))
	.OnSort(FOnSortModeChanged::CreateSP(this, &ThisClass::SortColumn))
	+ SHeaderRow::Column(DependencyViewer_ColumnID_DependencyCount)
	.DefaultLabel(FText::FromName(DependencyViewer_ColumnID_DependencyCount))
	.OnSort(FOnSortModeChanged::CreateSP(this, &ThisClass::SortColumn))
	+ SHeaderRow::Column(DependencyViewer_ColumnID_DependencyDepth)
	.DefaultLabel(FText::FromName(DependencyViewer_ColumnID_DependencyDepth))
	.OnSort(FOnSortModeChanged::CreateSP(this, &ThisClass::SortColumn));

	TSharedPtr<SHorizontalBox> Actions = SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).AutoWidth().Padding(5.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("AVDependencyViewer_RunTitle", "Run"))
		.ToolTipText(LOCTEXT("AVDependencyViewer_RunToolTip", ""))
		.OnClicked(this, &ThisClass::Run)
	]
	+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).AutoWidth().Padding(5.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("AVDependencyViewer_Clear", "Clear"))
		.ToolTipText(LOCTEXT("AVDependencyViewer_ClearToolTip", ""))
		.OnClicked(this, &ThisClass::Clear)
	]
	+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).AutoWidth().Padding(5.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("AVDependencyViewer_ExportTitle", "Export To CSV"))
		.ToolTipText(LOCTEXT("AVDependencyViewer_ExportToolTip", "Exports data (if any is listed) and brings up directory where CSV file is stored (overriding any existing file)"))
		.OnClicked(this, &ThisClass::Export)
	];
	

	TSharedPtr<SVerticalBox> Body = SNew(SVerticalBox)
	// Actions
	+ SVerticalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).AutoHeight()
	[
		Actions.ToSharedRef()
	]
#if 0
	// Navigation Path
	+ SVerticalBox::Slot()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Fill)
	.Padding(2, 0, 0, 0)
	[
		SNew(SNavigationBar)
		.OnPathClicked(this, &ThisClass::OnPathClicked)
		.HasPathMenuContent(this, &ThisClass::OnHasCrumbDelimiterContent)
		.GetPathMenuContent(this, &ThisClass::OnGetCrumbDelimiterContent)
		.GetComboOptions(this, &ThisClass::GetRecentPaths)
		.OnNavigateToPath(this, &ThisClass::OnNavigateToPath)
		.OnCompletePrefix(this, &ThisClass::OnCompletePathPrefix)
		.OnCanEditPathAsText(this, &ThisClass::OnCanEditPathAsText)
		.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserPath")))
	]
#endif
	+ SVerticalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Top).AutoHeight()
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SAssignNew(ListView, SListView<TSharedPtr<FAssetAuditResult>>)
			.HeaderRow(Header)
			.ListItemsSource(&ListItems)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &ThisClass::HandleGenerateListItem)
		]
	];

	ChildSlot
	[
		Body.ToSharedRef()
	];
}

FReply SAssetDependencyViewerWidget::Run()
{
	return FReply::Unhandled();
}

FReply SAssetDependencyViewerWidget::Clear()
{
	return FReply::Unhandled();	
}

FReply SAssetDependencyViewerWidget::Export()
{
	return FReply::Unhandled();
}

TSharedRef<ITableRow> SAssetDependencyViewerWidget::HandleGenerateListItem(TSharedPtr<FAssetAuditResult> Item, const TSharedRef<STableViewBase>& InOwnerTable)
{
	return SNew(SAssetDependencyViewerRowWidget)
	.Item(Item)
	.AuditViewer(SharedThis(this));
}


void SAssetDependencyViewerWidget::CreateContextMenu(TSharedPtr<FAssetAuditResult> ClickedItem, const FPointerEvent& PointerEvent)
{
	
}

void SAssetDependencyViewerWidget::OpenSizeMap(TSharedPtr<FAssetAuditResult> ClickedItem)
{
	IAssetManagerEditorModule::Get().OpenSizeMapUI(TArray{ClickedItem->AssetData.PackageName});
}

void SAssetDependencyViewerWidget::SortColumn(EColumnSortPriority::Type Priority, const FName& Name, EColumnSortMode::Type SortMode)
{
	
}

void SAssetDependencyViewerWidget::CopySelectedEntries()
{
	
}

void SAssetDependencyViewerWidget::RemoveSelectedEntries()
{
	
}

#undef LOCTEXT_NAMESPACE