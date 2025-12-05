
Some of my personal notes.

This file is not part of Nuklear! It's specific to fork at:
https://github.com/sleeptightAnsiC/Nuklear/tree/personal

Things that I need to upstream/report:
- crash: run any demo with configurator included > Configurator > Property > Left/Right Symbol > click > demo: ../../nuklear.h:6923: nk_strlen: Assertion `str' failed.

Changes (probably won't fit in upstream, but worth considering):
- Added NK_CONFIG_FILE which let's you inject macro definitions into [Nuklear]/src/nuklear.h
  This is usefull for having build configs without relaying on features specific to compiler and/or build system.
  Not so usefull when using Nuklear as single-header-lib or when including it in only one file.
  Note that C standard supports '#include FOO' where 'FOO' is a preprocesor macro,
  but it seems that some compilers (ekhemn... MSVC... ekhemn...) don't support it.
  I've seen this trick being used in freetype2 and they encountered the very same problem:
  https://github.com/freetype/freetype/blob/30e45abe939d7c2cbdf268f277c293400096868c/docs/INSTALL.ANY#L11-L19
  At first, I wanted to use '#include FOO' trick too, but looking for hardcoded header name is way more portable.
- src/stb_truetype.h and src/stb_rect_pack.h pull huge amount of libc/libm symbols and NK_* macros do not overwrite those
  Note that Nuklear/demo/sdl3_renderer/main.c already has a hack for it, which I recently contributed.

