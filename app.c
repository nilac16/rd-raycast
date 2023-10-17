#include "app.h"


/** @brief Initialize SDL libraries
 *  @returns Nonzero on error
 */
static int rc_app_init_sdl(void)
{
    if (SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Cannot initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    return 0;
}


/** @brief Create the main window
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_create_window(struct rc_app              *app,
                                const struct rc_app_params *params)
{
    const char *title = params->title;
    const int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
    const int w = params->width, h = params->height;
    const Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

    app->wnd = SDL_CreateWindow(title, x, y, w, h, flags);
    if (!app->wnd) {
        fprintf(stderr, "Cannot create window: %s\n", SDL_GetError());
    }
    return app->wnd == NULL;
}


/** @brief Create the renderer context
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_create_renderer(struct rc_app *app)
{
    const int idx = -1;
    const Uint32 flags = SDL_RENDERER_ACCELERATED;

    app->rend = SDL_CreateRenderer(app->wnd, idx, flags);
    if (!app->rend) {
        fprintf(stderr, "Cannot create renderer: %s\n", SDL_GetError());
    }
    return app->rend == NULL;
}


/** @brief Create the target texture
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_create_texture(struct rc_app *app)
{
    const Uint32 format = SDL_PIXELFORMAT_ABGR8888;
    const int access = SDL_TEXTUREACCESS_STREAMING;
    int w, h;

    SDL_GetWindowSize(app->wnd, &w, &h);
    app->tex = SDL_CreateTexture(app->rend, format, access, w, h);
    if (!app->tex) {
        fprintf(stderr, "Cannot create texture: %s\n", SDL_GetError());
    }
    return app->tex == NULL;
}


/** @brief Initialize information needed for controlling the viewport
 *  @param app
 *      Application state
 *  @returns Zero
 */
static int rc_app_init_input(struct rc_app *app)
{
    app->keystate = SDL_GetKeyboardState(&app->nkeys);
    return 0;
}


/** @brief Register custom events with SDL
 *  @param app
 *      Application state
 *  @returns Nonzero on error
 */
static int rc_app_init_events(struct rc_app *app)
{
    const int evcnt = 1;

    app->evwake = SDL_RegisterEvents(evcnt);
    if (app->evwake == (Uint32)-1) {
        fprintf(stderr, "Cannot allocate %d event slots\n", evcnt);
        return 1;
    }
    return 0;
}


/** @brief Start all SDL components
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_start_sdl(struct rc_app              *app,
                            const struct rc_app_params *params)
{
    return rc_app_init_sdl()
        || rc_app_create_window(app, params)
        || rc_app_create_renderer(app)
        || rc_app_create_texture(app)
        || rc_app_init_input(app)
        || rc_app_init_events(app);
}


/** @brief Load the dose
 *  @param app
 *      Application state buffer
 *  @param path
 *      Path to the dose file
 *  @returns Nonzero on error. Failing to load a dose file will cease to be an
 *      error at some point in the future
 */
static int rc_app_init_dose(struct rc_app *app, const char *path)
{
    if (rc_dose_load(&app->dose, path)) {
        fprintf(stderr, "Couldn't load dose file at %s\n", path);
        return 1;
    }
    if (rc_dose_compact(&app->dose, 0.01)) {
        fputs("Couldn't compact the dose\n", stderr);
    }
    return 0;
}


/** @brief Initialize the target context
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_init_target(struct rc_app *app)
{
    int acc, w, h;
    Uint32 fmt;

    SDL_QueryTexture(app->tex, &fmt, &acc, &w, &h);
    app->target.tex.dim[0] = w;
    app->target.tex.dim[1] = h;
    app->target.tex.stride = 4;
    return 0;
}


/** @brief Initialize the camera and its pitch/yaw quaternions
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_init_camera(struct rc_app *app)
{
    const double theta = RC_PI / 1080.0;
    double sintheta, costheta;
    vec_t mask, rot;

    rc_cam_default(&app->camera);
    /* Rotate camera to look in +y direction */
    rot = rc_set(sin(-RC_PI / 4.0), 0.0, 0.0, cos(-RC_PI / 4.0));
    app->camera.quat = rc_qmul(rot, app->camera.quat);
    sintheta = sin(theta);
    costheta = cos(theta);
    mask = rc_set(sintheta, sintheta, sintheta, costheta);
    app->pitch = rc_set(1.0, 0.0, 0.0, 1.0);
    app->yaw = rc_set(0.0, 0.0, 1.0, 1.0);
    app->pitch = rc_mul(app->pitch, mask);
    app->yaw = rc_mul(app->yaw, mask);
    {
        RC_ALIGN scal_t spill[12];

        rc_spill(spill + 0x0, rc_qrot(app->camera.quat, rc_set(1, 0, 0, 0)));
        rc_spill(spill + 0x4, rc_qrot(app->camera.quat, rc_set(0, 1, 0, 0)));
        rc_spill(spill + 0x8, rc_qrot(app->camera.quat, rc_set(0, 0, 1, 0)));
        printf("Camera axes: %-8.3f % -8.3f % -8.3f\n"
               "             %-8.3f % -8.3f % -8.3f\n"
               "             %-8.3f % -8.3f % -8.3f\n",
               spill[0], spill[4], spill[8],
               spill[1], spill[5], spill[9],
               spill[2], spill[6], spill[10]);
    }
    return 0;
}


