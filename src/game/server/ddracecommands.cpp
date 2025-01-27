/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"

#include <engine/antibot.h>

#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/player.h>

bool CheckClientID(int ClientID);

void CGameContext::ConGoLeft(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, -1, 0);
}

void CGameContext::ConGoRight(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 1, 0);
}

void CGameContext::ConGoDown(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 0, 1);
}

void CGameContext::ConGoUp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 0, -1);
}

void CGameContext::ConMove(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, pResult->GetInteger(0),
		pResult->GetInteger(1));
}

void CGameContext::ConMoveRaw(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, pResult->GetInteger(0),
		pResult->GetInteger(1), true);
}

void CGameContext::MoveCharacter(int ClientID, int X, int Y, bool Raw)
{
	CCharacter *pChr = GetPlayerChar(ClientID);

	if(!pChr)
		return;

	pChr->Core()->m_Pos.x += ((Raw) ? 1 : 32) * X;
	pChr->Core()->m_Pos.y += ((Raw) ? 1 : 32) * Y;
	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConEndlessHook(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
	if(pChr)
	{
		pChr->SetEndlessHook(true);
	}
}

void CGameContext::ConUnEndlessHook(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
	if(pChr)
	{
		pChr->SetEndlessHook(false);
	}
}

void CGameContext::ConJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
	if(pChr)
		pChr->m_Jetpack = true;
}

void CGameContext::ConUnJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
	if(pChr)
		pChr->m_Jetpack = false;
}

void CGameContext::Teleport(CCharacter *pChr, vec2 Pos)
{
	pChr->Core()->m_Pos = Pos;
	pChr->m_Pos = Pos;
	pChr->m_PrevPos = Pos;
	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConToTeleporter(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	unsigned int TeleTo = pResult->GetInteger(0);
	CGameControllerDDRace *pGameControllerDDRace = (CGameControllerDDRace *)pSelf->m_pController;

	if(!pGameControllerDDRace->m_TeleOuts[TeleTo - 1].empty())
	{
		CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
		if(pChr)
		{
			int TeleOut = pSelf->m_World.m_Core.RandomOr0(pGameControllerDDRace->m_TeleOuts[TeleTo - 1].size());
			pSelf->Teleport(pChr, pGameControllerDDRace->m_TeleOuts[TeleTo - 1][TeleOut]);
		}
	}
}

void CGameContext::ConToCheckTeleporter(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	unsigned int TeleTo = pResult->GetInteger(0);
	CGameControllerDDRace *pGameControllerDDRace = (CGameControllerDDRace *)pSelf->m_pController;

	if(!pGameControllerDDRace->m_TeleCheckOuts[TeleTo - 1].empty())
	{
		CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
		if(pChr)
		{
			int TeleOut = pSelf->m_World.m_Core.RandomOr0(pGameControllerDDRace->m_TeleCheckOuts[TeleTo - 1].size());
			pSelf->Teleport(pChr, pGameControllerDDRace->m_TeleCheckOuts[TeleTo - 1][TeleOut]);
			pChr->m_TeleCheckpoint = TeleTo;
		}
	}
}

void CGameContext::ConTeleport(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Tele = pResult->NumArguments() == 2 ? pResult->GetInteger(0) : pResult->m_ClientID;
	int TeleTo = pResult->NumArguments() ? pResult->GetInteger(pResult->NumArguments() - 1) : pResult->m_ClientID;
	int AuthLevel = pSelf->Server()->GetAuthedState(pResult->m_ClientID);

	if(Tele != pResult->m_ClientID && AuthLevel < g_Config.m_SvTeleOthersAuthLevel)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tele", "you aren't allowed to tele others");
		return;
	}

	CCharacter *pChr = pSelf->GetPlayerChar(Tele);
	if(pChr && pSelf->GetPlayerChar(TeleTo))
	{
		pSelf->Teleport(pChr, pSelf->m_apPlayers[TeleTo]->m_ViewPos);
	}
}

bool CGameContext::TryVoteMute(const NETADDR *pAddr, int Secs)
{
	// find a matching vote mute for this ip, update expiration time if found
	for(int i = 0; i < m_NumVoteMutes; i++)
	{
		if(net_addr_comp_noport(&m_aVoteMutes[i].m_Addr, pAddr) == 0)
		{
			m_aVoteMutes[i].m_Expire = Server()->Tick() + Secs * Server()->TickSpeed();
			return true;
		}
	}

	// nothing to update create new one
	if(m_NumVoteMutes < MAX_VOTE_MUTES)
	{
		m_aVoteMutes[m_NumVoteMutes].m_Addr = *pAddr;
		m_aVoteMutes[m_NumVoteMutes].m_Expire = Server()->Tick() + Secs * Server()->TickSpeed();
		m_NumVoteMutes++;
		return true;
	}
	// no free slot found
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemute", "vote mute array is full");
	return false;
}

