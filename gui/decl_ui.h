#ifndef _LANG_GUI_DECL_UI_H
#define _LANG_GUI_DECL_UI_H
int create_decl_ui(void *cb);
void *Plt(double x_min, double x_max, double y_min, double y_max);
void *Scatter(void *plt, int size, double *x, double *y);
void *LinePlt(void *_plt, int size, double *x, double *y);
#endif
