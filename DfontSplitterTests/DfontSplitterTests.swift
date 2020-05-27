//
//  DfontSplitterTests.swift
//  DfontSplitterTests
//
//  Created by Peter Upfold on 09/11/2018.
//  Copyright © 2018 Peter Upfold. All rights reserved.
//

import XCTest
import os
@testable import DfontSplitter

// Before the test will work, the SampleFiles/ directory must be copied to
// ~/Library/Containers/uk.org.upfold.DfontSplitter/Data/SampleFiles

// some tests may require fonts that cannot be redistributed in the open source repository

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

    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }
    
    func testPTSansOTF() {
    
        XCTAssertEqual(runFonduOnRelativePath(path: "SampleFiles/PTSans-Regular.otf.dfont", currentURL: currentURL),
                       1,
                       "fondu did not return 1")

        // we are expecting files in the output folder -- PTSans.otf and PTSansItalic.otf
        let ptSansOTFPath = tempURL.appendingPathComponent("PTSans.otf")
        let ptSansItalicOTFPath = tempURL.appendingPathComponent("PTSansItalic.otf")
        
        XCTAssertEqual(getSHA256HexString(url: ptSansOTFPath), "2101fe4abb8b97da7ec339afa93d4c3575d01a6498abc306e7349d811c128747", "PTSans.otf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansItalicOTFPath), "b84e4f1c297414a4d41d4d555da457d13f98a3a11b5e1612265ef15f9fc9a7c3", "PTSansItalic.otf did not match expected")
    }

    func testPerformanceExample() {
        // This is an example of a performance test case.
        self.measure {
            // Put the code you want to measure the time of here.
        }
    }
    
    /*func testAssertionForFileExt() {
        runFonduOnRelativePath(path: "SampleFiles/test.df", currentURL: currentURL)
    }*/

}