bool CGameContext::VoteMute(const NETADDR *pAddr, int Secs, const char *pDisplayName, int AuthedID)
{
	if(!TryVoteMute(pAddr, Secs))
		return false;

	if(!pDisplayName)
		return true;

	char aBuf[128];
	str_format(aBuf, sizeof aBuf, "'%s' banned '%s' for %d seconds from voting.",
		Server()->ClientName(AuthedID), pDisplayName, Secs);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemute", aBuf);
	return true;
}

bool CGameContext::VoteUnmute(const NETADDR *pAddr, const char *pDisplayName, int AuthedID)
{
	for(int i = 0; i < m_NumVoteMutes; i++)
	{
		if(net_addr_comp_noport(&m_aVoteMutes[i].m_Addr, pAddr) == 0)
		{
			m_NumVoteMutes--;
			m_aVoteMutes[i] = m_aVoteMutes[m_NumVoteMutes];
			if(pDisplayName)
			{
				char aBuf[128];
				str_format(aBuf, sizeof aBuf, "'%s' unbanned '%s' from voting.",
					Server()->ClientName(AuthedID), pDisplayName);
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voteunmute", aBuf);
			}
			return true;
		}
	}
	return false;
}

bool CGameContext::TryMute(const NETADDR *pAddr, int Secs, const char *pReason, bool InitialChatDelay)
{
	// find a matching mute for this ip, update expiration time if found
	for(int i = 0; i < m_NumMutes; i++)
	{
		if(net_addr_comp_noport(&m_aMutes[i].m_Addr, pAddr) == 0)
		{
			const int NewExpire = Server()->Tick() + Secs * Server()->TickSpeed();
			if(NewExpire > m_aMutes[i].m_Expire)
			{
				m_aMutes[i].m_Expire = NewExpire;
				str_copy(m_aMutes[i].m_aReason, pReason, sizeof(m_aMutes[i].m_aReason));
				m_aMutes[i].m_InitialChatDelay = InitialChatDelay;
			}
			return true;
		}
	}

	// nothing to update create new one
	if(m_NumMutes < MAX_MUTES)
	{
		m_aMutes[m_NumMutes].m_Addr = *pAddr;
		m_aMutes[m_NumMutes].m_Expire = Server()->Tick() + Secs * Server()->TickSpeed();
		str_copy(m_aMutes[m_NumMutes].m_aReason, pReason, sizeof(m_aMutes[m_NumMutes].m_aReason));
		m_aMutes[m_NumMutes].m_InitialChatDelay = InitialChatDelay;
		m_NumMutes++;
		return true;
	}
	// no free slot found
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", "mute array is full");
	return false;
}

void CGameContext::Mute(const NETADDR *pAddr, int Secs, const char *pDisplayName, const char *pReason, bool InitialChatDelay)
{
	if(Secs <= 0)
		return;
	if(!TryMute(pAddr, Secs, pReason, InitialChatDelay))
		return;
	if(InitialChatDelay)
		return;
	if(!pDisplayName)
		return;

	char aBuf[128];
	if(pReason[0])
		str_format(aBuf, sizeof aBuf, "'%s' has been muted for %d seconds (%s)", pDisplayName, Secs, pReason);
	else
		str_format(aBuf, sizeof aBuf, "'%s' has been muted for %d seconds", pDisplayName, Secs);
	SendChat(-1, CHAT_ALL, aBuf);
}

void CGameContext::ConVoteMute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(Victim < 0 || Victim > MAX_CLIENTS || !pSelf->m_apPlayers[Victim])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemute", "Client ID not found");
		return;
	}

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	int Seconds = clamp(pResult->GetInteger(1), 1, 86400);
	bool Found = pSelf->VoteMute(&Addr, Seconds, pSelf->Server()->ClientName(Victim), pResult->m_ClientID);

	if(Found)
	{
		char aBuf[128];
		str_format(aBuf, sizeof aBuf, "'%s' banned '%s' for %d seconds from voting.",
			pSelf->Server()->ClientName(pResult->m_ClientID), pSelf->Server()->ClientName(Victim), Seconds);
		pSelf->SendChat(-1, 0, aBuf);
	}
}

void CGameContext::ConVoteUnmute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(Victim < 0 || Victim > MAX_CLIENTS || !pSelf->m_apPlayers[Victim])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voteunmute", "Client ID not found");
		return;
	}

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	bool Found = pSelf->VoteUnmute(&Addr, pSelf->Server()->ClientName(Victim), pResult->m_ClientID);
	if(Found)
	{
		char aBuf[128];
		str_format(aBuf, sizeof aBuf, "'%s' unbanned '%s' from voting.",
			pSelf->Server()->ClientName(pResult->m_ClientID), pSelf->Server()->ClientName(Victim));
		pSelf->SendChat(-1, 0, aBuf);
	}
}

