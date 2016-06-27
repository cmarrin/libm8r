//
//  Document.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Document.h"

#import "NSTextView+JSDExtensions.h"

#import "Parser.h"
#import "Printer.h"

#include <iostream>
#import <sstream>

class MyPrinter;

@interface Document ()
{
    IBOutlet NSTextView* sourceEditor;
    __unsafe_unretained IBOutlet NSTextView *consoleOutput;
    __unsafe_unretained IBOutlet NSTextView *buildOutput;
    __weak IBOutlet NSTabView *outputView;
    __weak IBOutlet NSToolbarItem *runButton;
    __weak IBOutlet NSToolbarItem *buildButton;
    __weak IBOutlet NSToolbarItem *pauseButton;
    __weak IBOutlet NSToolbarItem *stopButton;
    
    NSString* _source;
    NSFont* _font;
    MyPrinter* _printer;
    m8r::ExecutionUnit* _eu;
    m8r::Program* _program;
    BOOL _running;
}

- (void)outputMessage:(NSString*) message toBuild:(BOOL) isBuild;

@end

class MyPrinter : public m8r::Printer
{
public:
    MyPrinter(Document* document) : _document(document), _isBuild(true) { }
    
    virtual void print(const char* s) const override
    {
        [_document outputMessage:[NSString stringWithUTF8String:s] toBuild: _isBuild];
    }
    
    void setToBuild(bool b) { _isBuild = b; }
    
private:
    Document* _document;
    bool _isBuild;
};

@implementation Document

- (void)textStorageDidProcessEditing:(NSNotification *)notification {
    NSTextStorage *textStorage = notification.object;
    NSString *string = textStorage.string;
    NSUInteger n = string.length;
    [textStorage removeAttribute:NSForegroundColorAttributeName range:NSMakeRange(0, n)];
    for (NSUInteger i = 0; i < n; i++) {
        unichar c = [string characterAtIndex:i];
        if (c == '\\') {
            [textStorage addAttribute:NSForegroundColorAttributeName value:[NSColor lightGrayColor] range:NSMakeRange(i, 1)];
            i++;
        } else if (c == '$') {
            NSUInteger l = ((i < n - 1) && isdigit([string characterAtIndex:i+1])) ? 2 : 1;
            [textStorage addAttribute:NSForegroundColorAttributeName value:[NSColor redColor] range:NSMakeRange(i, l)];
            i++;
        }
    }
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _printer = new MyPrinter(self);
        _eu = new m8r::ExecutionUnit(_printer);
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
    }
    return self;
}

- (void)dealloc
{
    delete _eu;
    delete _printer;
}

- (void)awakeFromNib
{
    [sourceEditor setFont:_font];
    [consoleOutput setFont:_font];
    [buildOutput setFont:_font];
    sourceEditor.ShowsLineNumbers = YES;
    [[sourceEditor textStorage] setDelegate:(id) self];
    if (_source) {
        [sourceEditor setString:_source];
    }

    runButton.enabled = NO;
    stopButton.enabled = NO;
    pauseButton.enabled = NO;
}

+ (BOOL)autosavesInPlace {
    return YES;
}

- (NSString *)windowNibName {
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"Document";
}

- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError {
    return [sourceEditor.string dataUsingEncoding:NSUTF8StringEncoding];
}

- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError {
    _source = [NSString stringWithUTF8String:(const char*)[data bytes]];
    if (sourceEditor) {
        [sourceEditor setString:_source];
    }
    return YES;
}

- (void)doImport:(id)sender {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  url = [[panel URLs] objectAtIndex:0];
            sourceEditor.string = [NSString stringWithContentsOfURL:url encoding:kUnicodeUTF8Format error:nil];
        }
    }];
}

- (void)outputMessage:(NSString*) message toBuild:(BOOL) isBuild
{
    if (isBuild) {
        [outputView selectTabViewItemAtIndex:1];
    } else {
        [outputView selectTabViewItemAtIndex:0];
    }
    NSTextView* view = isBuild ? buildOutput : consoleOutput;
    NSString* string = [NSString stringWithFormat: @"%@%@", view.string, message];
    [view setString:string];
    [view scrollRangeToVisible:NSMakeRange([[view string] length], 0)];
    [view setNeedsDisplay:YES];
}

- (IBAction)build:(id)sender {
    _program = nullptr;
    _running = false;
    runButton.enabled = NO;
    pauseButton.enabled = NO;
    stopButton.enabled = NO;
    
    [buildOutput setString: @""];
    
    _printer->setToBuild(true);
    [self outputMessage:[NSString stringWithFormat:@"Building %@\n", [self displayName]] toBuild:YES];

    m8r::StringStream stream([sourceEditor.string UTF8String]);
    m8r::Parser parser(&stream, _printer);
    [self outputMessage:@"Parsing finished...\n" toBuild:YES];

    if (parser.nerrors()) {
        [self outputMessage:[NSString stringWithFormat:@"***** %d error%s\n", 
                            parser.nerrors(), (parser.nerrors() == 1) ? "" : "s"] toBuild:YES];
    } else {
        [self outputMessage:@"0 errors. Ready to run\n" toBuild:YES];
        _program = parser.program();
        runButton.enabled = YES;

        m8r::ExecutionUnit eu(_printer);
        m8r::String codeString = eu.generateCodeString(_program);
        
        [self outputMessage:@"\n*** Start Generated Code ***\n\n" toBuild:YES];
        [self outputMessage:[NSString stringWithUTF8String:codeString.c_str()] toBuild:YES];
        [self outputMessage:@"\n*** End of Generated Code ***\n\n" toBuild:YES];
    }
}

- (IBAction)run:(id)sender {
    if (_running) {
        assert(0);
        return;
    }
    
    [buildOutput setString: @""];
    runButton.enabled = NO;
    stopButton.enabled = YES;
    _running = true;
    
    [consoleOutput setString: @""];
    [self outputMessage:@"*** Program started...\n\n" toBuild:NO];
    dispatch_queue_t queue = dispatch_queue_create("Run Queue", NULL);
    dispatch_async(queue, ^() {
        _printer->setToBuild(false);
        NSTimeInterval timeInSeconds = [[NSDate date] timeIntervalSince1970];
        _eu->run(_program);
        timeInSeconds = [[NSDate date] timeIntervalSince1970] - timeInSeconds;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [self outputMessage:[NSString stringWithFormat:@"\n\n*** Finished (run time:%fms)\n", timeInSeconds * 1000] toBuild:NO];
            _running = false;
            runButton.enabled = YES;
            stopButton.enabled = NO;
        });
    });
}

- (IBAction)pause:(id)sender {
}

- (IBAction)stop:(id)sender {
    if (!_running) {
        assert(0);
        return;
    }
    _eu->requestTermination();
    _running = false;
    runButton.enabled = YES;
    stopButton.enabled = NO;
    [self outputMessage:@"*** Stopped\n" toBuild:NO];
    return;
}

@end
