//
//  DragDropTableView.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 31/01/2019.
//  Copyright © 2019 Peter Upfold. All rights reserved.
//

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
