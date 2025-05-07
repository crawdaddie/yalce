#import <Cocoa/Cocoa.h>

// This function creates an NSView container for VST plugins
void* createVSTContainerView(void* windowPtr, int width, int height) {
    NSWindow* window = (NSWindow*)windowPtr;
    NSView* contentView = [window contentView];
    
    // Create a container view for the plugin
    NSView* containerView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
    
    // Important: NSView needs to be layer-backed for most modern VST plugins
    [containerView setWantsLayer:YES];
    
    // Add the container to the window's content view
    [contentView addSubview:containerView];
    
    // Return the container view pointer
    return containerView;
}
