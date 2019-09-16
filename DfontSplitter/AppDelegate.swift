//
//  AppDelegate.swift
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

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {

    var preferencesWindowController: NSWindowController?
    
    var acknowledgementsWindowController: NSWindowController?


    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Insert code here to initialize your application
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }
    
   

    @IBAction func openPreferences(_ sender: Any) {
        if (preferencesWindowController == nil) {
            let storyboard = NSStoryboard(name: "Preferences", bundle: nil)
            debugPrint("storyboard: \(storyboard)")
            preferencesWindowController = storyboard.instantiateController(withIdentifier: "Preferences") as? NSWindowController
            debugPrint("preferencesWindowController: \(preferencesWindowController)")
        }
        
        if (preferencesWindowController != nil) {
            preferencesWindowController!.showWindow(sender)
        }
        
    }
    
    @IBAction func openAcknowledgements(_ sender: Any) {
        if (acknowledgementsWindowController == nil) {
            let storyboard = NSStoryboard(name: "Acknowledgements", bundle: nil)
            acknowledgementsWindowController = storyboard.instantiateController(withIdentifier: "Acknowledgements") as? NSWindowController
        }
        
        if (acknowledgementsWindowController != nil) {
            acknowledgementsWindowController!.showWindow(sender)
        }
    }
    
    @IBAction func fileOpen(_ sender: Any) {
        NotificationCenter.default.post(name: Notification.Name("FileOpenPressed"), object: nil)
    }
    
    @IBAction func fileConvert(_ sender: Any) {
        NotificationCenter.default.post(name: Notification.Name("FileConvertPressed"), object: nil)
    }
    
}

