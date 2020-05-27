//
//  Utils.swift
//  DfontSplitterTests
//
//  Created by Peter Upfold on 27/05/2020.
//  Copyright © 2020 Peter Upfold. All rights reserved.
//

import Foundation
import DfontSplitter

func runFonduOnRelativePath(path: String, currentURL: URL) -> Int {
    let file = currentURL.appendingPathComponent(path).path
    
    let unsafePointerOfFilename = NSString(string: file).utf8String
    let unsafeMutablePointerOfFilename: UnsafeMutablePointer<Int8> = UnsafeMutablePointer<Int8>(mutating: unsafePointerOfFilename!)
    
    return Int(fondu_main_simple(unsafeMutablePointerOfFilename))
}
