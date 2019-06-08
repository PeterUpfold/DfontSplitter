//
//  ViewController.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 09/11/2018.
//  Copyright © 2018 Peter Upfold. All rights reserved.
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


class ViewController: NSViewController {
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
        //pathControl.stringValue = FileManager.default.homeDirectoryForCurrentUser.absoluteString // sandboxing makes this path ugly and not the home folder :(
        
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    @objc var filenames: NSMutableArray = []
    
    @objc func acceptFilenameDrag(file: NSPasteboardItem) {
        let fileName = URL(string: file.string(forType: NSPasteboard.PasteboardType.fileURL) ?? "")?.path
        //debugPrint(fileName!)
        arrayController.addObject(fileName)
        
    }

    @IBAction func convertButton(_ sender: Any) {
        for file in arrayController!.arrangedObjects as! [NSString] {
            debugPrint(file)
            
            
            let unsafePointerOfFilename = file.utf8String
            var unsafeMutablePointerOfFilename: UnsafeMutablePointer<Int8> = UnsafeMutablePointer<Int8>(mutating: unsafePointerOfFilename!)
            
            FileManager.default.changeCurrentDirectoryPath(NSTemporaryDirectory())
            debugPrint("temp dir is \(FileManager.default.currentDirectoryPath)")
            
            let returnValue = fondu_main_simple(unsafeMutablePointerOfFilename)
            // here we get the bool result of FindResourceFile, so '1' is success
            
            debugPrint("fondu returned \(returnValue)")
            
            // get file(s) from temp directory and move to target directory
            debugPrint("destination dir is \(pathControl.stringValue)")
            
          
            do {
                for file in try FileManager.default.contentsOfDirectory(atPath: FileManager.default.currentDirectoryPath) {
                  
                    // construct destination URL
                    let destination = URL(fileURLWithPath: pathControl.stringValue).appendingPathComponent(file)
                    
                    do {
                        try FileManager.default.copyItem(at: URL(fileURLWithPath: file), to: destination)
                    }
                    catch {
                        debugPrint("Failed to copy file \(file): \(error.localizedDescription)")
                    }
                }
            }
            catch {
                debugPrint("\(error.localizedDescription)")
            }
            
        }
    }
    
    @IBOutlet weak var fontTableView: NSTableView!
    
    @IBOutlet weak var arrayController: NSArrayController!
    
    @IBOutlet weak var pathControl: NSPathControl!
    
}

