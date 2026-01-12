#include "custom_sounds.hpp"

__forceinline void setup_sounds()
{
	CreateDirectory("csgo\\sound", nullptr);
	FILE* file = nullptr;

	file = fopen(crypt_str("csgo\\sound\\body.wav"), crypt_str("wb"));
	fwrite(body, sizeof(unsigned char), 82506, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\phonk.wav"), crypt_str("wb"));
	fwrite(phonk, sizeof(unsigned char), 31784, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\rifk1.wav"), crypt_str("wb"));
	fwrite(rifk1, sizeof(unsigned char), 102600, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\primordial.wav"), crypt_str("wb"));
	fwrite(primordial, sizeof(unsigned char), 8190, file);
	fclose(file);
}