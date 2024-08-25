#pragma once

#include "Logging/TokenizedMessage.h"

class UEdGraphPin;
class UEdGraphNode;
class UBlueprint;

namespace UE::AssetValidation
{

class FBlueprintGraphToken: public IMessageToken
{
private:
	struct FPrivateToken {};

public:
	FBlueprintGraphToken(FPrivateToken, const UEdGraphPin* InPin, const UEdGraphNode* InNode, const UBlueprint* InBlueprint, const FText& FailReason);

	static TSharedRef<FBlueprintGraphToken> Create(const UEdGraphPin* Pin, const UBlueprint* Blueprint, const FText& FailReason);
	static TSharedRef<FBlueprintGraphToken> Create(const UEdGraphNode* Node, const UBlueprint* Blueprint, const FText& FailReason);

	//~Begin IMessageToken interface
	virtual EMessageToken::Type GetType() const override;
	virtual const FOnMessageTokenActivated& GetOnMessageTokenActivated() const override;
	//~End IMessageToken interface

	FORCEINLINE static FOnMessageTokenActivated& DefaultOnMessageTokenActivated()
	{
		return DefaultMessageTokenActivated;
	}

	void HighlightNode(const FText& Msg) const;

	const UEdGraphPin* GetPin() const;
	const UEdGraphNode* GetNode() const;
	const UBlueprint* GetBlueprint() const;

	FORCEINLINE const FText& GetFailReason() const { return FailReason; }
private:

	static FOnMessageTokenActivated DefaultMessageTokenActivated;

	FEdGraphPinReference Pin;
	TWeakObjectPtr<const UObject> Node;
	TWeakObjectPtr<const UObject> Blueprint;
	FText FailReason = FText::GetEmpty();

	static FOnMessageTokenActivated TokenActivated;
};
	
}
