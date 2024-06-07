#include "string_proc.h"
#include <stdlib.h>

// Function to process escape sequences
char *process_escapes(const char *raw_str, int len) {
  // Allocate enough space to handle the worst case where no escapes are
  // processed
  char *processed_str = malloc(len + 1); // +1 for null terminator
  char *dst = processed_str;
  const char *src = raw_str;
  const char *end = raw_str + len;

  while (src < end) {
    if (*src == '\\' && src + 1 < end) {
      src++; // Skip the backslash
      switch (*src) {
      case 'n':
        *dst++ = '\n';
        break;
      case 't':
        *dst++ = '\t';
        break;
      case 'r':
        *dst++ = '\r';
        break;
      case '\\':
        *dst++ = '\\';
        break;
      case '\"':
        *dst++ = '\"';
        break;
      case '\'':
        *dst++ = '\'';
        break;
      // Add more escape sequences as needed
      default:
        // If an unknown escape sequence is found, just copy it literally
        *dst++ = '\\';
        *dst++ = *src;
        break;
      }
    } else {
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0'; // Null-terminate the processed string

  return processed_str;
}
