// CocoaBridge.h
#ifndef COCOA_BRIDGE_H
#define COCOA_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque type to represent Cocoa objects
typedef struct _CocoaWindow CocoaWindow;

// Create a window with title and dimensions
CocoaWindow *createCocoaWindow(const char *title, int width, int height);

// Show the window
void showCocoaWindow(CocoaWindow *window);

// Hide the window
void hideCocoaWindow(CocoaWindow *window);

// Destroy the window and release resources
void destroyCocoaWindow(CocoaWindow *window);

// Get the NSView for embedding VST content
void *getNSViewFromWindow(CocoaWindow *window);

// Resize the window
void resizeCocoaWindow(CocoaWindow *window, int width, int height);

// Check if window is still open
bool isCocoaWindowOpen(CocoaWindow *window);

// Process Cocoa events
void processCocoaEvents(void);

// Initialize Cocoa application (call once at program start)
void initCocoaApplication(void);

#ifdef __cplusplus
}
#endif

#endif // COCOA_BRIDGE_H
