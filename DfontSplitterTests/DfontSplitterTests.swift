//
//  DfontSplitterTests.swift
//  DfontSplitterTests
//
//  Created by Peter Upfold on 09/11/2018.
//  Copyright © 2018-2023 Peter Upfold. All rights reserved.
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
    
    func testPTSansSuitcase() {
        // extract the zip -- we must do this with macOS native tools that will extract the rsrc fork properly.
        // SampleFiles/PTSansFontSuitcase.zip
        let task = Process()
        task.launchPath = "/usr/bin/ditto"
        task.arguments = ["-x", "-k", "SampleFiles/PTSansFontSuitcase.zip", "SampleFiles"]
        task.currentDirectoryURL = currentURL
        task.launch()
        task.waitUntilExit()
        
        XCTAssertEqual(task.terminationStatus, 0, "ditto did not successfully extract the Font Suitcase zip")
        
        XCTAssertEqual(runFonduOnRelativePath(path: "SampleFiles/PTSans", currentURL: currentURL),
                       1,
                       "fondu did not return 1")
        
        // we are expecting files in the output folder -- PTSans.ttf, PTSansBold.ttf, PTSansBoldItalic.ttf, PTSansCaption.ttf, PTSansCaptionBold.ttf, PTSansItalic.ttf, PTSansNarrow.ttf, PTSansNarrowBold.ttf
        
        let ptSansTTFPath = tempURL.appendingPathComponent("PTSans.ttf")
        let ptSansBoldTTFPath = tempURL.appendingPathComponent("PTSansBold.ttf")
        let ptSansBoldItalicTTFPath = tempURL.appendingPathComponent("PTSansBoldItalic.ttf")
        let ptSansCaptionPath = tempURL.appendingPathComponent("PTSansCaption.ttf")
        let ptSansCaptionBoldPath = tempURL.appendingPathComponent("PTSansCaptionBold.ttf")
        let ptSansItalicPath = tempURL.appendingPathComponent("PTSansItalic.ttf")
        let ptSansNarrowTTFPath = tempURL.appendingPathComponent("PTSansNarrow.ttf")
        let ptSansNarrowBoldTTFPath = tempURL.appendingPathComponent("PTSansNarrowBold.ttf")
        
        XCTAssertEqual(getSHA256HexString(url: ptSansTTFPath), "bd39569d17eea12d4c9c5390da1ab519184328354f9ef75db7f5e6011e3d396e", "PTSans.ttf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansBoldTTFPath), "162d387a347b6e6c81b5439731c7075ea64f1185a169fe831e02577fc9bb5adb", "PTSansBold.ttf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansBoldItalicTTFPath), "fe0292b98440fcfe7f942fb83f79562b750330a1efa85809bb8cde57c7442320", "PTSansBold.ttf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansCaptionPath), "e0eceda826f3147024f8b40db4da6094c09140371508eb889323d04ef657a9fb", "PTSansBold.ttf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansCaptionBoldPath), "05f2b654f6cc04059150fc24625f856e3c71364d23374131e9a4a4998e27e0c2", "PTSansBold.ttf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansItalicPath), "f641597430bae9b5151f0595c7f5aa06f61bc9f19b31dd0a59bc29a787dea474", "PTSansBold.ttf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansNarrowTTFPath), "bf6dc13c29d8431e704eb2247abd7c96f56451cfb2db2bd5615435cf53583225", "PTSansBold.ttf did not match expected")
        XCTAssertEqual(getSHA256HexString(url: ptSansNarrowBoldTTFPath), "97fe6d9726c2466b46f1dac07c2b4d0aaa3f160b0319f9117c63bdb2048888db", "PTSansBold.ttf did not match expected")
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
