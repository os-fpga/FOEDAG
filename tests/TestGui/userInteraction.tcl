namespace eval Keys {
# Qt::Key enum values
    variable Tab 0x01000001
    variable Enter 0x01000004
    variable Up 0x01000013
    variable Key_A 0x41
}

proc sendKeyEvent {widget key} {
    set w [qt_getWidget $widget]
    qt_testWidget $w
	qt_sendKeyEvent $w $key
}

proc sendData {widget data} {
    set w [qt_getWidget $widget]
    qt_testWidget $w
	qt_setWidgetData $w $data
}

proc sendText {widget key text} {
    set w [qt_getWidget $widget]
    qt_testWidget $w
	qt_sendKeyEvent $w $key $text
}

proc EXPECT_EQ {expected actual} {
	if {$expected ne $actual} { error "Test failed. Expect equal. \"$expected\" EXPECTED but ACTUAL is \"$actual\"" }
}

proc EXPECT_NE {expected actual} {
	if {$expected eq $actual} { error "Test failed. Expect NOT equal. \"$expected\" EXPECTED but ACTUAL is \"$actual\"" }
}