__attribute_maybe_unused__
/** @brief Post a superfluous message to force the main loop to unblock
 *  @param app
 *      Application state
 */
static void rc_app_awaken(struct rc_app *app)
{
    SDL_Event e = { .type = app->evwake };
    int res;

    res = SDL_PushEvent(&e);
    if (res < 1) {
        fprintf(stderr, "Could not push awaken event: %s\n",
                (res) ? SDL_GetError() : "Event was filtered");
    }
}


/** @brief Mark the viewport for a redraw and force the event loop to unblock
 *  @param app
 *      Application state
 */
static void rc_app_mark_dirty(struct rc_app *app)
{
    app->dirty = true;
    //rc_app_awaken(app);
}


/** @brief Update the target texture with screen information. The target and
 *      screen must both be initialized
 *  @param app
 *      Application state buffer
 */
static void rc_app_screen_updated(struct rc_app *app)
{
    rc_target_update(&app->target, &app->screen);
    rc_app_mark_dirty(app);
}


/** @brief Initialize the screen buffer context
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_init_screen(struct rc_app *app)
{
    int w, h;

    SDL_GetWindowSize(app->wnd, &w, &h);
    app->screen.dim[0] = w;
    app->screen.dim[1] = h;
    app->screen.fov = 90.0;
    rc_app_screen_updated(app);
    return 0;
}


/** @brief Initialize the colormap
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_init_colormap(struct rc_app *app)
{
    dose_cmap_init(&app->cmap, app->dose.dmax);
    return 0;
}


/** @brief Allocate and initialize all of the viewport components
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_init_view(struct rc_app              *app,
                            const struct rc_app_params *params)
{
    return rc_app_init_dose(app, params->path)
        || rc_app_init_target(app)
        || rc_app_init_camera(app)
        || rc_app_init_screen(app)
        || rc_app_init_colormap(app);
}


/** @brief Copy the remainder of the settings
 *  @param app
 *      Application state
 *  @param params
 *      Passed parameters
 *  @returns Zero
 */
static int rc_app_init_settings(struct rc_app              *app,
                                const struct rc_app_params *params)
{
    app->mu_k      = params->mu_k;
    app->spdmult   = params->speed;
    app->turbomult = params->turbo;
    app->slowmult  = params->slow;
    app->keystate = SDL_GetKeyboardState(&app->nkeys);
    return 0;
}


int rc_app_open(struct rc_app *app, const struct rc_app_params *params)
{
    return rc_app_start_sdl(app, params)
        || rc_app_init_view(app, params)
        || rc_app_init_settings(app, params);
}


__attribute_maybe_unused__
/** @brief Await an event and store it in @p e
 *  @param app
 *      Application state buffer
 *  @param e
 *      Event buffer
 *  @returns Nonzero on error
 */
