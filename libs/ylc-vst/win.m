// In a .m file (Objective-C)
#import <Cocoa/Cocoa.h>

// C-callable function
void* createCocoaWindowBridge(const char* title, int width, int height) {
    @autoreleasepool {
        // Create a window
        NSRect frame = NSMakeRect(0, 0, width, height);
        NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                         styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
                                         backing:NSBackingStoreBuffered
                                         defer:NO];
        [window setTitle:@(title)];
        [window center];
        [window makeKeyAndOrderFront:nil];
        
        // Return as void pointer for C code
        return (__bridge_retained void*)window;
    }
}

// Header exported to C code
#ifdef __cplusplus
extern "C" {
#endif
    void* createCocoaWindowBridge(const char* title, int width, int height);
#ifdef __cplusplus
}
#endif
