// Single translation unit for stb_image GIF decoding.
// Uses STB_IMAGE_STATIC so all STB functions are file-local (static), preventing
// any symbol collisions with SFML in static library builds, while providing
// a clean C++ wrapper API (STBGif) for AssetManager across all platforms and CI builds.

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_GIF
#define STBI_NO_STDIO
#define STBI_NO_FAILURE_STRINGS

#include "stb_image.h"

namespace STBGif {

unsigned char* loadGifFromMemory(const unsigned char* buffer, int len,
                                 int** delays, int* x, int* y, int* z,
                                 int* comp, int req_comp) {
    return stbi_load_gif_from_memory(buffer, len, delays, x, y, z, comp, req_comp);
}

void freeGifData(void* ptr) {
    stbi_image_free(ptr);
}

} // namespace STBGif
