#ifndef _LANG_FORMAT_UTILS_H
#define _LANG_FORMAT_UTILS_H
// Text colors
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

// Background colors
#define BG_RED "\x1b[41m"
#define BG_GREEN "\x1b[42m"
#define BG_YELLOW "\x1b[43m"
#define BG_BLUE "\x1b[44m"
#define BG_MAGENTA "\x1b[45m"
#define BG_CYAN "\x1b[46m"
// Basic styles
// Text styles
#define STYLE_BOLD "\x1b[1m"
#define STYLE_UNDERLINE "\x1b[4m"
#define STYLE_REVERSED "\x1b[7m"
#define STYLE_DIM "\x1b[2m"
#define STYLE_ITALIC "\x1b[3m"
#define STYLE_BLINK "\x1b[5m"
#define STYLE_HIDDEN "\x1b[8m"
#define STYLE_STRIKETHROUGH "\x1b[9m"

// Underline styles
#define STYLE_UNDERLINE_DOUBLE "\x1b[21m"
#define STYLE_UNDERLINE_OFF "\x1b[24m"

// Overline (not widely supported)
#define STYLE_OVERLINE "\x1b[53m"

// Reset
#define STYLE_RESET_ALL "\x1b[0m"
#endif
