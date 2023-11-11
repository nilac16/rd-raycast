#pragma once

#ifndef RC_ERROR_H
#define RC_ERROR_H

#ifndef UNICODE
#   define UNICODE 1
#endif
#ifndef _UNICODE
#   define _UNICODE 1
#endif
#include <Windows.h>
#include <stdbool.h>


typedef enum {
    RC_ERROR_NONE,      /* Clear the error state */
    RC_ERROR_WIN32,     /* Pass a pointer to the DWORD or NULL to fetch */
    RC_ERROR_HRESULT,   /* Pass a pointer to the HRESULT */
    RC_ERROR_ERRNO,     /* Pass a pointer to the errno or NULL to fetch */
    RC_ERROR_USER       /* Pass a pointer to the error string itself */
} rc_err_t;


/** @brief Raise an error
 *  @param type
 *      Type/source of the error
 *  @param data
 *      Data associated with the error
 *  @param fmt
 *      Context format string
 */
void rc_error_raise(rc_err_t type, const void *data, const wchar_t *fmt, ...);


/** @brief Get the error state
 *  @returns true if an error has been raised
 */
bool rc_error_state(void);


/** @brief Get the error strings
 *  @param ctx
 *      If not NULL, the context string pointer is placed here
 *  @param msg
 *      If not NULL, the message string pointer is placed here
 */
void rc_error_strings(const wchar_t **ctx, const wchar_t **msg);


#endif /* RC_ERROR_H */
