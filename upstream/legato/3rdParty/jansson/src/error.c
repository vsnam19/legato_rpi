#include <string.h>
#include "jansson_private.h"

void jsonp_error_init(json_error_t *error, const char *source)
{
    if(error)
    {
        error->text[0] = '\0';
        error->line = -1;
        error->column = -1;
        error->position = 0;
        if(source)
            jsonp_error_set_source(error, source);
        else
            error->source[0] = '\0';
    }
}

void jsonp_error_set_source(json_error_t *error, const char *source)
{
    size_t size_avail = sizeof(error->source);
    size_t size_needed;

    if(!error || !source)
        return;

    /*
     * Place the entire error source into the target buffer if it fits,
     * otherwise place the characters "..." followed the maximally long
     * suffix of source that fits into the remaining space.
     */

    size_needed = snprintf(error->source, size_avail, "%s", source) + 1;

    if (size_needed > size_avail)
        snprintf(error->source, size_avail, "...%s",
                 source + (size_needed - size_avail));
}

void jsonp_error_set(json_error_t *error, int line, int column,
                     size_t position, enum json_error_code code,
                     const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    jsonp_error_vset(error, line, column, position, code, msg, ap);
    va_end(ap);
}

void jsonp_error_vset(json_error_t *error, int line, int column,
                      size_t position, enum json_error_code code,
                      const char *msg, va_list ap)
{
    if(!error)
        return;

    if(error->text[0] != '\0') {
        /* error already set */
        return;
    }

    error->line = line;
    error->column = column;
    error->position = (int)position;

    vsnprintf(error->text, JSON_ERROR_TEXT_LENGTH - 1, msg, ap);
    error->text[JSON_ERROR_TEXT_LENGTH - 2] = '\0';
    error->text[JSON_ERROR_TEXT_LENGTH - 1] = code;
}
