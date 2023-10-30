#include <stdio.h>
#include <inttypes.h>
#include "app.h"
#include "error.h"
#include <hidusage.h>


/** Oh boy this is a dumb hack */
extern void dose_cmapfn(struct rc_colormap *this, double dose, void *pixel);


static void rc_win32_cmapfn(struct rc_colormap *this, double dose, void *pixel)
{
    union {
        int           value;
        unsigned char bytes[4];
    } px;
    unsigned char swap;

    dose_cmapfn(this, dose, pixel);
    px.value = *(int *)pixel;
    swap = px.bytes[0];
    px.bytes[0] = px.bytes[2];
    px.bytes[2] = swap;
    *(int *)pixel = px.value;
}


static void rc_win32_cmap_init(struct rc_win32_cmap *cmap, double dmax)
{
    dose_cmap_init(&cmap->base, dmax);
    cmap->base.base.func = rc_win32_cmapfn;
}


/** @brief Fetch the app state pointer from main window handle @p hwnd
 *  @param hwnd
 *      Handle to the main window
 *  @returns A pointer to the application state buffer
 */
static struct rc_app *rc_get_state(HWND hwnd)
{
    LONG_PTR lptr;

    lptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return (struct rc_app *)lptr;
}


/** @brief Create the main window
 *  @param hwnd
 *      Main window handle
 *  @param lp
 *      LPARAM containing pointer to CREATESTRUCT
 *  @return -1 on error, zero on success
 */
static LRESULT rc_app_wndcreate(HWND hwnd, LPARAM lp)
{
    CREATESTRUCT *cs = (CREATESTRUCT *)lp;
    struct rc_app *app = (struct rc_app *)cs->lpCreateParams;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)app);
    return 0;
}


/** @brief Paint the render window
 *  @param hwnd
 *      HANDLE to the main window
 *  @returns
 */
static LRESULT rc_app_wndpaint(HWND hwnd)
{
    struct rc_app *app;
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc;

    app = rc_get_state(hwnd);
    rc_raycast_dose(&app->dose, &app->target, &app->cmap.base, &app->camera);
    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rect);
    StretchDIBits(hdc,
                  0,
                  0,
                  rect.right,
                  rect.bottom,
                  0,
                  0,
                  app->target.tex.dim[0],
                  app->target.tex.dim[1],
                  app->target.tex.pixels,
                  &app->bmpinfo,
                  DIB_RGB_COLORS,
                  SRCCOPY);
    EndPaint(hwnd, &ps);
    return 0;
}


/** @brief Mark the window for a redraw
 *  @param app
 *      Application state
 */
static void rc_app_mark_redraw(struct rc_app *app)
{
    InvalidateRect(app->hwnd, NULL, FALSE);
}


/** @brief Call this when the screen information changes, to update the texture
 *      and issue a WM_PAINT message for the whole client area
 *  @param app
 *      Application state
 */
static void rc_app_screen_updated(struct rc_app *app)
{
    rc_target_update(&app->target, &app->screen);
    rc_app_mark_redraw(app);
}


/** @brief Window was resized, update the target texture appropriately and queue
 *      a redraw
 *  @param hwnd
 *      Main window handle
 *  @param lp
 *      LPARAM containing the new size
 *  @returns Zero
 */
static LRESULT rc_app_wndsize(HWND hwnd, LPARAM lp)
{
    struct rc_app *app;
    WORD w, h;

    app = rc_get_state(hwnd);
    w = LOWORD(lp);
    h = HIWORD(lp);
    app->screen.dim[0] = w;
    app->screen.dim[1] = h;
    rc_app_screen_updated(app);
    return 0;
}


/** @brief Handle clicks of the right mouse button */
static LRESULT rc_app_wndrbutton(HWND hwnd, LPARAM lp)
{
    struct rc_app *app;

    (void)lp;
    app = rc_get_state(hwnd);
    rc_cam_lookat(&app->camera, app->dose.centr);
    app->autotarget = !app->autotarget;
    rc_app_mark_redraw(app);
    return 0;
}


/** @brief Main window callback function
 *  @param hwnd
 *      Main window handle
 *  @param msg
 *      Received message
 *  @param wp
 *      WPARAM associated with @p msg
 *  @param lp
 *      LPARAM associated with @p msg
 *  @return Whatever is apropos and expected of @p msg
 */
