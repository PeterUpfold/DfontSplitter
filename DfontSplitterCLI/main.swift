//
//  main.swift
//  DfontSplitterCLI
//
//  Created by Peter Upfold on 25/06/2020.
//  Copyright © 2020 Peter Upfold. All rights reserved.
//
/*
 This file is part of DfontSplitter.
 
 DfontSplitter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DfontSplitter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with DfontSplitter.  If not, see <https://www.gnu.org/licenses/>.
 */

import Foundation
import DfontSplitter
import CommonCrypto
import os

func runFonduOnRelativePath(path: String, currentURL: URL) -> Int {
    let file = currentURL.appendingPathComponent(path).path
    
    let unsafePointerOfFilename = NSString(string: file).utf8String
    let unsafeMutablePointerOfFilename: UnsafeMutablePointer<Int8> = UnsafeMutablePointer<Int8>(mutating: unsafePointerOfFilename!)
    
    return Int(fondu_main_simple(unsafeMutablePointerOfFilename))
}


let arguments = CommandLine.arguments

NSLog("Extracting \(arguments[1]) to \(URL(fileURLWithPath: FileManager.default.currentDirectoryPath))")

let result = runFonduOnRelativePath(path: arguments[1], currentURL: URL(fileURLWithPath: FileManager.default.currentDirectoryPath))
NSLog("Result of runFonduOnRelativePath is \(result)")
