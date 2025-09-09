/*
 * Nuklear - 4.9.4 - public domain
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */

#ifndef NK_SDL3_RENDERER_H_
#define NK_SDL3_RENDERER_H_

#if SDL_MAJOR_VERSION < 3
#error "nuklear_sdl3_renderer requires at least SDL 3.0.0"
#endif

/* We have to redefine it because demos do not include any headers
 * This is the same default value as the one from "src/nuklear_internal.h" */
#ifndef NK_BUFFER_DEFAULT_INITIAL_SIZE
    #define NK_BUFFER_DEFAULT_INITIAL_SIZE (4*1024)
#endif

NK_API struct nk_context*   nk_sdl_init(SDL_Window *win, SDL_Renderer *renderer);
NK_API struct nk_font_atlas* nk_sdl_font_stash_begin(struct nk_context* ctx);
NK_API void                 nk_sdl_font_stash_end(struct nk_context* ctx);
NK_API int                  nk_sdl_handle_event(struct nk_context* ctx, SDL_Event *evt);
NK_API void                 nk_sdl_render(struct nk_context* ctx, enum nk_anti_aliasing);
NK_API void                 nk_sdl_shutdown(struct nk_context* ctx);
NK_API nk_handle            nk_sdl_userdata(struct nk_context* ctx);
NK_API void                 nk_sdl_set_userdata(struct nk_context* ctx, nk_handle userdata);
NK_API void                 nk_sdl_style_set_font_debug(struct nk_context* ctx);

#endif /* NK_SDL3_RENDERER_H_ */

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_SDL3_RENDERER_IMPLEMENTATION
#ifndef NK_SDL3_RENDERER_IMPLEMENTATION_ONCE
#define NK_SDL3_RENDERER_IMPLEMENTATION_ONCE

#ifndef NK_INCLUDE_COMMAND_USERDATA
#error "nuklear_sdl3 requires the NK_INCLUDE_COMMAND_USERDATA define"
#endif

struct nk_sdl_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture tex_null;
    SDL_Texture *font_tex;
};

struct nk_sdl_vertex {
    float position[2];
    float uv[2];
    float col[4];
};

struct nk_sdl {
    SDL_Window *win;
    SDL_Renderer *renderer;
    struct nk_user_font* font_debug;
    struct nk_sdl_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    nk_handle userdata;
};

NK_API nk_handle nk_sdl_userdata(struct nk_context* ctx) {
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    return sdl->userdata;
}

NK_API void nk_sdl_set_userdata(struct nk_context* ctx, nk_handle userdata) {
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    sdl->userdata = userdata;
}

NK_INTERN void *
nk_sdl_alloc(nk_handle user, void *old, nk_size size)
{
    NK_UNUSED(user);
    NK_UNUSED(old);
    return SDL_malloc(size);
}

NK_INTERN void
nk_sdl_free(nk_handle user, void *old)
{
    NK_UNUSED(user);
    SDL_free(old);
}

NK_GLOBAL const struct nk_allocator nk_sdl_allocator = {
    .userdata = NULL,
    .alloc = nk_sdl_alloc,
    .free = nk_sdl_free,
};

NK_INTERN void
nk_sdl_device_upload_atlas(struct nk_context* ctx, const void *image, int width, int height)
{
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);

    /* Clean up if the texture already exists. */
    if (sdl->ogl.font_tex != NULL) {
        SDL_DestroyTexture(sdl->ogl.font_tex);
        sdl->ogl.font_tex = NULL;
    }

    SDL_Texture *g_SDLFontTexture = SDL_CreateTexture(sdl->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    if (g_SDLFontTexture == NULL) {
        SDL_Log("error creating texture");
        return;
    }
    SDL_UpdateTexture(g_SDLFontTexture, NULL, image, 4 * width);
    SDL_SetTextureBlendMode(g_SDLFontTexture, SDL_BLENDMODE_BLEND);
    sdl->ogl.font_tex = g_SDLFontTexture;
}

