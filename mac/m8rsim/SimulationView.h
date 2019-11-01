//
//  SimulationView.h
//  m8rsim
//
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#import <Cocoa/Cocoa.h>

@class Document;

@interface SimulationView : NSViewController

- (void)updateGPIOState:(uint32_t) state withMode:(uint32_t) mode;

@end

