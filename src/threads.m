#ifdef __APPLE__

#include "rJava.h"

/* NSThread */
#import <Foundation/Foundation.h>

static NSConnection *auxiliaryConnection = nil;

#define m_ptr_t unsigned long

@protocol JavaCalls
- (oneway void) createObject: (m_ptr_t) ci;
- (oneway void) callMethod: (m_ptr_t) ci;
@end

@interface CallInterface : NSObject <JavaCalls>
+ (void)connectWithPorts:(NSArray *)portArray;
+ (CallInterface *) createMasterController;
@end

static CallInterface *masterInterface = nil;

@implementation CallInterface

+ (void)connectWithPorts:(NSArray *)portArray
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	_dbg(NSLog(@"connectWithPorts: starting auxiliary thread"));
	NSConnection *connectionToController = [NSConnection connectionWithReceivePort:[portArray objectAtIndex:0]
									      sendPort:[portArray objectAtIndex:1]];
	
	CallInterface *auxiliaryController = [[self alloc] init];
	[connectionToController setRootObject:auxiliaryController];
	[auxiliaryController release];
	
	auxiliaryConnection = connectionToController;

	NSRunLoop *theLoop = [NSRunLoop currentRunLoop];
	NSThread *theThread = [NSThread currentThread];
	_dbg(NSLog(@" - running run loop on thread %@", theThread));
	while (![theThread isCancelled]) {
		[theLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
	}
	_dbg(NSLog(@"Interface run loop terminated for thread %@", theThread));

	[pool release];	
	return;
}

+ (CallInterface *) createMasterController
{
	NSPort *port1;
	NSPort *port2;
	NSConnection *connectionToTransferServer;
	NSArray *portArray;
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	port1 = [NSPort port];
	port2 = [NSPort port];
	connectionToTransferServer = [[NSConnection alloc] initWithReceivePort:port1 sendPort:port2];
	portArray = [NSArray arrayWithObjects:port2, port1, nil];
	_dbg(NSLog(@"createMasterController: created connection, detaching new thread"));
	[NSThread detachNewThreadSelector:@selector(connectWithPorts:)
				 toTarget:[CallInterface class]
			       withObject:portArray];
	_dbg(NSLog(@" - waiting for the connection ..."));
	
	while ( [connectionToTransferServer rootProxy] == nil ) {}

	_dbg(NSLog(@" - connection established, setting protocol"));
	[[connectionToTransferServer rootProxy] setProtocolForProxy:@protocol(JavaCalls)];

	CallInterface *root = (CallInterface*) [connectionToTransferServer rootProxy];
	_dbg(NSLog(@" - root proxy: %p (%@)", root, root);)

	[root retain];
	[pool release];
	_dbg(NSLog(@" - pool released, returning proxy");)
	return root;
}

- (void) createObject: (m_ptr_t) ci_ptr
{
	call_interface_t *ci = (call_interface_t*) ci_ptr;
	JNIEnv *env = getJNIEnv();
	_dbg(NSLog(@" - createObject: %p (class '%s', signature '%s', silent '%@')", ci, ci->clnam, ci->sig, ci->silent?@"yes":@"no"));
	ci->o = createObject(env, ci->clnam, ci->sig, ci->jpar, ci->silent);
	_dbg(NSLog(@"   result: %p", ci->o));
	ci->done = 1;
	CFRunLoopRef loop = (CFRunLoopRef) ci->aux;
	if (loop)
		CFRunLoopWakeUp(loop);
}

- (void) callMethod: (m_ptr_t) ci_ptr;
{
	call_interface_t *ci = (call_interface_t*) ci_ptr;
	JNIEnv *env = getJNIEnv();
}

@end

void thread_createObject(call_interface_t *ci)
{
	_dbg(Rprintf("thread_createObject: ci=%p, masterInterface=%p\n (class='%s', sig='%s', silent=%d)\n", ci, masterInterface, ci->clnam, ci->sig, ci->silent));
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if (!masterInterface) {
		masterInterface = [CallInterface createMasterController];
		_dbg(NSLog(@" - created new master interface: %@", masterInterface));
	}
	ci->done = 0;
	NSRunLoop *theLoop = [NSRunLoop currentRunLoop];
	ci->aux = [theLoop getCFRunLoop];

	[masterInterface createObject:(m_ptr_t)ci];

	while (!ci->done) {
		_dbg(NSLog(@" - waiting for result..."));
		R_CheckUserInterrupt();
		/* [theLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]]; */
	}

	[pool release];
	_dbg(Rprintf(" - createObject returned, o=%p\n", ci->o));
}

void thread_callMethod(call_interface_t *ci)
{
	if (!masterInterface) {
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		masterInterface = [CallInterface createMasterController];
		[pool release];
	}
	[masterInterface callMethod:(m_ptr_t)ci];
}

#endif
