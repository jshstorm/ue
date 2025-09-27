#include "ReferenceBuilder.h"

#include "Reference.h"
//#include "Dom/JsonObject.h"

void UReferenceBuilder::URefGlobalHandler(const FXmlNode* root)
{
	IterateNodes(root, [this](const FXmlNode* child) {
		URefGlobal* reference = URefGlobal::StaticParse(child);
		_globals.Emplace(*reference->Key, reference->Value);
	});
}
