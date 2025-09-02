// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "EdGraph/EdGraphNode.h"
#include "LDNode.h"
#include "Styling/SlateBrush.h"
#include "LDNode_Base.generated.h"

class ULDNode_Root;

class ANULEVELDESIGNEDITOR_API FNodePropertyObserver
{
public:
	virtual void OnPropertyChanged(class ULDNode_Base* Sender, const FName& PropertyName) = 0;
};

UCLASS(abstract)
class ANULEVELDESIGNEDITOR_API ULDNode_Base : public UEdGraphNode 
{
	GENERATED_BODY()

public:
	TArray<ULDNode_Base*> GetNodeChildren();
	ULDNode_Root* FindRootNode();
	void DestroyAllChildNodes();
	ULDNode_Base* GetParentNode();
	TArray<ULDNode_Base*> GetParentNodes();

	// Associated object used for nodes which are represented in model (runtime)
	UPROPERTY(VisibleAnywhere, Instanced, BlueprintReadWrite, Category = "RootInfo")
	class ULDNode* AssociatedObject;

	// Only user defined nodes can be deleted
	bool bUserDefined = true;

	// If node should show index orde
	bool bShowIndexOrder = true;

	// nodes which can cross-reference
	bool bIsNodeSelected;

protected:
	TSharedPtr<FSlateBrush> BoxBrush;

public:
	virtual void OnChangeAssociatedNode() {};
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e);
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual const FSlateBrush* GetNodeIcon() const;
	virtual FSlateColor GetNodeBackgroundColor() const;
	virtual const FSlateBrush* GetNodeBackgroundImage() const;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual UEdGraphPin* GetInputPin() const { return nullptr;  }
	virtual UEdGraphPin* GetOutputPin() const { return nullptr; }
	virtual void NodeConnectionListChanged() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual void DestroyNode() override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	// Clearing pasted node and prepare structure for registering new nodes
	virtual void PostPasteNode() override;
	// Adding and registering missing child nodes in the model
	virtual void PostPasteNodeFinal();
	virtual void PrepareForCopying() override;

	void SetTitle(FName NewTitle);
	void PostCopying();

private:
	bool HasAnyParentNodes();
	bool HasAnyChildNodes();
	void CreateModelChilds();
};