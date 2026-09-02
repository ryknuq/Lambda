#pragma once

struct hitsound_entry
{
	const char* file;
	const unsigned char* data;
	unsigned int size;
};

extern const hitsound_entry hitsound_entries[];
extern const int hitsound_count;
