//
//  DfontSplitterTests.swift
//  DfontSplitterTests
//
//  Created by Peter Upfold on 09/11/2018.
//  Copyright © 2018 Peter Upfold. All rights reserved.
//

import XCTest
import CommonCrypto
@testable import DfontSplitter

// Before the test will work, the SampleFiles/ directory must be copied to
// ~/Library/Containers/uk.org.upfold.DfontSplitter/Data/SampleFiles

class DfontSplitterTests: XCTestCase {
    
    var currentURL = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
    var tempURL = URL(fileURLWithPath: NSTemporaryDirectory())

    override func setUp() {
        // Put setup code here. This method is called before the invocation of each test method in the class.
        let tempPath = tempURL.path
        FileManager.default.changeCurrentDirectoryPath(tempPath)
    }

    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }
    
    func testPTSansOTF() {
        

        XCTAssertEqual(runFonduOnRelativePath(path: "SampleFiles/PTSans-Regular.otf.dfont", currentURL: currentURL),
                       1,
                       "fondu did not return 1")

        // we are expecting files in the output folder -- PTSans.otf and PTSansItalic.otf
        
        
        
    }

    func testPerformanceExample() {
        // This is an example of a performance test case.
        self.measure {
            // Put the code you want to measure the time of here.
        }
    }

}