static LRESULT CALLBACK rc_app_wndproc(HWND   hwnd, UINT   msg,
                                       WPARAM wp,   LPARAM lp)
{
    LRESULT res = 0;

    switch (msg) {
    case WM_RBUTTONDOWN:
        res = rc_app_wndrbutton(hwnd, lp);
        break;
    case WM_SIZE:
        res = rc_app_wndsize(hwnd, lp);
        break;
    case WM_PAINT:
        res = rc_app_wndpaint(hwnd);
        break;
    case WM_CREATE:
        res = rc_app_wndcreate(hwnd, lp);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        res = DefWindowProc(hwnd, msg, wp, lp);
        break;
    }
    return res;
}


/** @brief Fetch a pointer to the main window class name in static storage
 *  @returns A wide string describing the main window's class name
 */
static const wchar_t *rc_app_classname(void)
{
    return L"RaycastWindow";
}


/** @brief Register the window class
 *  @param app
 *      Application state
 *  @param params
 *      Initial app params
 *  @returns Nonzero on error
 */
static int rc_app_register_window(struct rc_app              *app,
                                  const struct rc_app_params *params)
{
    static const wchar_t *failmsg = L"Could not register the main window class";
    WNDCLASSEX wcex = {
        .cbSize        = sizeof wcex,
        .style         = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc   = &rc_app_wndproc,
        .cbClsExtra    = 0,
        .cbWndExtra    = 0,
        .hInstance     = app->hinst,
        .hIcon         = NULL,
        .hCursor       = NULL,
        .hbrBackground = NULL,
        .lpszMenuName  = NULL,
        .lpszClassName = NULL,
        .hIconSm       = NULL
    };
    ATOM res;

    wcex.hCursor = LoadCursor(NULL, IDC_HAND);
    wcex.lpszClassName = rc_app_classname();
    res = RegisterClassEx(&wcex);
    if (!res) {
        rc_error_raise(RC_ERROR_WIN32, NULL, failmsg);
    }
    return !res;
}


/** @brief Create the main window
 *  @param app
 *      App state buffer
 *  @param params
 *      Initial app params
 *  @returns Nonzero on error
 */
static int rc_app_create_window(struct rc_app              *app,
                                const struct rc_app_params *params)
{
    static const wchar_t *failmsg = L"Cannot create the main window";
    const DWORD style = WS_TILEDWINDOW;
    const wchar_t *wclass;
    wchar_t buf[256];

    swprintf(buf, sizeof buf / sizeof *buf, L"%S", params->title);
    wclass = rc_app_classname();
    app->hwnd = CreateWindow(wclass,
                             buf,
                             style,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             params->width,
                             params->height,
                             NULL,
                             NULL,
                             app->hinst,
                             (void *)app);
    if (!app->hwnd) {
        rc_error_raise(RC_ERROR_WIN32, NULL, failmsg);
    }
    return !app->hwnd;
}


/** @brief Initialize the main window
 *  @param app
 *      Application state buffer
 *  @param params
 *      Initial app parameters
 *  @returns Nonzero on error
 */
static int rc_app_init_window(struct rc_app              *app,
                              const struct rc_app_params *params)
{
    return rc_app_register_window(app, params)
        || rc_app_create_window(app, params);
}


/** @brief Load the dose that was provided on the command line
 *  @param app
 *      Application state
 *  @param params
 *      App parameters
 *  @returns Nonzero on error
 */
static int rc_app_load_dose(struct rc_app              *app,
                            const struct rc_app_params *params)
{
    static const wchar_t *failfmt = L"Cannot load the dose file at %S";
    const double threshold = 0.01;

    if (rc_dose_load(&app->dose, params->path)) {
        rc_error_raise(RC_ERROR_USER, NULL, failfmt, params->path);
        return -1;
    }
    if (rc_dose_compact(&app->dose, threshold)) {
        fputws(L"Could not compact dose!", stderr);
    }
    return 0;
}


/** @brief Initialize screen- and target-specific information
 *  @param app
 *      Application state
 *  @returns Nonzero on error
 */
