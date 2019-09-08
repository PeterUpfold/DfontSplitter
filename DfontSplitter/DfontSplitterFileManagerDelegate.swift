//
//  DfontSplitterFileManagerDelegate.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 08/09/2019.
//  Copyright © 2019 Peter Upfold. All rights reserved.
//

import Foundation
import Cocoa
import os

class DfontSplitterFileManagerDelegate: NSObject, FileManagerDelegate {
    
    override func awakeFromNib() {
        os_log("init delegate for file manager")
    }
    
    func fileManager(_ fileManager: FileManager, shouldRemoveItemAt URL: URL) -> Bool {
        os_log("Removing “%s”", URL.absoluteString)
        return true
    }
    
    func fileManager(_ fileManager: FileManager, shouldCopyItemAt srcURL: URL, to dstURL: URL) -> Bool {
        os_log("Copying “%s” to “%s”", srcURL.absoluteString, dstURL.absoluteString)
        return true
    }
    
    func fileManager(_ fileManager: FileManager, shouldProceedAfterError error: Error, removingItemAt URL: URL) -> Bool {
        os_log("File Manager reports error when removing “%s”: %{public}s", URL.absoluteString, error.localizedDescription)
        return false
    }
    
    func fileManager(_ fileManager: FileManager, shouldProceedAfterError error: Error, movingItemAt srcURL: URL, to dstURL: URL) -> Bool {
        os_log("File Manager reports error when moving “%s” to “%s”: %{public}s", srcURL.absoluteString, dstURL.absoluteString, error.localizedDescription)
        return false
    }
    
    func fileManager(_ fileManager: FileManager, shouldProceedAfterError error: Error, copyingItemAt srcURL: URL, to dstURL: URL) -> Bool {
        os_log("File Manager reports error when copying “%s” to “%s”: %{public}s", srcURL.absoluteString, dstURL.absoluteString, error.localizedDescription)
        return false
    }
    
    func fileManager(_ fileManager: FileManager, shouldMoveItemAt srcURL: URL, to dstURL: URL) -> Bool {
        os_log("Moving “%s” to “%s”", srcURL.absoluteString, dstURL.absoluteString)
        return true
    }
    
    
    
}
