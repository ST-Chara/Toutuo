#include "serverbrowser_ping_cache.h"

#include <engine/console.h>
#include <engine/sqlite.h>

#include <sqlite3.h>

#include <algorithm>
#include <vector>

class CServerBrowserPingCache : public IServerBrowserPingCache
{
public:
	CServerBrowserPingCache(IConsole *pConsole, IStorage *pStorage);
	virtual ~CServerBrowserPingCache() = default;

	void Load() override;

	void CachePing(NETADDR Addr, int Ping) override;
	void GetPingCache(const CEntry **ppEntries, int *pNumEntries) override;

private:
	IConsole *m_pConsole;

	CSqlite m_pDisk;
	CSqliteStmt m_pLoadStmt;
	CSqliteStmt m_pStoreStmt;

	std::vector<CEntry> m_vEntries;
	std::vector<CEntry> m_vNewEntries;
};

CServerBrowserPingCache::CServerBrowserPingCache(IConsole *pConsole, IStorage *pStorage) :
	m_pConsole(pConsole)
{
	m_pDisk = SqliteOpen(pConsole, pStorage, "ddnet-cache.sqlite3");
	if(!m_pDisk)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "serverbrowse_ping_cache", "failed to open ddnet-cache.sqlite3");
		return;
	}
	sqlite3 *pSqlite = m_pDisk.get();
	static const char TABLE[] = "CREATE TABLE IF NOT EXISTS server_pings (ip_address TEXT PRIMARY KEY NOT NULL, ping INTEGER NOT NULL, utc_timestamp TEXT NOT NULL)";
	if(SQLITE_HANDLE_ERROR(sqlite3_exec(pSqlite, TABLE, nullptr, nullptr, nullptr)))
	{
		m_pDisk = nullptr;
		pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "serverbrowse_ping_cache", "failed to create server_pings table");
		return;
	}
	m_pLoadStmt = SqlitePrepare(pConsole, pSqlite, "SELECT ip_address, ping FROM server_pings");
	m_pStoreStmt = SqlitePrepare(pConsole, pSqlite, "INSERT OR REPLACE INTO server_pings (ip_address, ping, utc_timestamp) VALUES (?, ?, datetime('now'))");
}

void CServerBrowserPingCache::Load()
{
	if(m_pDisk)
	{
		int PrevNewEntriesSize = m_vNewEntries.size();

		sqlite3 *pSqlite = m_pDisk.get();
		IConsole *pConsole = m_pConsole;
		bool Error = false;
		bool WarnedForBadAddress = false;
		Error = Error || !m_pLoadStmt;
		while(!Error)
		{
			int StepResult = SQLITE_HANDLE_ERROR(sqlite3_step(m_pLoadStmt.get()));
			if(StepResult == SQLITE_DONE)
			{
				break;
			}
			else if(StepResult == SQLITE_ROW)
			{
				const char *pIpAddress = (const char *)sqlite3_column_text(m_pLoadStmt.get(), 0);
				int Ping = sqlite3_column_int(m_pLoadStmt.get(), 1);
				NETADDR Addr;
				if(net_addr_from_str(&Addr, pIpAddress))
				{
					if(!WarnedForBadAddress)
					{
						char aBuf[64];
						str_format(aBuf, sizeof(aBuf), "invalid address: %s", pIpAddress);
						pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "serverbrowse_ping_cache", aBuf);
						WarnedForBadAddress = true;
					}
					continue;
				}
				m_vNewEntries.push_back(CEntry{Addr, Ping});
			}
			else
			{
				Error = true;
			}
		}
		if(Error)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "serverbrowse_ping_cache", "failed to load ping cache");
			m_vNewEntries.resize(PrevNewEntriesSize);
		}
	}
}

void CServerBrowserPingCache::CachePing(NETADDR Addr, int Ping)
{
	Addr.port = 0;
	m_vNewEntries.push_back(CEntry{Addr, Ping});
	if(m_pDisk)
	{
		sqlite3 *pSqlite = m_pDisk.get();
		IConsole *pConsole = m_pConsole;
		char aAddr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aAddr, sizeof(aAddr), false);

		bool Error = false;
		Error = Error || !m_pStoreStmt;
		Error = Error || SQLITE_HANDLE_ERROR(sqlite3_reset(m_pStoreStmt.get())) != SQLITE_OK;
		Error = Error || SQLITE_HANDLE_ERROR(sqlite3_bind_text(m_pStoreStmt.get(), 1, aAddr, -1, SQLITE_STATIC)) != SQLITE_OK;
		Error = Error || SQLITE_HANDLE_ERROR(sqlite3_bind_int(m_pStoreStmt.get(), 2, Ping)) != SQLITE_OK;
		Error = Error || SQLITE_HANDLE_ERROR(sqlite3_step(m_pStoreStmt.get())) != SQLITE_DONE;
		if(Error)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "serverbrowse_ping_cache", "failed to store ping");
		}
	}
}

void CServerBrowserPingCache::GetPingCache(const CEntry **ppEntries, int *pNumEntries)
{
	if(!m_vNewEntries.empty())
	{
		class CAddrComparer
		{
		public:
			bool operator()(const CEntry &a, const CEntry &b)
			{
				return net_addr_comp(&a.m_Addr, &b.m_Addr) < 0;
			}
		};
		std::vector<CEntry> vOldEntries;
		std::swap(m_vEntries, vOldEntries);

		// Remove duplicates, keeping newer ones.
		std::stable_sort(m_vNewEntries.begin(), m_vNewEntries.end(), CAddrComparer());
		{
			unsigned To = 0;
			for(unsigned int From = 0; From < m_vNewEntries.size(); From++)
			{
				if(To < From)
				{
					m_vNewEntries[To] = m_vNewEntries[From];
				}
				if(From + 1 >= m_vNewEntries.size() ||
					net_addr_comp(&m_vNewEntries[From].m_Addr, &m_vNewEntries[From + 1].m_Addr) != 0)
				{
					To++;
				}
			}
			m_vNewEntries.resize(To);
		}
		// Only keep the new entries where there are duplicates.
		m_vEntries.reserve(m_vNewEntries.size() + vOldEntries.size());
		{
			unsigned i = 0;
			unsigned j = 0;
			while(i < vOldEntries.size() && j < m_vNewEntries.size())
			{
				int Cmp = net_addr_comp(&vOldEntries[i].m_Addr, &m_vNewEntries[j].m_Addr);
				if(Cmp != 0)
				{
					if(Cmp < 0)
					{
						m_vEntries.push_back(vOldEntries[i]);
						i++;
					}
					else
					{
						m_vEntries.push_back(m_vNewEntries[j]);
						j++;
					}
				}
				else
				{
					// Ignore the old element if we have both.
					i++;
				}
			}
			// Add the remaining elements.
			for(; i < vOldEntries.size(); i++)
			{
				m_vEntries.push_back(vOldEntries[i]);
			}
			for(; j < m_vNewEntries.size(); j++)
			{
				m_vEntries.push_back(m_vNewEntries[j]);
			}
		}
		m_vNewEntries.clear();
	}
	*ppEntries = m_vEntries.data();
	*pNumEntries = m_vEntries.size();
}

IServerBrowserPingCache *CreateServerBrowserPingCache(IConsole *pConsole, IStorage *pStorage)
{
	return new CServerBrowserPingCache(pConsole, pStorage);
}
