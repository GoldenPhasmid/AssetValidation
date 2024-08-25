#include "BlueprintGraphToken.h"

#include "Engine/Blueprint.h"
#include "BlueprintEditor.h"
#include "AssetValidators/AssetValidator_BlueprintGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
FOnMessageTokenActivated FBlueprintGraphToken::DefaultMessageTokenActivated{};

struct FBlueprintGraphTokenMessageHandler
{
public:
	FBlueprintGraphTokenMessageHandler()
	{
		FBlueprintGraphToken::DefaultOnMessageTokenActivated().BindStatic(&HandleTokenActivated);
	}

private:
	
	static void HandleTokenActivated(const TSharedRef<IMessageToken>& InToken)
	{
		check(InToken->GetType() == EMessageToken::EdGraph);

		const TSharedRef<FBlueprintGraphToken> Token = StaticCastSharedRef<FBlueprintGraphToken>(InToken);

		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			UBlueprint* Blueprint = const_cast<UBlueprint*>(Token->GetBlueprint());
			AssetEditorSubsystem->OpenEditorForAsset(Blueprint, EToolkitMode::Standalone);
		
			if (IAssetEditorInstance* AssetEditor = AssetEditorSubsystem->FindEditorForAsset(Blueprint, true))
			{
				check(AssetEditor->GetEditorName() == TEXT("BlueprintEditor"));
				FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(AssetEditor);

				if (const UEdGraphPin* Pin = Token->GetPin())
				{
					UAssetValidator_BlueprintGraph::HighlightNode(Pin->GetOwningNode(), Token->GetFailReason(), true);
					BlueprintEditor->JumpToPin(Pin);
				}
				else if (const UEdGraphNode* Node = Token->GetNode())
				{
					BlueprintEditor->JumpToNode(Node);
					UAssetValidator_BlueprintGraph::HighlightNode(Node, Token->GetFailReason(), true);
				}
			}
		}
	}
	
};
	
TSharedRef<FBlueprintGraphToken> FBlueprintGraphToken::Create(const UEdGraphPin* Pin, const UBlueprint* Blueprint, const FText& FailReason)
{
	 return MakeShared<FBlueprintGraphToken>(FPrivateToken{}, Pin, Pin->GetOwningNode(), Blueprint, FailReason);
}

TSharedRef<FBlueprintGraphToken> FBlueprintGraphToken::Create(const UEdGraphNode* Node, const UBlueprint* Blueprint, const FText& FailReason)
{
	return MakeShared<FBlueprintGraphToken>(FPrivateToken{}, nullptr, Node, Blueprint, FailReason);
}

FBlueprintGraphToken::FBlueprintGraphToken(FPrivateToken, const UEdGraphPin* InPin, const UEdGraphNode* InNode, const UBlueprint* InBlueprint, const FText& FailReason)
	: Pin(InPin)
	, Node(InNode)
	, Blueprint(InBlueprint)
{
	static FBlueprintGraphTokenMessageHandler MessageHandler{};
	
	if (InPin != nullptr)
	{
		CachedText = InPin->GetDisplayName();
		if (CachedText.IsEmpty())
		{
			CachedText = LOCTEXT("UnnamedPin", "<Unnamed>");
		}
	}
	else if (InNode != nullptr)
	{
		CachedText = InNode->GetNodeTitle(ENodeTitleType::ListView);
	}
	else if (InBlueprint != nullptr)
	{
		CachedText = FBlueprintEditorUtils::GetFriendlyClassDisplayName(InBlueprint->GeneratedClass);
	}
}

void FBlueprintGraphToken::HighlightNode(const FText& Msg) const
{
	if (UEdGraphNode* GraphNode = const_cast<UEdGraphNode*>(GetNode()))
	{
		GraphNode->bHasCompilerMessage = true;
		GraphNode->ErrorType = EMessageSeverity::Error;
		GraphNode->ErrorMsg = Msg.ToString();
	}
}

EMessageToken::Type FBlueprintGraphToken::GetType() const
{
	return EMessageToken::EdGraph;
}

const FOnMessageTokenActivated& FBlueprintGraphToken::GetOnMessageTokenActivated() const
{
	if (MessageTokenActivated.IsBound())
	{
		return MessageTokenActivated;
	}
	else
	{
		return DefaultMessageTokenActivated;
	}
}

const UEdGraphPin* FBlueprintGraphToken::GetPin() const
{
	return Pin.Get();
}

const UEdGraphNode* FBlueprintGraphToken::GetNode() const
{
	return Cast<UEdGraphNode>(Node.Get());
}

const UBlueprint* FBlueprintGraphToken::GetBlueprint() const
{
	return Cast<UBlueprint>(Blueprint.Get());
}
	
} // UE::AssetValidation

#undef LOCTEXT_NAMESPACE