// harness/main.mm
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CADisplayLink.h>

#include "rendering_engine_api.h"

@interface HarnessDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) CADisplayLink *displayLink;
@end;

@implementation HarnessDelegate
    - (void)applicationDidFinishLaunching:(NSNotification *)notification {
        NSRect frame = NSMakeRect(100, 100, 800, 600);
        self.window = [[NSWindow alloc] initWithContentRect:frame
            styleMask:NSWindowStyleMaskTitled |
            NSWindowStyleMaskClosable |
            NSWindowStyleMaskResizable
        backing: NSBackingStoreBuffered
        defer: NO
        ];

        self.window.title = @"Deco Rendering Engine";

        CAMetalLayer *layer = [CAMetalLayer layer];
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        layer.opaque = YES;
        self.window.contentView.layer = layer;
        self.window.contentView.wantsLayer = YES;

        [self.window makeKeyAndOrderFront:nil];
        [self.window center];
        [NSApp activateIgnoringOtherApps:YES];

        NSRect backing = [self.window convertRectToBacking:frame];
        int width = (int)backing.size.width;
        int height = (int)backing.size.height;

        rendering_engine_init((__bridge void*)layer, width, height);
        
        // The screen should be in the main thread, so we grab it before detaching a new thread
        NSScreen *screen = self.window.screen;
        [NSThread detachNewThreadSelector:@selector(renderLoopWithScreen)
            toTarget:self
            withObject:nil];
    }

    - (void)renderLoopWithScreen:(NSScreen*)screen {
        self.displayLink = [screen displayLinkWithTarget:self selector:@selector(render:)];
        NSRunLoop* runLoop = [NSRunLoop currentRunLoop];
        [self.displayLink addToRunLoop:runLoop forMode:NSDefaultRunLoopMode];
        [runLoop run];
    }

    - (void)render {
        rendering_engine_draw();
    }

    - (void)applicationWillTerminate:(NSNotification*)notification {
        [self.displayLink invalidate];
        self.displayLink = nil;
        rendering_engine_shutdown();
    }

    - (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app {
        return YES;
    }

@end

int main()
{
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    HarnessDelegate* delegate = [[HarnessDelegate alloc] init];
    app.delegate = delegate;
    [app run];
    return 0;
}
