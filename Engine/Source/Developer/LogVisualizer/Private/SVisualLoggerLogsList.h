// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class SVisualLoggerLogsList : public SVisualLoggerBaseWidget
{
public:
	SLATE_BEGIN_ARGS(SVisualLoggerLogsList){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& InCommandList);
	virtual ~SVisualLoggerLogsList();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	void ResetData();
	void ObjectSelectionChanged(const TArray<FName>& Selection);
	void OnItemSelectionChanged(const FVisualLoggerDBRow& BDRow, int32 ItemIndex);
	void OnFiltersChanged();
	void OnFiltersSearchChanged(const FText& Filter);

protected:
	FText GetFilterText() const;
	void RegenerateLogEntries();
	void GenerateLogs(const FVisualLogDevice::FVisualLogEntryItem& EntryItem, bool bGenerateHeader);
	void LogEntryLineSelectionChanged(TSharedPtr<FLogEntryItem> SelectedItem, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> LogEntryLinesGenerateRow(TSharedPtr<struct FLogEntryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

protected:
	TSharedPtr<SListView<TSharedPtr<struct FLogEntryItem> > > LogsLinesWidget;
	TArray<TSharedPtr<struct FLogEntryItem> > CachedLogEntryLines;
};
