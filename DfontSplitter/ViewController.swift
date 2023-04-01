//
//  ViewController.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 09/11/2018.
//  Copyright © 2018-23 Peter Upfold. All rights reserved.
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
import os

class ViewController: NSViewController, NSWindowDelegate {
    
    // remaining file operations. Once back to 0, we can clean the temp dir
    /*var pendingOperations : Int = 0 {
        didSet {
                if pendingOperations == 0 {
                    DispatchQueue.global(qos: .background).async {
                    self.cleanTempFolder()
                }
            }
        }
    }*/
   
    override func viewDidLoad() {
        super.viewDidLoad()

        NotificationCenter.default.addObserver(self, selector: #selector(addFileToSourceFonts(_:)), name: Notification.Name("FileOpenPressed"), object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(convertButton(_:)), name: Notification.Name("FileConvertPressed"), object: nil)
    }
    
    func windowWillClose(_ notification: Notification) {
        cleanTempFolder()
        NSApplication.shared.terminate(self)
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
        self.view.window?.delegate = self
        self.view.window?.title = "DfontSplitter"
        self.view.window?.representedURL = nil
        
        if UserDefaults.standard.object(forKey: "OpenFinderWindowAfterConvert") == nil {
            UserDefaults.standard.set(true, forKey: "OpenFinderWindowAfterConvert")
        }
        
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
        
        if pathControl.stringValue.count < 1 {
            let alert = NSAlert()
            alert.messageText = NSLocalizedString("Please choose a destination folder.", comment: "alert title for if Convert is pressed with no destination folder selected")
            alert.informativeText = NSLocalizedString("Please choose a destination folder where DfontSplitter will save the converted files.", comment: "alert window detail for if Convert is pressed with no destination folder selected")
            alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: nil)
            return
        }
        
        // check destination folder

//        var isDir:ObjCBool = false
//
//        if !FileManager.default.fileExists(atPath: pathControl.stringValue, isDirectory: &isDir) {
//            // does not exist
//            os_log("“%s” does not exist as a destination folder.", pathControl.stringValue)
//            let alert = NSAlert()
//            alert.messageText = "Please choose a destination folder."
//            alert.informativeText = "The chosen destination folder no longer exists."
//            alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: nil)
//            return
//        }
//        if !isDir.boolValue {
//            // not folder
//            os_log("“%s” as a destination folder is not valid because it is not a folder.", pathControl.stringValue)
//            let alert = NSAlert()
//            alert.messageText = "Please choose a destination folder."
//            alert.informativeText = "The chosen destination must be a folder."
//            alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: nil)
//            return
//        }
        
        var tempUrl = URL(fileURLWithPath: NSTemporaryDirectory())
        do {
            tempUrl = try createTempFolderForConvertSession()
        }
        catch {
            os_log("Failed to create temporary folder: %{public}s", error.localizedDescription)
            showCopyError(text: error.localizedDescription)
            return
        }
    

        for file in arrayController!.arrangedObjects as! [NSString] {
            debugPrint(file)
            
        
            
            let unsafePointerOfFilename = file.utf8String
            let unsafeMutablePointerOfFilename: UnsafeMutablePointer<Int8> = UnsafeMutablePointer<Int8>(mutating: unsafePointerOfFilename!)
            
            FileManager.default.changeCurrentDirectoryPath(tempUrl.path)
            os_log("temp dir is %s", FileManager.default.currentDirectoryPath)
            
            let fileURL = URL(fileURLWithPath: String(file))
            
            if (fileIsDfont(file: fileURL) || fileIsSuitcase(file: fileURL)) {
                let returnValue = fondu_main_simple(unsafeMutablePointerOfFilename)
                // here we get the bool result of FindResourceFile, so '1' is success
                
                os_log("fondu returned %d", returnValue)
            }
            else if (fileIsTTC(file: fileURL)) {
                let returnValue = handlefile(unsafeMutablePointerOfFilename)
                
                os_log("stripttc returned %d", returnValue)
       
            }
            else {
                let alert = NSAlert()
                alert.messageText = NSLocalizedString("Unable to determine the type of file “\(file)”.", comment: "title for error message if source file is of an unknown type")
                alert.informativeText = NSLocalizedString("DfontSplitter could not determine the type of this file, so does not understand how to convert it.", comment: "alert box content for if a source file is of an unknown type")
                alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: nil)
                return
            }

            
            // get file(s) from temp directory and move to target directory
            os_log("destination dir is %s", pathControl.url!.absoluteString)
            
          
            
            do {
                for file in try FileManager.default.contentsOfDirectory(atPath: FileManager.default.currentDirectoryPath) {
                  
                    // construct destination URL
                    let destination = pathControl.url!.appendingPathComponent(file)
                    
                    // does destination exist?
                    if FileManager.default.fileExists(atPath: destination.path) {
                        maybeOverwriteFileWithPrompt(
                            question: String(format: NSLocalizedString("“%@” already exists. Do you want to replace it?", comment:"question for a dialogue box if a destination file already exists"), destination.path),
                            text: String(format: NSLocalizedString("A file that will be extracted has the same name as a file that already exists in the destination folder. Replacing it will overwrite its current contents.", comment: "explanatory text in dialogue box for overwriting a file")),
                            file: URL(fileURLWithPath: file),
                            destination: destination)
                    }
                    else {
                        do {
                            try FileManager.default.copyItem(at: URL(fileURLWithPath: file), to: destination)
                            
                            if (UserDefaults.standard.bool(forKey: "OpenFinderWindowAfterConvert")) {
                                os_log("Will spawn Finder at %s", pathControl.url!.absoluteString)
                                NSWorkspace.shared.selectFile(destination.path, inFileViewerRootedAtPath: pathControl.url!.path)
                            }
                            
                        }
                        catch {
                            os_log("Failed to copy extracted file %s: %{public}s", file, error.localizedDescription)
                            showCopyError(text: error.localizedDescription)
                        }
                    }
                }
            }
            catch {
                os_log("%{public}s", error.localizedDescription)
            }
            
        }
        
    }
    
    func showCopyError(text: String) -> Void {
        let alert = NSAlert()
        alert.messageText = NSLocalizedString("Unable to copy an extracted file to the destination.", comment: "error message when a file cannot be extracted. The default localized error text from the OS is displayed below this line in the dialogue box and does not need separate translation")
        alert.informativeText = text
        alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: nil)
    }
    
    func cleanTempFolder() -> Void {
        
        os_log("Cleaning temp folder")
        
        let enumerator = FileManager.default.enumerator(atPath: NSTemporaryDirectory())
        
        while let file = enumerator?.nextObject() as? String {
            os_log("clean %s", URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(file).absoluteString)
            do {
                try FileManager.default.removeItem(at: URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(file))
            }
            catch {
                os_log("failed to remove %s", URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(file).absoluteString)
            }
        }
    }
    
    func createTempFolderForConvertSession() throws -> URL {
        let url = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent("\(UUID())")
        try FileManager.default.createDirectory(at: url, withIntermediateDirectories: true, attributes: nil)
        return url
    }
    
    func maybeOverwriteFileWithPrompt(question: String, text: String, file: URL, destination: URL) -> Void {
        //pendingOperations += 1;
        let alert = NSAlert()
        alert.messageText = question
        alert.informativeText = text
        alert.alertStyle = .warning
        alert.addButton(withTitle: NSLocalizedString("Replace", comment: "button to overwrite an existing file"))
        alert.addButton(withTitle: NSLocalizedString("Cancel", comment: "button to cancel an overwrite operation"))
        
        alert.beginSheetModal(for: NSApp.mainWindow!, completionHandler: {{ (response) in
            if (response == NSApplication.ModalResponse.alertFirstButtonReturn) {
                os_log("Will overwrite %s as requested", file.absoluteString)
                
                
                do {
                    try FileManager.default.removeItem(atPath: destination.path)
                    try FileManager.default.copyItem(at: URL(resolvingAliasFileAt: file), to: destination)
                    
                    if (UserDefaults.standard.bool(forKey: "OpenFinderWindowAfterConvert")) {
                        NSWorkspace.shared.selectFile(destination.path, inFileViewerRootedAtPath: destination.path)
                    }
                }
                catch {
                    os_log("Failed to copy file %s %{public}s", file.absoluteString, error.localizedDescription)
                    self.showCopyError(text: error.localizedDescription)
                }
            }
            
            //self.pendingOperations -= 1;
            
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
        
        let dialogue = NSOpenPanel()
        dialogue.allowedFileTypes = ["com.apple.font-suitcase","com.apple.truetype-datafork-suitcase-font","public.truetype-collection-font"]
        
        if (dialogue.runModal() == NSApplication.ModalResponse.OK) {
            arrayController.addObject(dialogue.url!.path)
        }
        
        
    }
    
    @IBAction func removeFromSourceFonts(_ sender: Any) {
        os_log("remove at %d", arrayController!.selectionIndex)
        arrayController.remove(atArrangedObjectIndex: arrayController!.selectionIndex)
        
        /* debug */
        /*for file in arrayController!.arrangedObjects as! [NSString] {
            debugPrint("found \(file) in arrayController objects after remove")
        }*/
        
    }
}

