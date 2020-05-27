//
//  Utils.swift
//  DfontSplitterTests
//
//  Created by Peter Upfold on 27/05/2020.
//  Copyright © 2020 Peter Upfold. All rights reserved.
//

import Foundation
import DfontSplitter
import CommonCrypto

func runFonduOnRelativePath(path: String, currentURL: URL) -> Int {
    let file = currentURL.appendingPathComponent(path).path
    
    let unsafePointerOfFilename = NSString(string: file).utf8String
    let unsafeMutablePointerOfFilename: UnsafeMutablePointer<Int8> = UnsafeMutablePointer<Int8>(mutating: unsafePointerOfFilename!)
    
    return Int(fondu_main_simple(unsafeMutablePointerOfFilename))
}

func getSHA256Hash(url: URL) -> NSData {
    let data = FileManager.default.contents(atPath: url.path)
    
    let bytes = [UInt8](data!)
    
    var hash = [UInt8](repeating: 0, count: Int(CC_SHA256_DIGEST_LENGTH))
    CC_SHA256(bytes, CC_LONG(bytes.count), &hash)
    
    let result = NSData(bytes: hash, length: Int(CC_SHA256_DIGEST_LENGTH))
    return result
}

func getSHA256HexString(url: URL) -> String {
    let data = getSHA256Hash(url: url)
    var bytes = [UInt8](repeating:0, count:data.count)
    data.getBytes(&bytes, length: data.count)
    
    var output = ""
    for byte in bytes {
        output += String(format:"%02x", UInt8(byte))
    }
    return output
}