static int rc_app_await(struct rc_app *app, SDL_Event *e)
{
    int res;

    res = SDL_WaitEvent(e);
    if (!res) {
        fprintf(stderr, "Failed awaiting event! %s\n", SDL_GetError());
        app->shouldquit = true;
    }
    return !res;
}


/** @brief Handle the current input
 *  @param app
 *      Application state
 */
static void rc_app_input(struct rc_app *app)
{
    vec_t fwd, right, up, accel;
    scal_t tau;
    Uint32 tik;

    tik = SDL_GetTicks();
    tau = (scal_t)tik - (scal_t)app->lasttik;
    app->lasttik = tik;

    accel = rc_zero();
    fwd   = rc_qrot(app->camera.quat, rc_set(0.0, 0.0, 1.0, 0.0));
    right = rc_qrot(app->camera.quat, rc_set(1.0, 0.0, 0.0, 0.0));
    up    = rc_set(0.0, 0.0, 1.0, 0.0);

    if (app->keystate[SDL_SCANCODE_W]) {
        accel = rc_add(accel, fwd);
    }
    if (app->keystate[SDL_SCANCODE_A]) {
        accel = rc_sub(accel, right);
    }
    if (app->keystate[SDL_SCANCODE_S]) {
        accel = rc_sub(accel, fwd);
    }
    if (app->keystate[SDL_SCANCODE_D]) {
        accel = rc_add(accel, right);
    }
    if (app->keystate[SDL_SCANCODE_SPACE]) {
        accel = rc_add(accel, up);
    }
    if (app->keystate[SDL_SCANCODE_LCTRL]) {
        accel = rc_sub(accel, up);
    }

    if (app->keystate[SDL_SCANCODE_LSHIFT]) {
        accel = rc_mul(accel, rc_set1(app->turbomult));
    } else if (app->keystate[SDL_SCANCODE_LALT]) {
        accel = rc_mul(accel, rc_set1(app->slowmult));
    } else {
        accel = rc_mul(accel, rc_set1(app->spdmult));
    }

    accel = rc_sub(accel, rc_mul(rc_set1(app->mu_k), app->camera.vel));
    if (rc_cam_update(&app->camera, accel, tau)) {
        rc_app_mark_dirty(app);
    }
}


/** @brief Handle the quit message
 *  @param app
 *      Application state buffer
 *  @param e
 *      SDL quit event
 *  @returns Zero
 */
static int rc_app_quit(struct rc_app *app, SDL_QuitEvent *e)
{
    (void)e;

    app->shouldquit = true;
    return 0;
}


/** @brief Post a quit event, or die trying */
static void rc_app_postquit(void)
{
    SDL_Event e = { .type = SDL_QUIT };

    if (SDL_PushEvent(&e) <= 0) {
        fputs("Failed posting the quit message. Bye!~", stderr);
        abort();
    }
}


/** @brief Update the screen and target with the new display information
 *  @param app
 *      Application state buffer
 *  @param w
 *      The new window width
 *  @param h
 *      The new window height
 *  @returns Nonzero on error
 */
static int rc_app_size_changed(struct rc_app *app, int w, int h)
{
    app->screen.dim[0] = w;
    app->screen.dim[1] = h;
    rc_app_screen_updated(app);
    return 0;
}


/** @brief Handle window events
 *  @param app
 *      Application state buffer
 *  @param e
 *      SDL window event
 *  @returns Nonzero on error
 */
static int rc_app_window(struct rc_app *app, SDL_WindowEvent *e)
{
    int res = 0;

    switch (e->event) {
    case SDL_WINDOWEVENT_CLOSE:
        rc_app_postquit();
        break;
    case SDL_WINDOWEVENT_SIZE_CHANGED:
        res = rc_app_size_changed(app, e->data1, e->data2);
        break;
    default:
        break;
    }
    return res;
}


/** @brief Handle mouse motion events
 *  @param app
 *      Application state
 *  @param e
 *      SDL mouse motion event
 *  @returns Zero
 */
