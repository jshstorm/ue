// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_LevelDesignProp.generated.h"

class ULDNode_Root;

// Action types
UENUM(BlueprintType)
enum class ELDActionType : uint8
{
	None,
	All,
};

/** Action to add a node to the graph */
USTRUCT()
struct ANULEVELDESIGNEDITOR_API FLevelDesignSchemaAction : public FEdGraphSchemaAction
{
	GENERATED_BODY();

	/** Template of node we want to create */
	UPROPERTY()
	class UEdGraphNode* NodeTemplate = nullptr;

	FLevelDesignSchemaAction()
		: FEdGraphSchemaAction()
		, NodeTemplate(NULL)
	{}

	FLevelDesignSchemaAction(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
		, NodeTemplate(NULL)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FEdGraphSchemaAction interface

};

UCLASS()
class UEdLevelDesignSchema : public UEdGraphSchema {
	GENERATED_BODY()

	// Begin EdGraphSchema interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	// End EdGraphSchema interface

public:
	void GetActionList(TArray<TSharedPtr<FEdGraphSchemaAction> >& OutActions, const UEdGraph* Graph, ELDActionType ActionType) const;

private:
	// Finding root and setting Outer to the Asset
	ULDNode_Root* GetRootNode(const UEdGraph* Graph) const;

	UPROPERTY()
	TArray<FEdGraphSchemaAction> ActionsCached;

};

class ANULEVELDESIGNEDITOR_API LevelDesignSchemaUtils {
public:
	template<typename T>
	static void AddAction(FString Title, FString Tooltip, TArray<TSharedPtr<FEdGraphSchemaAction> >& OutActions, UObject* Owner) 
	{
		const FText MenuDesc = FText::FromString(Title);
		const FText Category = FText::FromString(TEXT("LevelDesign"));
		TSharedPtr<FLevelDesignSchemaAction> NewActorNodeAction = AddNewNodeAction(OutActions, Category, MenuDesc, Tooltip);
		T* ActorNode = NewObject<T>(Owner);
		NewActorNodeAction->NodeTemplate = ActorNode;
	}

	static void AddActionWithClass(UClass* ClassPtr, FString Title, FString Tooltip, TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, UObject* Owner)
	{
		const FText MenuDesc = FText::FromString(Title);
		const FText Category = FText::FromString(TEXT("LevelDesign"));
		TSharedPtr<FLevelDesignSchemaAction> NewActorNodeAction = AddNewNodeAction(OutActions, Category, MenuDesc, Tooltip);
		UEdGraphNode* ActorNode = NewObject<UEdGraphNode>(Owner, ClassPtr);
		NewActorNodeAction->NodeTemplate = ActorNode;
	}

	static TSharedPtr<FLevelDesignSchemaAction> AddNewNodeAction(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, const FText& Category, const FText& MenuDesc, const FString& Tooltip)
	{
		TSharedPtr<FLevelDesignSchemaAction> NewAction = TSharedPtr<FLevelDesignSchemaAction>(new FLevelDesignSchemaAction(Category, MenuDesc, FText::FromString(Tooltip), 0));

		OutActions.Add(NewAction);
		return NewAction;
	}
};