#include "custom_sounds.hpp"
#include "hitsounds.hpp"
#include <sys/stat.h>

__forceinline void setup_sounds()
{
	CreateDirectory(crypt_str("csgo\\sound"), nullptr);

	char path[MAX_PATH];

	for (auto i = 0; i < hitsound_count; ++i)
	{
		auto entry = &hitsound_entries[i];

		sprintf_s(path, crypt_str("csgo\\sound\\%s"), entry->file);

		struct _stat info;

		if (_stat(path, &info) == 0 && info.st_size == (long)entry->size)
			continue;

		auto file = fopen(path, crypt_str("wb"));

		if (!file)
			continue;

		fwrite(entry->data, sizeof(unsigned char), entry->size, file);
		fclose(file);
	}
}
