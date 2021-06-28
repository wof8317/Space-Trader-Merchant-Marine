//
//  webkit_plugin.m
//  SpaceTrader
//
//  Created by Scott Brooks on 21/02/07.
//  Copyright 2007 Hermitworks Entertainment. All rights reserved.
//

#import "webkit_plugin.h"
#import <WebKit/WebKit.h>

int TheRealMain(char *cmdline, void * hWnd);

int ThreadProc( void *data ) {
	webkit_plugin *pi = data;
	return TheRealMain([pi cmdline], pi);
}
	
inline void processEvent(NSEvent *event, int currentTime);

@implementation webkit_plugin

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)keyDown:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)keyUp:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)mouseUp:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)mouseDown:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)rightMouseDown:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)otherMouseDown:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)scrollWheel:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)mouseMoved:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)mouseDragged:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)rightMouseDragged:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
- (void)otherMouseDragged:(NSEvent *)evt
{
	processEvent(evt, [evt timestamp] * 1000.0);
}
/*
- (NSDragOperation)draggingUpdated:(NSDraggingInfo*) sender
{
	NSEvent *evt = [NSEvent mouseEventWithType: NSMouseMoved];
	
	return NSDragOperationNone;
}*/

+ (NSView *)plugInViewWithArguments:(NSDictionary *)arguments
{
	webkit_plugin *game = [[[self alloc] init] autorelease];
	[game setArguments: arguments];
	return game;
}

- (void)setArguments:(NSDictionary *)arguments
{
	[arguments copy];
	[_arguments release];
	_arguments = arguments;
}

- (char *)cmdline
{
	static char challenge[2048];
	[_commandline getBytes: challenge length: sizeof(challenge)];
	return challenge;
}

- (void)webPlugInInitialize
{
}

- (void)webPlugInStart
{
	static int started=0;
	NSDictionary *webPluginAttributesObj = [_arguments objectForKey:WebPlugInAttributesKey];
	NSString *URLString = [webPluginAttributesObj objectForKey:@"src"];
	if (URLString != nil && [URLString length] != 0 ) {
		NSURL *baseURL = [_arguments objectForKey:WebPlugInBaseURLKey];
		NSError *err=nil;
		NSURL *URL = [NSURL URLWithString:URLString relativeToURL:baseURL];
		NSData *data = [[NSData alloc] initWithContentsOfURL: URL options: 0 error: &err];
		_commandline = [[NSData alloc] initWithBytes: [data bytes] length: [data length]];
			
		if ( started == 0 )
		{
			if (err == nil)
			{
				pthread_create(&_thread, NULL, (void *)ThreadProc, self);
				started = 1;
			} else 
			{
				printf("Error creating plugin: %s\n", [err localizedDescription]);
			}
		}
	}	
}
- (id)objectForWebScript
{
    return self;
}
- (void)webPlugInStop
{
	int status;
	if (_thread != 0)
	{
		pthread_kill(_thread,SIGUSR1);
		pthread_join(_thread, (void *)&status);
	}
}

@end
