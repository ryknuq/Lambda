#include "sound_player.hpp"
#include "hitsounds.hpp"

#define NOMINMAX

#include <Windows.h>
#include <xaudio2.h>
#include <mmsystem.h>

#include <algorithm>
#include <mutex>
#include <vector>

#pragma comment(lib, "winmm.lib")

namespace
{
	struct wav_data
	{
		WAVEFORMATEXTENSIBLE format;
		const unsigned char* samples;
		unsigned int sample_bytes;
	};

	struct pooled_voice
	{
		IXAudio2SourceVoice* voice;
		WAVEFORMATEX format;
	};

	std::mutex player_lock;
	std::vector<pooled_voice> player_voices;

	IXAudio2* player_engine = nullptr;
	IXAudio2MasteringVoice* player_master = nullptr;

	bool player_broken = false;

	bool parse_wav(const unsigned char* data, unsigned int size, wav_data& out)
	{
		if (!data || size < 44)
			return false;

		if (memcmp(data, "RIFF", 4) || memcmp(data + 8, "WAVE", 4))
			return false;

		memset(&out.format, 0, sizeof(out.format));

		out.samples = nullptr;
		out.sample_bytes = 0;

		auto offset = 12u;
		auto have_format = false;

		while (offset + 8u <= size)
		{
			unsigned int chunk_size;
			memcpy(&chunk_size, data + offset + 4, sizeof(chunk_size));

			const auto body = offset + 8u;

			if (chunk_size > size - body)
				chunk_size = size - body;

			if (!memcmp(data + offset, "fmt ", 4) && chunk_size >= 16u)
			{
				memcpy(&out.format, data + body, std::min(chunk_size, (unsigned int)sizeof(out.format)));

				if (chunk_size <= 16u)
					out.format.Format.cbSize = 0;

				have_format = true;
			}
			else if (!memcmp(data + offset, "data", 4))
			{
				out.samples = data + body;
				out.sample_bytes = chunk_size;
			}

			offset = body + chunk_size + (chunk_size & 1u);
		}

		return have_format && out.samples && out.sample_bytes > 0u;
	}

	bool ensure_engine()
	{
		if (player_engine && player_master)
			return true;

		if (player_broken)
			return false;

		player_broken = true;

		using create_t = HRESULT(__stdcall*)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);

		static const char* modules[] = { "XAudio2_9.dll", "XAudio2_8.dll" };

		create_t create = nullptr;

		for (auto name : modules)
		{
			auto module = LoadLibraryA(name);

			if (!module)
				continue;

			create = (create_t)GetProcAddress(module, "XAudio2Create");

			if (create)
				break;
		}

		if (!create)
			return false;

		if (FAILED(create(&player_engine, 0, XAUDIO2_DEFAULT_PROCESSOR)) || !player_engine)
		{
			player_engine = nullptr;
			return false;
		}

		if (FAILED(player_engine->CreateMasteringVoice(&player_master)) || !player_master)
		{
			player_engine->Release();

			player_engine = nullptr;
			player_master = nullptr;

			return false;
		}

		player_broken = false;
		return true;
	}

	bool same_format(const WAVEFORMATEX& a, const WAVEFORMATEX& b)
	{
		return a.wFormatTag == b.wFormatTag && a.nChannels == b.nChannels &&
			a.nSamplesPerSec == b.nSamplesPerSec && a.wBitsPerSample == b.wBitsPerSample &&
			a.nBlockAlign == b.nBlockAlign;
	}

	IXAudio2SourceVoice* acquire_voice(const WAVEFORMATEXTENSIBLE& format)
	{
		auto reusable = (IXAudio2SourceVoice*)nullptr;

		for (auto entry = player_voices.begin(); entry != player_voices.end(); )
		{
			XAUDIO2_VOICE_STATE state{};
			entry->voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

			if (state.BuffersQueued > 0)
			{
				++entry;
				continue;
			}

			if (!reusable && same_format(entry->format, format.Format))
			{
				reusable = entry->voice;
				++entry;
				continue;
			}

			if (player_voices.size() <= 8)
			{
				++entry;
				continue;
			}

			entry->voice->DestroyVoice();
			entry = player_voices.erase(entry);
		}

		if (reusable)
		{
			reusable->Stop(0);
			reusable->FlushSourceBuffers();

			return reusable;
		}

		if (player_voices.size() >= 16)
			return nullptr;

		auto voice = (IXAudio2SourceVoice*)nullptr;

		if (FAILED(player_engine->CreateSourceVoice(&voice, (const WAVEFORMATEX*)&format)) || !voice)
			return nullptr;

		player_voices.push_back({ voice, format.Format });

		return voice;
	}
}

void play_hitsound(int index, int volume)
{
	if (index < 0 || index >= hitsound_count)
		return;

	const auto entry = &hitsound_entries[index];

	if (!entry->data || !entry->size)
		return;

	const auto gain = std::clamp(volume, 0, 100) / 100.0f;

	if (gain <= 0.0f)
		return;

	wav_data wav;

	if (parse_wav(entry->data, entry->size, wav))
	{
		std::lock_guard<std::mutex> guard(player_lock);

		if (ensure_engine())
		{
			auto voice = acquire_voice(wav.format);

			if (voice)
			{
				XAUDIO2_BUFFER buffer{};

				buffer.AudioBytes = wav.sample_bytes;
				buffer.pAudioData = wav.samples;
				buffer.Flags = XAUDIO2_END_OF_STREAM;

				if (SUCCEEDED(voice->SubmitSourceBuffer(&buffer)))
				{
					voice->SetVolume(gain);

					if (SUCCEEDED(voice->Start(0)))
						return;
				}

				voice->Stop(0);
				voice->FlushSourceBuffers();
			}
		}
	}

	PlaySoundA((const char*)entry->data, nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

bool sound_preview_blocked = true;

void preview_hitsound(int menu_index, int volume)
{
	if (sound_preview_blocked)
		return;

	play_hitsound(menu_index - 1, volume);
}

const std::vector<std::string>& GetSoundNames()
{
	static const std::vector<std::string> names = {
		"None",
		"sparkle",
		"phonk",
		"rifk1",
		"primordial",
		"hit9",
		"ting",
		"click",
		"satisfying",
		"pop",
		"bonk",
		"kick",
		"bell",
		"bubble",
		"flush",
		"door",
		"water drop",
		"quaver",
		"combo break",
		"killcard",
		"arena switch",
		"rankdown",
		"zelda",
		"spiral knight",
		"regulus",
		"brotato",
		"trident",
		"minecraft hit",
		"minecraft old hit",
		"minecraft bow",
		"minecraft button",
		"minecraft egg",
		"minecraft xp",
		"pubg pan",
		"rust headshot",
		"cod",
		"fatality",
		"kill doof",
		"amongus kill",
		"aimbooster",
		"tavern",
		"money claim",
		"apple pay",
		"bameware",
		"msfrs",
		"stony",
		"ben",
		"mouthsound",
		"agpa 1",
		"agpa 2",
		"hentai 1",
		"hentai 2",
		"hentai 3"
	};

	return names;
}
