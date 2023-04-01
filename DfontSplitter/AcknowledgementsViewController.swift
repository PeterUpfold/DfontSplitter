//
//  AcknowledgementsViewController.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 16/09/2019.
//  Copyright © 2019-2023 Peter Upfold. All rights reserved.
//

import Cocoa

class AcknowledgementsViewController: NSViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do view setup here.
    }
    
    @IBAction func getSourceCode(_ sender: Any) {
        NSWorkspace.shared.open(URL(string: "https://github.com/PeterUpfold/DfontSplitter")!)
    }
    
    @IBAction func viewLicence(_ sender: Any) {
        NSWorkspace.shared.open(URL(string: "https://www.gnu.org/licenses/gpl-3.0-standalone.html")!)
    }
    
}
