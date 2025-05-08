// CocoaBridge.m
#import <Cocoa/Cocoa.h>
#include "CocoaBridge.h"

// Custom window delegate to handle window close events
@interface VSTWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) bool windowClosed;
@end

@implementation VSTWindowDelegate
- (instancetype)init {
    self = [super init];
    if (self) {
        _windowClosed = false;
    }
    return self;
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
    self.windowClosed = true;
    return YES;
}
@end

// Define the opaque struct
struct _CocoaWindow {
    NSWindow* window;
    NSView* contentView;
    VSTWindowDelegate* delegate;
};

// Initialize the Cocoa application
void initCocoaApplication(void) {
    // Ensure we have an autorelease pool
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Create the application
    [NSApplication sharedApplication];
    
    // Set up application properties
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    // Finish setup
    [NSApp finishLaunching];
    
    // Create a menu (required for app to work properly)
    NSMenu* menubar = [[NSMenu alloc] init];
    NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    
    NSMenu* appMenu = [[NSMenu alloc] init];
    NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                         action:@selector(terminate:) 
                                                  keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];
    
    // Start the app in the background
    [NSApp activateIgnoringOtherApps:YES];
    
    [pool drain];
}

// Create a Cocoa window
CocoaWindow* createCocoaWindow(const char* title, int width, int height) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Create the window data structure
    CocoaWindow* cocoaWindow = (CocoaWindow*)malloc(sizeof(CocoaWindow));
    if (!cocoaWindow) {
        [pool drain];
        return NULL;
    }
    
    // Create the window delegate
    cocoaWindow->delegate = [[VSTWindowDelegate alloc] init];
    
    // Create the style mask for the window
    NSUInteger styleMask = NSWindowStyleMaskTitled | 
                           NSWindowStyleMaskClosable | 
                           NSWindowStyleMaskMiniaturizable | 
                           NSWindowStyleMaskResizable;
    
    // Create the window
    NSRect contentRect = NSMakeRect(0, 0, width, height);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:contentRect
                                                   styleMask:styleMask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    
    if (!window) {
        free(cocoaWindow);
        [pool drain];
        return NULL;
    }
    
    // Set window properties
    [window setTitle:[NSString stringWithUTF8String:title]];
    [window setDelegate:cocoaWindow->delegate];
    [window center];
    
    // Store window in our struct
    cocoaWindow->window = window;
    cocoaWindow->contentView = [window contentView];
    
    [pool drain];
    return cocoaWindow;
}

// Show the window
void showCocoaWindow(CocoaWindow* window) {
    if (!window || !window->window) return;
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Dispatch this on the main thread since UI operations must happen there
    dispatch_async(dispatch_get_main_queue(), ^{
        [window->window makeKeyAndOrderFront:nil];
    });
    
    [pool drain];
}

// Hide the window
void hideCocoaWindow(CocoaWindow* window) {
    if (!window || !window->window) return;
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [window->window orderOut:nil];
    });
    
    [pool drain];
}

// Destroy the window and release resources
void destroyCocoaWindow(CocoaWindow* window) {
    if (!window) return;
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Close and release the window
    if (window->window) {
        [window->window close];
        [window->window release];
    }
    
    // Release the delegate
    if (window->delegate) {
        [window->delegate release];
    }
    
    // Free our struct
    free(window);
    
    [pool drain];
}

// Get the NSView for embedding VST content
void* getNSViewFromWindow(CocoaWindow* window) {
    if (!window || !window->contentView) return NULL;
    return (void*)window->contentView;
}

// Resize the window
void resizeCocoaWindow(CocoaWindow* window, int width, int height) {
    if (!window || !window->window) return;
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        NSRect frame = [window->window frame];
        NSRect contentRect = [window->window contentRectForFrameRect:frame];
        
        float deltaWidth = width - contentRect.size.width;
        float deltaHeight = height - contentRect.size.height;
        
        frame.size.width += deltaWidth;
        frame.size.height += deltaHeight;
        
        [window->window setFrame:frame display:YES animate:NO];
    });
    
    [pool drain];
}

// Check if window is still open
bool isCocoaWindowOpen(CocoaWindow* window) {
    if (!window || !window->window || !window->delegate) return false;
    
    // If the window was closed through the delegate, return false
    if (window->delegate.windowClosed) return false;
    
    // Otherwise check if the window is still valid
    return [window->window isVisible];
}

// Process Cocoa events
void processCocoaEvents(void) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Process all available events
    NSDate* untilDate = [NSDate dateWithTimeIntervalSinceNow:0.001];
    NSEvent* event;
    
    do {
        event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                   untilDate:untilDate
                                      inMode:NSDefaultRunLoopMode
                                     dequeue:YES];
        
        if (event) {
            [NSApp sendEvent:event];
        }
    } while (event);
    
    [pool drain];
}
