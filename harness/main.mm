// harness/main.mm
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CAMetalDisplayLink.h>

#include "rendering_engine_api.h"

@interface HarnessDelegate : NSObject <NSApplicationDelegate, CAMetalDisplayLinkDelegate>
@property (strong) NSWindow *window;
@property (strong) CAMetalDisplayLink *displayLink;
@property (strong) CAMetalLayer *metalLayer;
@property (assign) BOOL shouldStopLooping;
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

        // CAMetalLayer *layer = [CAMetalLayer layer];
        self.metalLayer = [CAMetalLayer layer];
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.metalLayer.opaque = YES;
        self.window.contentView.wantsLayer = YES;
        self.window.contentView.layer = self.metalLayer;

        [self.window makeKeyAndOrderFront:nil];
        [self.window center];
        [NSApp activateIgnoringOtherApps:YES];

        NSRect backing = [self.window convertRectToBacking:frame];
        int width = (int)backing.size.width;
        int height = (int)backing.size.height;

        rendering_engine_init((__bridge void*)self.metalLayer, width, height);

        // Grabbing the display link while in the main thread
        self.displayLink = [[CAMetalDisplayLink alloc] initWithMetalLayer:self.metalLayer];
        self.displayLink.delegate = self;
        self.displayLink.preferredFrameRateRange = CAFrameRateRangeMake(60, 120, 60);

        [NSThread detachNewThreadSelector:@selector(renderLoop)
            toTarget:self
            withObject:nil];
    }

    - (void)renderLoop {
        NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
        // We set up a mach port to keep the run loop alive.
        [runLoop addPort:[NSMachPort port] forMode:NSDefaultRunLoopMode];
        [self.displayLink addToRunLoop:runLoop forMode:NSDefaultRunLoopMode];
        
        while(!self.shouldStopLooping && [runLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]]) {};

        NSLog(@"Run loop ended");
    }

    - (void)metalDisplayLink:(CAMetalDisplayLink*)link
                 needsUpdate:(CAMetalDisplayLinkUpdate*)update {
        @autoreleasepool {
            rendering_engine_draw((__bridge void*)update.drawable);
        }
    }

    - (void)applicationWillTerminate:(NSNotification*)notification {
        self.shouldStopLooping = YES;
        [NSThread sleepForTimeInterval:0.2]; // gives time for "run loop ended" log to print
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
