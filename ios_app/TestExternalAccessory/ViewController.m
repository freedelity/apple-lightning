//
//  ViewController.m
//  TestExternalAccessory
//
//  Created by Nicolas Dusart on 13/04/15.
//  Copyright (c) 2015 Freedelity. All rights reserved.
//

#import "ViewController.h"

NSString* PROTOCOL = @"Your accessory's protocol";

@interface ViewController ()
@property (nonatomic, strong) EAAccessory* accessory;
@property (nonatomic, strong) EASession* session;
@property (nonatomic, weak) NSOutputStream* output;
@property (nonatomic, weak) NSInputStream* input;
@property (weak, nonatomic) IBOutlet UITextField *text;
@property (weak, nonatomic) IBOutlet UIButton *btn;
@property (weak, nonatomic) IBOutlet UITextView *response;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    _accessory = nil;
    _session = nil;
    _response.text = @"Response\n";
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void)viewDidAppear:(BOOL)animated
{
    [self accessoryConnected];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(accessoryConnected) name:EAAccessoryDidConnectNotification object:nil];
    [[EAAccessoryManager sharedAccessoryManager] registerForLocalNotifications];
}

-(void)viewWillDisappear:(BOOL)animated
{
    [[EAAccessoryManager sharedAccessoryManager] unregisterForLocalNotifications];
}

- (IBAction)btnTouched:(id)sender {
    @synchronized( self.session )
    {
        if( self.session == nil )
            return;
        
        NSData* data = [[self.text.text stringByAppendingString:@"\n"] dataUsingEncoding:NSUTF8StringEncoding];
        
        [self.output write:data.bytes maxLength:data.length];
    }
}

-(void)accessoryConnected
{
    
    if( self.accessory != nil )
        return;
    
    NSArray *accessories = [[EAAccessoryManager sharedAccessoryManager]
                            connectedAccessories];
    
    for (EAAccessory *obj in accessories)
    {
        
        if([[obj protocolStrings] containsObject:PROTOCOL])
        {
            self.accessory = obj;
            obj.delegate = self;
            NSLog(@"Connection of %@ %@", obj.name, obj.modelNumber);
            break;
        }
    }
    
    if( self.accessory != nil )
    {
        self.session = [[EASession alloc] initWithAccessory:self.accessory forProtocol:[self.accessory.protocolStrings objectAtIndex:0]];
        self.output = [self.session outputStream];
        [self.output setDelegate:self];
        [self.output scheduleInRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
        [self.output open];
        self.input = [self.session inputStream];
        [self.input setDelegate:self];
        [self.input scheduleInRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
        [self.input open];
    }
}

-(void)stream:(NSStream *)aStream handleEvent:(NSStreamEvent)eventCode
{
    @synchronized(self.session)
    {
        if( self.session == nil )
            return;
    
    NSInteger read;
    uint8_t buffer[512];
    NSData* data;
    NSMutableString* newstring;
        
    switch (eventCode) {
        case NSStreamEventHasSpaceAvailable:
            break;
            
        case NSStreamEventHasBytesAvailable:
            read = [self.input read:buffer maxLength:512];
            data = [NSData dataWithBytes:buffer length:read];
            newstring = [[NSMutableString alloc] initWithString:self.response.text];
            [newstring appendString:[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]];
            self.response.text = newstring;
            break;
            
        default:
            break;
    }
    }
}

-(void)accessoryDidDisconnect:(EAAccessory *)accessory
{
    NSLog(@"accessory disconnection");
    
    if( self.accessory != nil )
    {
        @synchronized(self.session)
        {
            [self.output close];
            [self.output removeFromRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
            [self.output setDelegate:nil];
            [self.input close];
            [self.input removeFromRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
            [self.input setDelegate:nil];
            self.session = nil;
        }
        self.accessory.delegate = nil;
        self.accessory = nil;
    }
}

@end
