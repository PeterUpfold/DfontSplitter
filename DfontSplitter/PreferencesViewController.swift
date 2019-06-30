//
//  PreferencesViewController.swift
//  DfontSplitter
//
//  Created by Peter Upfold on 30/06/2019.
//  Copyright © 2019 Peter Upfold. All rights reserved.
//
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
import Cocoa

class PreferencesViewController: NSViewController {
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
        let shouldOpen = UserDefaults.standard.bool(forKey: "OpenFinderWindowAfterConvert")
                
    }
    
    
    
}
