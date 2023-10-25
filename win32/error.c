#include <stdarg.h>
#include <stdio.h>
#include "error.h"


__declspec(thread)
static struct {
    rc_err_t type;
    wchar_t  msg[256];
    wchar_t  ctx[256];
    union {
        DWORD   dwerr;
        HRESULT hr;
        int     erno;
    } data;
} err = { 0 };


void rc_error_raise(rc_err_t type, const void *data, const wchar_t *fmt, ...)
{
    va_list args;

    memset(&err, 0, sizeof err);
    switch (type) {
    case RC_ERROR_NONE:
    default:
        return;
    case RC_ERROR_WIN32:
        err.data.dwerr = (data) ? *(const DWORD *)data : GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      err.data.dwerr,
                      LANG_USER_DEFAULT,
                      err.msg,
                      sizeof err.msg / sizeof *err.msg,
                      NULL);
        break;
    case RC_ERROR_HRESULT:
        err.data.hr = (data) ? *(const HRESULT *)data : 0;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      HRESULT_CODE(err.data.hr),
                      LANG_USER_DEFAULT,
                      err.msg,
                      sizeof err.msg / sizeof *err.msg,
                      NULL);
        break;
    case RC_ERROR_ERRNO:
        err.data.erno = (data) ? *(const int *)data : errno;
        _wcserror_s(err.msg, sizeof err.msg / sizeof *err.msg, err.data.erno);
        break;
    case RC_ERROR_USER:
        swprintf(err.msg, sizeof err.msg / sizeof *err.msg, L"%s",
                (data) ? (const wchar_t *)data : L"");
        break;
    }
    va_start(args, fmt);
    swprintf(err.ctx, sizeof err.ctx / sizeof *err.ctx, fmt, args);
    va_end(args);
}


bool rc_error_state(void)
{
    return err.type != RC_ERROR_NONE;
}


void rc_error_strings(const wchar_t **ctx, const wchar_t **msg)
{
    if (ctx) {
        *ctx = err.ctx;
    }
    if (msg) {
        *msg = err.msg;
    }
}
