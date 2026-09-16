#include "NeovimCodeAccessorSettings.h"
#include "NeovimSourceCodeAccessor.h"

#define LOCTEXT_NAMESPACE "UNeovimCodeAccessorSettings"

UNeovimCodeAccessorSettings::UNeovimCodeAccessorSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("Neovim");

	NeovideExecutable = TEXT("neovide");
	NeovideRPCAddress = TEXT("");
}

#if WITH_EDITOR
FText UNeovimCodeAccessorSettings::GetSectionText() const
{
	return LOCTEXT("SettingsDisplayName", "Neovim");
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
