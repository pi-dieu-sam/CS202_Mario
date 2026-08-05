// SFML 2.6 already compiles stb_image internally (ImageLoader.cpp).
// We do NOT define STB_IMAGE_IMPLEMENTATION here — that would create
// multiply-defined symbol linker errors.  Instead we just forward-declare
// the one GIF-specific function we need and let the linker satisfy it from
// SFML's object files.  All other stbi_* symbols are also available the same
// way (they are already linked in via sfml-graphics).
//
// Note: stb_image.h is still included in AssetManager.cpp but with
// STBI_NO_IMPLEMENTATION defined to suppress the definitions — only the
// extern "C" declarations (the function prototypes) are needed here.
