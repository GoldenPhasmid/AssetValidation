#pragma once

#include "Widgets/Views/STableRow.h"
#include "AssetDependencyTree.h"

class SAssetDependencyViewerWidget;
class SAssetDependencyViewerRowWidget;

/**
 *
 */
class SAssetDependencyViewerRowWidget: public SMultiColumnTableRow<TSharedPtr<FAssetAuditResult>>
{
	using ThisClass = SAssetDependencyViewerRowWidget;
public:

	SLATE_BEGIN_ARGS(SAssetDependencyViewerRowWidget) {}
		SLATE_ARGUMENT(TSharedPtr<SAssetDependencyViewerWidget>, AuditViewer)
		SLATE_ARGUMENT(TSharedPtr<FAssetAuditResult>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	
	FReply HandleClicked(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent);
	FReply HandleDoubleClicked(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent);
	
	TSharedPtr<SAssetDependencyViewerWidget> AuditViewer;
	TSharedPtr<FAssetAuditResult> Item;
};

/**
 *
 */
class SAssetDependencyViewerWidget: public SCompoundWidget
{
	using ThisClass = SAssetDependencyViewerWidget;
	friend class SAssetDependencyViewerRowWidget;
public:
	SLATE_BEGIN_ARGS(SAssetDependencyViewerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	FReply Run();
	FReply Clear();
	FReply Export();
	
	TSharedRef<ITableRow> HandleGenerateListItem(TSharedPtr<FAssetAuditResult> Item, const TSharedRef<STableViewBase>& InOwnerTable);
	
	void SortColumn(EColumnSortPriority::Type Priority, const FName& Name, EColumnSortMode::Type SortMode);
	void CreateContextMenu(TSharedPtr<FAssetAuditResult> ClickedItem, const FPointerEvent& PointerEvent);

	void OpenAsset(TSharedPtr<FAssetAuditResult> ClickedItem);
	void OpenSizeMap(TSharedPtr<FAssetAuditResult> ClickedItem);
	
	
	void CopySelectedEntries();
	void RemoveSelectedEntries();
	
	TArray<TSharedPtr<FAssetAuditResult>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FAssetAuditResult>>> ListView;
	TSharedPtr<FUICommandList> CommandList;
};