static int rc_app_init_target(struct rc_app *app)
{
    static const wchar_t *failmsg = L"Cannot create target texture";
    const unsigned stride = 4;
    const double fov = 90.0;
    RECT rect = { 0 };
    size_t size;

    GetClientRect(app->hwnd, &rect);
    app->target.tex.dim[0] = rect.right;
    app->target.tex.dim[1] = rect.bottom;
    app->target.tex.stride = stride;
    size = (size_t)stride * rect.right * rect.bottom;
    app->target.tex.pixels = malloc(size);
    if (!app->target.tex.pixels) {
        if (size) {
            rc_error_raise(RC_ERROR_ERRNO, NULL, failmsg);
        } else {
            rc_error_raise(RC_ERROR_USER, L"Window has size zero", failmsg);
        }
        return -1;
    }
    app->screen.dim[0] = rect.right;
    app->screen.dim[1] = rect.bottom;
    app->screen.fov = fov;
    rc_app_screen_updated(app);
    return 0;
}


/** @brief Initialize the BMPINFO needed for GDI to blit the texture to the
 *      window
 *  @param app
 *      Application state
 *  @returns Nonzero on error
 */
static int rc_app_init_bmpinfo(struct rc_app *app)
{
    const BITMAPINFO myinfo = {
        .bmiHeader = {
            .biSize          = sizeof myinfo.bmiHeader,
            .biWidth         = app->target.tex.dim[0],
            .biHeight        = -((LONG)app->target.tex.dim[1]),
            .biPlanes        = 1,
            .biBitCount      = 32,
            .biCompression   = BI_RGB,
            .biSizeImage     = 0,
            .biXPelsPerMeter = 0,
            .biYPelsPerMeter = 0,
            .biClrUsed       = 0,
            .biClrImportant  = 0
        }
    };

    app->bmpinfo = myinfo;
    return 0;
}


/** @brief Initialize the camera
 *  @param app
 *      Application state
 *  @returns Nonzero on error
 */
static int rc_app_init_camera(struct rc_app *app)
{
    const double theta = RC_PI / 1080.0;
    double sintheta, costheta;
    vec_t mask, rot;

    rc_cam_default(&app->camera);
    rot = rc_set(sin(-RC_PI / 4.0), 0.0, 0.0, cos(-RC_PI / 4.0));
    app->camera.quat = rot;
    sintheta = sin(theta);
    costheta = cos(theta);
    mask = rc_set(sintheta, sintheta, sintheta, costheta);
    app->yaw = rc_set(0.0, 0.0, 1.0, 1.0);
    app->pitch = rc_set(1.0, 0.0, 0.0, 1.0);
    app->yaw = rc_mul(app->yaw, mask);
    app->pitch = rc_mul(app->pitch, mask);
    return 0;
}


/** @brief Initialize the colormap
 *  @param app
 *      Application state
 *  @returns Nonzero on error
 */
static int rc_app_init_colormap(struct rc_app *app)
{
    rc_win32_cmap_init(&app->cmap, app->dose.dmax);
    return 0;
}


/** TODO: Just make a header for this or something */
#if _OPENMP
#   include <omp.h>
#   define omp_nthreads() omp_get_max_threads()
#else
#   define omp_nthreads() 1
#endif


/** @brief Copy the remaining settings
 *  @param app
 *      Application state
 *  @param params
 *      Application parameters
 *  @returns Zero
 */
static int rc_app_copy_params(struct rc_app              *app,
                              const struct rc_app_params *params)
{
    const scal_t mult = (scal_t)1e-1;
    LARGE_INTEGER freq;
    int nthrd;

    nthrd = omp_nthreads();
    wprintf(L"This machine has %d thread%s available for raycasting\n",
            nthrd, (nthrd == 1) ? L"" : L"s");
    QueryPerformanceFrequency(&freq);
    app->tikmult = (scal_t)1e3 / (scal_t)freq.QuadPart;
    app->mu_k  = mult * params->mu_k;
    app->speed = mult * params->speed;
    app->slow  = mult * params->slow;
    app->turbo = mult * params->turbo;
    return 0;
}


/** @brief Load the dose and set up the camera
 *  @param app
 *      Application state
 *  @param params
 *      Initial app params
 *  @returns Nonzero on error
 */
static int rc_app_init_view(struct rc_app              *app,
                            const struct rc_app_params *params)
{
    return rc_app_load_dose(app, params)
        || rc_app_init_target(app)
        || rc_app_init_bmpinfo(app)
        || rc_app_init_camera(app)
        || rc_app_init_colormap(app)
        || rc_app_copy_params(app, params);
}