void CGameContext::ConVoteMutes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pSelf->m_NumVoteMutes <= 0)
	{
		// Just to make sure.
		pSelf->m_NumVoteMutes = 0;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemutes",
			"There are no active vote mutes.");
		return;
	}

	char aIpBuf[64];
	char aBuf[128];
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemutes",
		"Active vote mutes:");
	for(int i = 0; i < pSelf->m_NumVoteMutes; i++)
	{
		net_addr_str(&pSelf->m_aVoteMutes[i].m_Addr, aIpBuf, sizeof(aIpBuf), false);
		str_format(aBuf, sizeof aBuf, "%d: \"%s\", %d seconds left", i,
			aIpBuf, (pSelf->m_aVoteMutes[i].m_Expire - pSelf->Server()->Tick()) / pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemutes", aBuf);
	}
}

void CGameContext::ConMute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Console()->Print(
		IConsole::OUTPUT_LEVEL_STANDARD,
		"mutes",
		"Use either 'muteid <client_id> <seconds> <reason>' or 'muteip <ip> <seconds> <reason>'");
}

// mute through client id
void CGameContext::ConMuteID(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(Victim < 0 || Victim > MAX_CLIENTS || !pSelf->m_apPlayers[Victim])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "muteid", "Client id not found.");
		return;
	}

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	const char *pReason = pResult->NumArguments() > 2 ? pResult->GetString(2) : "";

	pSelf->Mute(&Addr, clamp(pResult->GetInteger(1), 1, 86400),
		pSelf->Server()->ClientName(Victim), pReason);
}

// mute through ip, arguments reversed to workaround parsing
void CGameContext::ConMuteIP(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	NETADDR Addr;
	if(net_addr_from_str(&Addr, pResult->GetString(0)))
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
			"Invalid network address to mute");
	}
	const char *pReason = pResult->NumArguments() > 2 ? pResult->GetString(2) : "";
	pSelf->Mute(&Addr, clamp(pResult->GetInteger(1), 1, 86400), NULL, pReason);
}

// unmute by mute list index
void CGameContext::ConUnmute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Index = pResult->GetInteger(0);

	if(Index < 0 || Index >= pSelf->m_NumMutes)
		return;

	char aIpBuf[64];
	char aBuf[64];
	net_addr_str(&pSelf->m_aMutes[Index].m_Addr, aIpBuf, sizeof(aIpBuf), false);
	str_format(aBuf, sizeof(aBuf), "Unmuted %s", aIpBuf);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", aBuf);

	pSelf->m_NumMutes--;
	pSelf->m_aMutes[Index] = pSelf->m_aMutes[pSelf->m_NumMutes];
}

// unmute by player id
void CGameContext::ConUnmuteID(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(Victim < 0 || Victim > MAX_CLIENTS || !pSelf->m_apPlayers[Victim])
		return;

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	for(int i = 0; i < pSelf->m_NumMutes; i++)
	{
		if(net_addr_comp_noport(&pSelf->m_aMutes[i].m_Addr, &Addr) == 0)
		{
			char aIpBuf[64];
			char aBuf[64];
			net_addr_str(&pSelf->m_aMutes[i].m_Addr, aIpBuf, sizeof(aIpBuf), false);
			str_format(aBuf, sizeof(aBuf), "Unmuted %s", aIpBuf);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", aBuf);
			pSelf->m_NumMutes--;
			pSelf->m_aMutes[i] = pSelf->m_aMutes[pSelf->m_NumMutes];
			return;
		}
	}
}

// list mutes
void CGameContext::ConMutes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pSelf->m_NumMutes <= 0)
	{
		// Just to make sure.
		pSelf->m_NumMutes = 0;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
			"There are no active mutes.");
		return;
	}

	char aIpBuf[64];
	char aBuf[128];
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
		"Active mutes:");
	for(int i = 0; i < pSelf->m_NumMutes; i++)
	{
		net_addr_str(&pSelf->m_aMutes[i].m_Addr, aIpBuf, sizeof(aIpBuf), false);
		str_format(aBuf, sizeof aBuf, "%d: \"%s\", %d seconds left (%s)", i, aIpBuf,
			(pSelf->m_aMutes[i].m_Expire - pSelf->Server()->Tick()) / pSelf->Server()->TickSpeed(), pSelf->m_aMutes[i].m_aReason);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", aBuf);
	}
}

void CGameContext::ConModerate(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID))
		return;

	bool HadModerator = pSelf->PlayerModerating();

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	pPlayer->m_Moderating = !pPlayer->m_Moderating;

	char aBuf[256];
	if(!HadModerator && pPlayer->m_Moderating)
		str_format(aBuf, sizeof(aBuf), "Server kick/spec votes will now be actively moderated.");

	if(!pSelf->PlayerModerating())
		str_format(aBuf, sizeof(aBuf), "Server kick/spec votes are no longer actively moderated.");

	pSelf->SendChat(-1, CHAT_ALL, aBuf, 0);

	if(pPlayer->m_Moderating)
		pSelf->SendChatTarget(pResult->m_ClientID, "Active moderator mode enabled for you.");
	else
		pSelf->SendChatTarget(pResult->m_ClientID, "Active moderator mode disabled for you.");
}

void CGameContext::ConVoteNo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->ForceVote(pResult->m_ClientID, false);
}

void CGameContext::ConDumpAntibot(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Antibot()->Dump();
}
