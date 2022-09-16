#include "NeovimCodeAccessorSettings.h"
#include "NeovimSourceCodeAccessor.h"

#define LOCTEXT_NAMESPACE "UNeovimCodeAccessorSettings"

UNeovimCodeAccessorSettings::UNeovimCodeAccessorSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("Neovim");

	RemoteExecutionURL = TEXT("127.0.0.1:12345");
	RemoteExecutionTerminal = TEXT("");
}

#if WITH_EDITOR

bool UNeovimCodeAccessorSettings::CanEditChange(const FProperty* InProperty) const
{
	bool bCanEditChange = Super::CanEditChange(InProperty);

	if (bCanEditChange && InProperty)
	{
		if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UNeovimCodeAccessorSettings, RemoteExecutionTerminal) ||
			InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UNeovimCodeAccessorSettings, RemoteExecutionTerminalOpts))
		{
			bCanEditChange &= bStartInTerminal;
		}
	}

	return bCanEditChange;
}

void UNeovimCodeAccessorSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UNeovimCodeAccessorSettings, bStartInTerminal)
			|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UNeovimCodeAccessorSettings, RemoteExecutionTerminal)
			|| PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UNeovimCodeAccessorSettings, RemoteExecutionTerminalOpts))
		{
		}
	}
}

FText UNeovimCodeAccessorSettings::GetSectionText() const
{
	return LOCTEXT("SettingsDisplayName", "Neovim");
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
