#include "spammers.h"

void spammers::clan_tag()
{
	auto apply = [](const char* tag) -> void
		{
			using Fn = int(__fastcall*)(const char*, const char*);
			static auto fn = reinterpret_cast<Fn>(util::FindSignature(crypt_str("engine.dll"), crypt_str("53 56 57 8B DA 8B F9 FF 15")));

			fn(tag, tag);
		};

	static auto removed = false;

	if (!cfg.misc.clantag_spammer && !removed)
	{
		removed = true;
		apply(crypt_str(""));
		return;
	}

	if (cfg.misc.clantag_spammer)
	{
		auto nci = m_engine()->GetNetChannelInfo();

		if (!nci)
			return;

		static auto time = 0;

		auto ticks = TIME_TO_TICKS(nci->GetAvgLatency(FLOW_OUTGOING)) + (float)m_globals()->m_tickcount;
		auto intervals = 0.4f / m_globals()->m_intervalpertick;

		auto main_time = (int)(ticks / intervals) % 24;

		if (main_time != time && !m_clientstate()->iChokedCommands)
		{
			const char* frames[] = {
				crypt_str(">_____"),
				crypt_str("L>____"),
				crypt_str("La>___"),
				crypt_str("Lam>__"),
				crypt_str("Lamb>_"),
				crypt_str("Lambd>"),
				crypt_str("Lambda"),
				crypt_str(">ambda"),
				crypt_str("_>mbda"),
				crypt_str("__>bda"),
				crypt_str("___>da"),
				crypt_str("____>a"),
				crypt_str("_____<"),
				crypt_str("____<a"),
				crypt_str("___<da"),
				crypt_str("__<bda"),
				crypt_str("_<mbda"),
				crypt_str("<ambda"),
				crypt_str("Lambda"),
				crypt_str("Lambd<"),
				crypt_str("Lamb<_"),
				crypt_str("Lam<__"),
				crypt_str("La<___"),
				crypt_str("L<____"),
			};

			apply(frames[main_time]);
			time = main_time;
		}

		removed = false;
	}
}