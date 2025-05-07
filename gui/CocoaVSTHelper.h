#ifndef COCOA_VST_HELPER_H
#define COCOA_VST_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

// Create an NSView container for VST plugins
void *createVSTContainerView(void *windowPtr, int width, int height);

#ifdef __cplusplus
}
#endif

#endif // COCOA_VST_HELPER_H