NK_API void
nk_sdl_render(struct nk_context* ctx, enum nk_anti_aliasing AA)
{
    /* setup global state */
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);

    {
        SDL_Rect saved_clip;
        bool clipping_enabled;
        int vs = sizeof(struct nk_sdl_vertex);
        size_t vp = NK_OFFSETOF(struct nk_sdl_vertex, position);
        size_t vt = NK_OFFSETOF(struct nk_sdl_vertex, uv);
        size_t vc = NK_OFFSETOF(struct nk_sdl_vertex, col);

        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;
        struct nk_buffer vbuf, ebuf;

        /* fill converting configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION,    NK_FORMAT_FLOAT,                NK_OFFSETOF(struct nk_sdl_vertex, position)},
            {NK_VERTEX_TEXCOORD,    NK_FORMAT_FLOAT,                NK_OFFSETOF(struct nk_sdl_vertex, uv)},
            {NK_VERTEX_COLOR,       NK_FORMAT_R32G32B32A32_FLOAT,   NK_OFFSETOF(struct nk_sdl_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };
        NK_MEMSET(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_sdl_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
        config.tex_null = sdl->ogl.tex_null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;

        /* convert shapes into vertexes */
        nk_buffer_init(&vbuf, &nk_sdl_allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
        nk_buffer_init(&ebuf, &nk_sdl_allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
        nk_convert(&sdl->ctx, &sdl->ogl.cmds, &vbuf, &ebuf, &config);

        /* iterate over and execute each draw command */
        offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);

        clipping_enabled = SDL_RenderClipEnabled(sdl->renderer);
        SDL_GetRenderClipRect(sdl->renderer, &saved_clip);

        nk_draw_foreach(cmd, &sdl->ctx, &sdl->ogl.cmds)
        {
            if (!cmd->elem_count) continue;

            {
                SDL_Rect r;
                r.x = cmd->clip_rect.x;
                r.y = cmd->clip_rect.y;
                r.w = cmd->clip_rect.w;
                r.h = cmd->clip_rect.h;
                SDL_SetRenderClipRect(sdl->renderer, &r);
            }

            {
                const void *vertices = nk_buffer_memory_const(&vbuf);

                SDL_RenderGeometryRaw(sdl->renderer,
                        (SDL_Texture *)cmd->texture.ptr,
                        (const float*)((const nk_byte*)vertices + vp), vs,
                        (const SDL_FColor*)((const nk_byte*)vertices + vc), vs,
                        (const float*)((const nk_byte*)vertices + vt), vs,
                        (vbuf.needed / vs),
                        (void *) offset, cmd->elem_count, 2);

                offset += cmd->elem_count;
            }
        }

        SDL_SetRenderClipRect(sdl->renderer, &saved_clip);
        if (!clipping_enabled) {
            SDL_SetRenderClipRect(sdl->renderer, NULL);
        }

        nk_clear(&sdl->ctx);
        nk_buffer_clear(&sdl->ogl.cmds);
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }
}

static void
nk_sdl_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText();
    NK_UNUSED(usr);
    if (text) {
        nk_textedit_paste(edit, text, nk_strlen(text));
        SDL_free((void  *)text);
    }
}

static void
nk_sdl_clipboard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    NK_UNUSED(usr);
    if (len <= 0 || text == NULL) return;
    str = SDL_strndup(text, (size_t)len);
    if (str == NULL) return;
    SDL_SetClipboardText(str);
    SDL_free((void*)str);
}

NK_API struct nk_context*
nk_sdl_init(SDL_Window *win, SDL_Renderer *renderer)
{
    struct nk_sdl* sdl;
    NK_ASSERT(win);
    NK_ASSERT(renderer);
    sdl = SDL_malloc(sizeof(struct nk_sdl));
    NK_ASSERT(sdl);
    SDL_zerop(sdl);
    sdl->win = win;
    sdl->renderer = renderer;
    nk_init(&sdl->ctx, &nk_sdl_allocator, NULL);
    sdl->ctx.userdata = nk_handle_ptr((void*)sdl);
    sdl->ctx.clip.copy = nk_sdl_clipboard_copy;
    sdl->ctx.clip.paste = nk_sdl_clipboard_paste;
    sdl->ctx.clip.userdata = nk_handle_ptr(0);
    nk_buffer_init(&sdl->ogl.cmds, &nk_sdl_allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);
    return &sdl->ctx;
}

NK_API struct nk_font_atlas*
nk_sdl_font_stash_begin(struct nk_context* ctx)
{
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    nk_font_atlas_init(&sdl->atlas, &nk_sdl_allocator);
    nk_font_atlas_begin(&sdl->atlas);
    return &sdl->atlas;
}

NK_API void
nk_sdl_font_stash_end(struct nk_context* ctx)
{
    struct nk_sdl* sdl;
    const void *image; int w, h;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);
    image = nk_font_atlas_bake(&sdl->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    NK_ASSERT(image);
    nk_sdl_device_upload_atlas(&sdl->ctx, image, w, h);
    nk_font_atlas_end(&sdl->atlas, nk_handle_ptr(sdl->ogl.font_tex), &sdl->ogl.tex_null);
    if (sdl->atlas.default_font) {
        nk_style_set_font(&sdl->ctx, &sdl->atlas.default_font->handle);
    }
}

