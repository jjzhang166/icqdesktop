//
//  NSData+Base64.h
//  imosx
//
//  Created by Александр Черный on 21.08.13.
//  Copyright (c) 2013 Mail.Ru. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSData (Base64)

- (id)initWithBase64EncodedString:(NSString *)string;

- (NSString *)base64Encoding;
- (NSString *)base64EncodingWithLineLength:(unsigned int)lineLength;

+ (NSData *)dataWithBase64EncodedString:(NSString *)string;

@end
