//
//  webkit_plugin.h
//  SpaceTrader
//
//  Created by Scott Brooks on 21/02/07.
//  Copyright 2007 Hermitworks Entertainment. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

@interface webkit_plugin : NSView {
	NSDictionary	*_arguments;
	pthread_t		_thread;
	NSData			*_commandline;

}
/*- (void)mouseMoved:(NSEvent *)evt;
- (void)keyDown:(NSEvent *)evt;
- (void)mouseUp:(NSEvent *)evt;
- (void)mouseDown:(NSEvent *)evt;*/
- (void)setArguments:(NSDictionary *)arguments;

- (char *)cmdline;
@end
