
Some of my personal notes.

This file is not part of Nuklear! It's specific to fork at:
https://github.com/sleeptightAnsiC/Nuklear/tree/personal

TODOs:
- I should proably investigate this: https://github.com/Immediate-Mode-UI/Nuklear/pull/779#discussion_r1966566010
- Investigate this: https://github.com/Immediate-Mode-UI/Nuklear/pull/779#discussion_r1965047664

Things that I need to upstream:
- fix for: https://github.com/Immediate-Mode-UI/Nuklear/pull/779#discussion_r2331856071
- fix for broken static asserts about sizeof(nk_bool) in src/nuklear.h and src/nuklear_internal.h
- fix nk_do_propetry using nk_strtod (function) instead of NK_STRTOD (macro) in src/nuklear_property.c
- fix for build failure mentioned in: https://github.com/Immediate-Mode-UI/Nuklear/issues/256#issuecomment-2949725477
- (later perhaps) some feedback to: https://github.com/Immediate-Mode-UI/Nuklear/pull/779

Changes (probably won't fit with upstream):
- Added NK_CONFIG_FILE which let's you inject the header into [Nuklear]/src/nuklear.h
  This is usefull when you want to have config without using Nuklear as single-header-lib
  Not so usefull when using Nuklear as single-header-lib or when including it in only one file.
- src/stb_truetype.h and src/stb_rect_pack.h pull huge amount of libc/libm symbols and NK_* macros do not overwrite those

