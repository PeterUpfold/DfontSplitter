//
//  DragDropTableView.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 31/01/2019.
//  Copyright © 2019 Peter Upfold. All rights reserved.
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

import Cocoa

class DragDropTableView: NSTableView {

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        // Drawing code here.
    }
    
    override func draggingUpdated(_ sender: NSDraggingInfo) -> NSDragOperation {
        return draggingEntered(sender)
    }
    
    override func awakeFromNib() {
        debugPrint("Registering for dragged types")
        registerForDraggedTypes([NSPasteboard.PasteboardType.fileURL])
    }
    
    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
        // if no delegate connected, no dragging is supported
        if (delegate == nil) {
            debugPrint("No delegate connected for dragging")
            return []
        }
        
        if (sender.draggingPasteboard.types!.contains(NSPasteboard.PasteboardType.fileURL)) {
            return NSDragOperation.copy
        }
        
        return []
    }
    
    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
        let pasteboard = sender.draggingPasteboard
        let filenames = pasteboard.pasteboardItems
        if (pasteboard.types!.contains(NSPasteboard.PasteboardType.fileURL) && delegate!.responds(to: #selector(ViewController.acceptFilenameDrag))) {

            for file in filenames! {
                delegate!.perform(#selector(ViewController.acceptFilenameDrag), with: file)
            }
            return true
        }
        return false
    }
    
}