static int rc_app_motion(struct rc_app *app, SDL_MouseMotionEvent *e)
{
    vec_t inert, body;
    Uint32 mstate;

    mstate = SDL_GetMouseState(NULL, NULL);
    if (app->fullscreen || (SDL_BUTTON(1) & mstate)) {
        inert = rc_verspow(app->yaw, -e->xrel);
        body = rc_verspow(app->pitch, -e->yrel);
        rc_cam_comp_left(&app->camera, inert);
        rc_cam_comp_right(&app->camera, body);
        rc_app_mark_dirty(app);
    }
    return 0;
}


/** @brief Handle mouse wheel events
 *  @param app
 *      Application state
 *  @param e
 *      SDL mouse wheel event
 *  @returns Nonzero on error
 */
static int rc_app_wheel(struct rc_app *app, SDL_MouseWheelEvent *e)
{
    app->screen.fov = fmax(fmin(app->screen.fov - e->preciseY, 180.0), 1.0);
    rc_target_update(&app->target, &app->screen);
    rc_app_mark_dirty(app);
    return 0;
}


/** @brief Guess what this does?
 *  @param app
 *      Application state
 */
static void rc_app_toggle_fullscreen(struct rc_app *app)
{
    const Uint32 type = SDL_WINDOW_FULLSCREEN_DESKTOP;
    Uint32 cur, next;
    SDL_bool grab;

    cur = SDL_GetWindowFlags(app->wnd);
    next = cur ^ type;
    grab = !(cur & type);
    SDL_SetWindowFullscreen(app->wnd, next);
    SDL_SetWindowGrab(app->wnd, grab);
    SDL_SetRelativeMouseMode(grab);
    app->fullscreen = grab;
}


/** @brief Handle a keypress
 *  @param app
 *      Application state
 *  @param e
 *      SDL keyboard event
 *  @returns Nonzero on error
 */
static int rc_app_keydown(struct rc_app *app, SDL_KeyboardEvent *e)
{
    switch (e->keysym.sym) {
    case SDLK_RETURN:
        if (!(SDL_GetModState() | KMOD_ALT)) {
            break;
        }
        /* FALL THRU */
    case SDLK_F11:
        rc_app_toggle_fullscreen(app);
        break;
    default:
        break;
    }
    return 0;
}


/** @brief Poll and process all pending events
 *  @param app
 *      Application state buffer
 *  @returns Nonzero on error
 */
static int rc_app_process(struct rc_app *app)
{
    SDL_Event e;
    int res = 0;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            res = rc_app_quit(app, &e.quit);
            break;
        case SDL_WINDOWEVENT:
            res = rc_app_window(app, &e.window);
            break;
        case SDL_MOUSEMOTION:
            res = rc_app_motion(app, &e.motion);
            break;
        case SDL_MOUSEWHEEL:
            res = rc_app_wheel(app, &e.wheel);
            break;
        case SDL_KEYDOWN:
            res = rc_app_keydown(app, &e.key);
            break;
        default:
            break;
        }
    }
    return res;
}


/** @brief Redraw the dose to the target texture
 *  @param app
 *      Application state buffer
 */
static void rc_app_redraw(struct rc_app *app)
{
    int pitch;

    SDL_LockTexture(app->tex, NULL, &app->target.tex.pixels, &pitch);
    rc_raycast_dose(&app->dose, &app->target, &app->cmap.base, &app->camera);
    SDL_UnlockTexture(app->tex);
    SDL_RenderCopy(app->rend, app->tex, NULL, NULL);
    SDL_RenderPresent(app->rend);
    app->dirty = false;
}


int rc_app_run(struct rc_app *app)
{
    int res;

    app->lasttik = SDL_GetTicks();
    do {
        if (app->dirty) {
            rc_app_redraw(app);
            rc_cam_normalize(&app->camera);
        }
        //rc_app_await(app, NULL);
        rc_app_input(app);
        res = rc_app_process(app);
    } while (!app->shouldquit);
    return res;
}


void rc_app_close(struct rc_app *app)
{
    if (!app) {
        return;
    }
    SDL_DestroyTexture(app->tex);
    SDL_DestroyRenderer(app->rend);
    SDL_DestroyWindow(app->wnd);
    rc_dose_clear(&app->dose);
    SDL_Quit();
}
