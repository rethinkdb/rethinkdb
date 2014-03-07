// (c) Copyright Juergen Hunold 2012
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

import QtQuick 2.0

Rectangle {
    id: page
    width: 400; height: 200
    color: "#d6d6d6"
    Text {
        id: helloText
        text: "Boost.Build built!"
        color: "darkgray"
        anchors.horizontalCenter: page.horizontalCenter
        anchors.verticalCenter: page.verticalCenter
        font.pointSize: 30; font.italic: true ; font.bold: true
    }
}