NK_API int
nk_sdl_handle_event(struct nk_context* ctx, SDL_Event *evt)
{
    NK_ASSERT(ctx);
    NK_ASSERT(evt);

    switch(evt->type)
    {
        case SDL_EVENT_KEY_UP: /* KEYUP & KEYDOWN share same routine */
        case SDL_EVENT_KEY_DOWN:
            {
                int down = evt->type == SDL_EVENT_KEY_DOWN;
                int ctrl_down = evt->key.mod & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL);

                switch(evt->key.key)
                {
                    case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
                    case SDLK_LSHIFT:    nk_input_key(ctx, NK_KEY_SHIFT, down); break;
                    case SDLK_DELETE:    nk_input_key(ctx, NK_KEY_DEL, down); break;
                    case SDLK_RETURN:    nk_input_key(ctx, NK_KEY_ENTER, down); break;
                    case SDLK_TAB:       nk_input_key(ctx, NK_KEY_TAB, down); break;
                    case SDLK_BACKSPACE: nk_input_key(ctx, NK_KEY_BACKSPACE, down); break;
                    case SDLK_HOME:      nk_input_key(ctx, NK_KEY_TEXT_START, down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_START, down); break;
                    case SDLK_END:       nk_input_key(ctx, NK_KEY_TEXT_END, down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_END, down); break;
                    case SDLK_PAGEDOWN:  nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down); break;
                    case SDLK_PAGEUP:    nk_input_key(ctx, NK_KEY_SCROLL_UP, down); break;
                    case SDLK_A:         nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, down && ctrl_down); break;
                    case SDLK_Z:         nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && ctrl_down); break;
                    case SDLK_R:         nk_input_key(ctx, NK_KEY_TEXT_REDO, down && ctrl_down); break;
                    case SDLK_C:         nk_input_key(ctx, NK_KEY_COPY, down && ctrl_down); break;
                    case SDLK_V:         nk_input_key(ctx, NK_KEY_PASTE, down && ctrl_down); break;
                    case SDLK_X:         nk_input_key(ctx, NK_KEY_CUT, down && ctrl_down); break;
                    case SDLK_B:         nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && ctrl_down); break;
                    case SDLK_E:         nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && ctrl_down); break;
                    case SDLK_UP:        nk_input_key(ctx, NK_KEY_UP, down); break;
                    case SDLK_DOWN:      nk_input_key(ctx, NK_KEY_DOWN, down); break;
                    case SDLK_LEFT:
                        if (ctrl_down)
                            nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
                        else
                            nk_input_key(ctx, NK_KEY_LEFT, down);
                        break;
                    case SDLK_RIGHT:
                        if (ctrl_down)
                            nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                        else
                            nk_input_key(ctx, NK_KEY_RIGHT, down);
                        break;
                }
                return 1;
            }

        case SDL_EVENT_MOUSE_BUTTON_UP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share same routine */
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                int down = evt->button.down;
                const int x = evt->button.x, y = evt->button.y;
                switch(evt->button.button)
                {
                    case SDL_BUTTON_LEFT:
                        // FIXME: This isn't perfect but should detect double-click
                        //        and will not prevent consecutive clicks
                        //        https://github.com/Immediate-Mode-UI/Nuklear/pull/779#issuecomment-2859431959
                        if (evt->button.clicks == 2)
                            nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
                        else
                            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
                        break;
                    case SDL_BUTTON_MIDDLE: nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down); break;
                    case SDL_BUTTON_RIGHT:  nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down); break;
                }
            }
            return 1;

        case SDL_EVENT_MOUSE_MOTION:
            ctx->input.mouse.pos.x = evt->motion.x;
            ctx->input.mouse.pos.y = evt->motion.y;
            ctx->input.mouse.delta.x = ctx->input.mouse.pos.x - ctx->input.mouse.prev.x;
            ctx->input.mouse.delta.y = ctx->input.mouse.pos.y - ctx->input.mouse.prev.y;
            return 1;

        case SDL_EVENT_TEXT_INPUT:
            {
                nk_glyph glyph;
                SDL_memcpy(glyph, evt->text.text, NK_UTF_SIZE);
                nk_input_glyph(ctx, glyph);
            }
            return 1;

        case SDL_EVENT_MOUSE_WHEEL:
            nk_input_scroll(ctx, nk_vec2(evt->wheel.x, evt->wheel.y));
            return 1;
    }
    return 0;
}

