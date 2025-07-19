#ifndef _LANG_GUI_ARRAY_EDIT_H
#define _LANG_GUI_ARRAY_EDIT_H

#include <stdbool.h>
int create_array_editor(int size, double *data, double min_value,
                        double max_value);

int create_bool_array_editor(int size, bool *data);
#endif