int rc_app_open(struct rc_app *app, const struct rc_app_params *params)
{
    app->hinst = GetModuleHandle(NULL);
    return rc_app_init_window(app, params)
        || rc_app_init_view(app, params);
}


/** @brief Fetch the time difference since the last call to this function
 *  @param app
 *      Application state
 *  @returns The time difference in milliseconds
 */
static scal_t rc_app_get_tdiff(struct rc_app *app)
{
    LARGE_INTEGER count;
    scal_t res;

    QueryPerformanceCounter(&count);
    res = (scal_t)(count.QuadPart - app->lasttik.QuadPart);
    app->lasttik = count;
    return res * app->tikmult;
}


/** @brief Check the current keystate and accelerate the camera accordingly
 *  @param app
 *      Application state
 */
static void rc_process_keys(struct rc_app *app)
{
    const int VK_W = 0x57, VK_A = 0x41, VK_S = 0x53, VK_D = 0x44;
    const scal_t taulim = 10.0;
    vec_t fwd, right, up, accel;
    scal_t tau;

    tau = rc_app_get_tdiff(app);
    tau = (taulim * tau) / (tau + taulim);

    up    = rc_set(0.0, 0.0, 1.0, 0.0);
    fwd   = rc_qrot(app->camera.quat, up);
    right = rc_qrot(app->camera.quat, rc_set(1.0, 0.0, 0.0, 0.0));
    accel = rc_zero();

    if (GetAsyncKeyState(VK_W)) {
        accel = rc_add(accel, fwd);
    }
    if (GetAsyncKeyState(VK_A)) {
        accel = rc_sub(accel, right);
    }
    if (GetAsyncKeyState(VK_S)) {
        accel = rc_sub(accel, fwd);
    }
    if (GetAsyncKeyState(VK_D)) {
        accel = rc_add(accel, right);
    }
    if (GetAsyncKeyState(VK_SPACE)) {
        accel = rc_add(accel, up);
    }
    if (GetAsyncKeyState(VK_LCONTROL)) {
        accel = rc_sub(accel, up);
    }

    if (GetAsyncKeyState(VK_LSHIFT)) {
        accel = rc_mul(accel, rc_set1(app->turbo));
    } else if (GetAsyncKeyState(VK_LMENU)) {
        accel = rc_mul(accel, rc_set1(app->slow));
    } else {
        accel = rc_mul(accel, rc_set1(app->speed));
    }

    accel = rc_fmadd(rc_set1(-app->mu_k), app->camera.vel, accel);
    if (rc_cam_update(&app->camera, accel, tau)) {
        if (app->autotarget) {
            rc_cam_lookat(&app->camera, app->dose.centr);
        }
        rc_app_mark_redraw(app);
    }
}


/** @brief Process mouse input
 *  @param app
 *      Application state
 */
static void rc_process_mouse(struct rc_app *app)
{
    typedef uint64_t QWORD;     /* hack */
    alignas(alignof (QWORD)) char buf[1024];
    RAWINPUT *ptr = (RAWINPUT *)buf;
    UINT i;

    
}


/** @brief Process input keystrokes and mouse motion
 *  @param app
 *      Application state
 */
static void rc_process_input(struct rc_app *app)
{
    rc_process_keys(app);
    rc_process_mouse(app);
}


/** @brief Clear the message queue
 *  @param app
 *      Application state
 *  @returns Nonzero on error
 */
static int rc_process_messages(struct rc_app *app)
{
    const UINT rmmsg = PM_REMOVE;
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, rmmsg)) {
        switch (msg.message) {
        case WM_QUIT:
            app->shouldquit = true;
            break;
        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }
    return 0;
}


int rc_app_run(struct rc_app *app)
{
    HWND hfocus;
    int res;

    ShowWindow(app->hwnd, 1);
    rc_app_get_tdiff(app);
    do {
        hfocus = GetFocus();
        if (hfocus == app->hwnd) {
            rc_process_input(app);
        }
        res = rc_process_messages(app);
    } while (!app->shouldquit);
    return res;
}


void rc_app_close(struct rc_app *app)
{
    if (!app) {
        return;
    }
    rc_dose_clear(&app->dose);
    free(app->target.tex.pixels);
}
