//
//  PreferencesViewController.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 30/06/2019.
//  Copyright © 2019-2023 Peter Upfold. All rights reserved.
//

import Cocoa

class PreferencesViewController: NSViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do view setup here.
    }
    
    override func viewDidAppear() {
        
        let openFinderWindowAfterConvert = UserDefaults.standard.bool(forKey: "OpenFinderWindowAfterConvert")
        
        if (openFinderWindowAfterConvert) {
            openFinderWindowAfterConvertBox.state = .on
        }
        else {
            openFinderWindowAfterConvertBox.state = .off
        }
    }
    
    @IBAction func openFinderWindowAfterConvertBoxDidChange(_ sender: Any) {
        if (openFinderWindowAfterConvertBox.state == .on) {
            UserDefaults.standard.set(true, forKey: "OpenFinderWindowAfterConvert")
        }
        else {
            UserDefaults.standard.set(false, forKey: "OpenFinderWindowAfterConvert")
        }
    }
    
    @IBOutlet weak var openFinderWindowAfterConvertBox: NSButton!
    
}
