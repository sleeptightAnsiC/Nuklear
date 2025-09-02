
This file is specific to Nuklear fork at: https://github.com/sleeptightAnsiC/Nuklear/tree/personal

Things that I need to upstream:
- fix for broken static asserts about sizeof(nk_bool) in src/nuklear.h and src/nuklear_internal.h
- fix nk_do_propetry using nk_strtod (function) instead of NK_STRTOD (macro) in src/nuklear_property.c
- fix for build failure mentioned in: https://github.com/Immediate-Mode-UI/Nuklear/issues/256#issuecomment-2949725477
- (later perhaps) some feedback to: https://github.com/Immediate-Mode-UI/Nuklear/pull/825

