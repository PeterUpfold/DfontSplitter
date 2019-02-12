Summary of ArrayController and NSView based table Cocoa bindings
================================================================

NSArrayController has Controller Content bound to ViewController > filenames model key path (@objc decorated field of ViewController.swift)

Table Column > Value is bound to the NSArrayController with controller key arrangedObjects

Table View Cell > Value is bound to Table Cell View > model key path objectValue

(Table Cell View implicitly gets from Table Column?)

It feels like this is not the Swift way, given we have to decorate with @objc for KVC to work...
