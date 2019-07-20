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
    
    override func viewDidAppear() {
        super.viewDidAppear()
        self.view.window?.title = "DfontSplitter"
        self.view.window?.representedURL = nil
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
            let unsafeMutablePointerOfFilename: UnsafeMutablePointer<Int8> = UnsafeMutablePointer<Int8>(mutating: unsafePointerOfFilename!)
            
            FileManager.default.changeCurrentDirectoryPath(NSTemporaryDirectory())
            debugPrint("temp dir is \(FileManager.default.currentDirectoryPath)")
            
            let fileURL = URL(fileURLWithPath: String(file))
            
            if (fileIsDfont(file: fileURL) || fileIsSuitcase(file: fileURL)) {
                let returnValue = fondu_main_simple(unsafeMutablePointerOfFilename)
                // here we get the bool result of FindResourceFile, so '1' is success
                
                debugPrint("fondu returned \(returnValue)")
            }
            else if (fileIsTTC(file: fileURL)) {
                let returnValue = handlefile(unsafeMutablePointerOfFilename)
                
                debugPrint("stripttc returned \(returnValue)")
       
            }
            else {
                let alert = NSAlert()
                alert.messageText = "Unable to determine the type of file “\(file)”."
                alert.informativeText = "DfontSplitter could not determine the type of this file, so does not understand how to convert it."
                alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: nil)
                return
            }

            
            // get file(s) from temp directory and move to target directory
            debugPrint("destination dir is \(pathControl.stringValue)")
            
          
            
            do {
                for file in try FileManager.default.contentsOfDirectory(atPath: FileManager.default.currentDirectoryPath) {
                  
                    // construct destination URL
                    let destination = URL(fileURLWithPath: pathControl.stringValue).appendingPathComponent(file)
                    
                    // does destination exist?
                    if FileManager.default.fileExists(atPath: destination.path) {
                        maybeOverwriteFileWithPrompt(
                            question: "“\(destination.path)” already exists. Do you want to replace it?", text: "A file that will be extracted has the same name as a file that already exists in the destination folder. Replacing it will overwrite its current contents.",
                            file: URL(fileURLWithPath: file),
                            destination: destination)
                    }
                    else {
                        do {
                            try FileManager.default.copyItem(at: URL(fileURLWithPath: file), to: destination)
                            
                            if (UserDefaults.standard.bool(forKey: "OpenFinderWindowAfterConvert")) {
                                debugPrint("Will spawn Finder at \(pathControl.stringValue)")
                                NSWorkspace.shared.selectFile(destination.path, inFileViewerRootedAtPath: pathControl.stringValue)
                            }
                            
                        }
                        catch {
                            debugPrint("Failed to copy extracted file \(file): \(error.localizedDescription)")
                            showCopyError(text: error.localizedDescription)
                        }
                    }
                }
            }
            catch {
                debugPrint("\(error.localizedDescription)")
            }
            
            
        }
    }
    
    func showCopyError(text: String) -> Void {
        let alert = NSAlert()
        alert.messageText = "Unable to copy an extracted file to the destination."
        alert.informativeText = text
        alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: nil)
    }
    
    
    func maybeOverwriteFileWithPrompt(question: String, text: String, file: URL, destination: URL) -> Void {
        let alert = NSAlert()
        alert.messageText = question
        alert.informativeText = text
        alert.alertStyle = .warning
        alert.addButton(withTitle: "Replace")
        alert.addButton(withTitle: "Cancel")
        
        alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: {{ (response) in
            if (response == NSApplication.ModalResponse.alertFirstButtonReturn) {
                debugPrint("Will overwrite \(file) as requested")
                
                
                do {
                    try FileManager.default.removeItem(atPath: destination.path)
                    try FileManager.default.copyItem(at: URL(resolvingAliasFileAt: file), to: destination)
                    
                    if (UserDefaults.standard.bool(forKey: "OpenFinderWindowAfterConvert")) {
                        NSWorkspace.shared.selectFile(destination.path, inFileViewerRootedAtPath: destination.path)
                    }
                }
                catch {
                    debugPrint("Failed to copy file \(file): \(error.localizedDescription)")
                    self.showCopyError(text: error.localizedDescription)
                }
            }
            }}())
        
        
    }
    
    func fileIsDfont(file: URL) -> Bool {
        let uti = try? file.resourceValues(forKeys:[.typeIdentifierKey]).typeIdentifier

        return (uti == "com.apple.truetype-datafork-suitcase-font")
    }
    
    func fileIsSuitcase(file: URL) -> Bool {
        let uti = try? file.resourceValues(forKeys:[.typeIdentifierKey]).typeIdentifier
        
        return (uti == "com.apple.font-suitcase")
    }
    
    func fileIsTTC(file: URL) -> Bool {
        let uti = try? file.resourceValues(forKeys:[.typeIdentifierKey]).typeIdentifier
        
        return (uti == "public.truetype-collection-font")
    }
    
    @IBOutlet weak var fontTableView: NSTableView!
    
    @IBOutlet weak var arrayController: NSArrayController!
    
    @IBOutlet weak var pathControl: NSPathControl!
    
    @IBOutlet weak var statusLabel: NSTextField!
    
    @IBAction func addFileToSourceFonts(_ sender: Any) {
        arrayController.addObject("Added with button \(Date())")
    }
    
    @IBAction func removeFromSourceFonts(_ sender: Any) {
        debugPrint(" \(arrayController!.selectionIndex)")
        arrayController.remove(atArrangedObjectIndex: arrayController!.selectionIndex)
    }
}