NK_API
void nk_sdl_shutdown(struct nk_context* ctx)
{
    struct nk_sdl* sdl;
    NK_ASSERT(ctx);
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    NK_ASSERT(sdl);

    nk_font_atlas_clear(&sdl->atlas);
    nk_buffer_free(&sdl->ogl.cmds);

    if (sdl->ogl.font_tex != NULL) {
        SDL_DestroyTexture(sdl->ogl.font_tex);
        sdl->ogl.font_tex = NULL;
    }

    ctx->userdata = nk_handle_ptr(0);
    SDL_free(sdl->font_debug);
    SDL_free(sdl);
    nk_free(ctx);
}

/* FIXME: width/height of the debug font atlas (integer)
 *        Probably needs better name... */
#define WH (10)

NK_INTERN float
nk_sdl_query_debug_font_width(nk_handle handle, float height,
                              const char *text, int len)
{
    NK_UNUSED(handle);
    NK_UNUSED(text);
    /* FIXME: does this thing need to handle newlines and tabs ??? */
    return height * len;
}

NK_INTERN void
nk_sdl_query_debug_font_glypth(nk_handle handle, float font_height,
                               struct nk_user_font_glyph *glyph,
                               nk_rune codepoint, nk_rune next_codepoint)
{
    char ascii;
    int idx, x, y;
    NK_UNUSED(next_codepoint);

    ascii = (codepoint > (nk_rune)'~' || codepoint < (nk_rune)' ')
            ? '?' : (char)codepoint;
    NK_ASSERT(ascii <= '~' && ascii >= ' ');

    idx = (int)(ascii - ' ');
    NK_ASSERT(idx >= 0 && idx <= (WH * WH));

    x = idx / WH;
    y = idx % WH;

    glyph->height = font_height;
    glyph->width = font_height;
    glyph->xadvance = font_height;
    glyph->uv[0].x = (float)(x + 0) / WH;
    glyph->uv[0].y = (float)(y + 0) / WH;
    glyph->uv[1].x = (float)(x + 1) / WH;
    glyph->uv[1].y = (float)(y + 1) / WH;
    glyph->offset.x = 0.0f;
    glyph->offset.y = 0.0f;
}

NK_API void
nk_sdl_style_set_font_debug(struct nk_context* ctx)
{
    struct nk_user_font* font;
    struct nk_sdl* sdl;
    SDL_Surface *surface;
    SDL_Renderer *renderer;
    char buf[2];
    int x, y;
    bool success;
    NK_ASSERT(ctx);

    /* FIXME: For now, use another software Renderer just to make sure
     *        that we won't change any state in the main Renderer,
     *        but it would be nice to reuse the main one */
    surface = SDL_CreateSurface(WH * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE,
                                WH * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE,
                                SDL_PIXELFORMAT_RGBA32);
    NK_ASSERT(surface);
    renderer = SDL_CreateSoftwareRenderer(surface);
    NK_ASSERT(renderer);
    success = SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    NK_ASSERT(success);

    /* SPACE is the first printable ASCII character */
    buf[0] = ' ';
    buf[1] = '\0';
    for (x = 0; x < WH; x++)
    {
        for (y = 0; y < WH; y++)
        {
            success = SDL_RenderDebugText(renderer,
                                          (float)(x * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE),
                                          (float)(y * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE),
                                          buf);
            NK_ASSERT(success);

            buf[0]++;
            /* TILDE is the last printable ASCII character */
            if (buf[0] > '~')
                break;
        }
    }
    success = SDL_RenderPresent(renderer);
    NK_ASSERT(success);

    font = SDL_malloc(sizeof(*font));
    NK_ASSERT(font);
    font->userdata.ptr = NULL;
    font->height = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    font->width = &nk_sdl_query_debug_font_width;
    font->query = &nk_sdl_query_debug_font_glypth;

    /* HACK: nk_sdl_device_upload_atlas turns pixels into SDL_Texture
     *       and sets said Texture into sdl->ogl.font_tex
     *       then nk_sdl_render expects same Texture at font->texture */
    sdl = (struct nk_sdl*)ctx->userdata.ptr;
    nk_sdl_device_upload_atlas(&sdl->ctx, surface->pixels, surface->w, surface->h);
    font->texture.ptr = sdl->ogl.font_tex;

    sdl->font_debug = font;
    nk_style_set_font(ctx, font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
}

#endif /* NK_SDL3_RENDERER_IMPLEMENTATION_ONCE */
#endif /* NK_SDL3_RENDERER_IMPLEMENTATION */
