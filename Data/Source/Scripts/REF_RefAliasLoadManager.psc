Scriptname REF_RefAliasLoadManager Extends ReferenceAlias
{Handles loads between game sessions.}

Event OnPlayerLoadGame()

    (self.GetOwningQuest() As REF_QuestEventHandler).Maintenance()
EndEvent