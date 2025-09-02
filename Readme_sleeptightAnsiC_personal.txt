
Some of my personal notes.
This file is not part of Nuklear! It's specific to fork at:
https://github.com/sleeptightAnsiC/Nuklear/tree/personal

I use '//' (C++/C99 style) comments for each of my FIXME/NOTE
so it will be very clear when reading the code which comments are mine.

Things that I need to upstream:
- fix for broken static asserts about sizeof(nk_bool) in src/nuklear.h and src/nuklear_internal.h
- fix nk_do_propetry using nk_strtod (function) instead of NK_STRTOD (macro) in src/nuklear_property.c
- fix for build failure mentioned in: https://github.com/Immediate-Mode-UI/Nuklear/issues/256#issuecomment-2949725477
- (later perhaps) some feedback to: https://github.com/Immediate-Mode-UI/Nuklear/pull/825

Changes (probably won't fit with upstream):
- Added NK_CONFIG_FILE which let's you inject the header into [Nuklear]/src/nuklear.h
  This is usefull when you want to have config without using Nuklear as single-header-lib
  Not so usefull when using Nuklear as single-header-lib or when including it in only one file.
- sdl3_renderer requires those defines in order to work:
    #define NK_INCLUDE_STANDARD_VARARGS
    #define NK_INCLUDE_DEFAULT_ALLOCATOR
    #define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
    #define NK_INCLUDE_FONT_BAKING
    #define NK_INCLUDE_DEFAULT_FONT
  Maybe I could rewrite this demo in order to be less dependent on those features (?)
  Either way, I've added preprocessor errors in order to catch those

