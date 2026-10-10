// Copyright (C) 2023-2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Treeland
import LockScreen

FocusScope {
    id: root
    clip: true

    required property QtObject output
    required property QtObject outputItem
    visible: true

    x: outputItem.x
    y: outputItem.y
    width: outputItem.width
    height: outputItem.height

    palette.windowText: Qt.rgba(1.0, 1.0, 1.0, 1.0)

    /**************/
    /* Components */
    /**************/

    Rectangle {
        id: background

        readonly property int duration: 1000
        property real wallpaperBrightness: 0.0
        property real wallpaperSaturation: 0.0
        anchors.fill: parent
        clip: true
        color: 'black'
        transformOrigin: Item.Center
        state: {
            if (GreeterProxy.isLocked || GreeterProxy.undecided)
                return "Show"
            if (GreeterProxy.showShutdownView)
                return "ShowWithoutScale"
            return "Hide"
        }
        states: [
            State {
                name: "Show"
                PropertyChanges {
                    target: background
                    scale: 1.2
                }
                PropertyChanges {
                    target: background
                    wallpaperBrightness: -0.25
                }
                PropertyChanges {
                    target: background
                    wallpaperSaturation: 0.10
                }
            },
            State {
                name: "ShowWithoutScale"
                PropertyChanges {
                    target: background
                    scale: 1
                }
                PropertyChanges {
                    target: background
                    wallpaperBrightness: -0.25
                }
                PropertyChanges {
                    target: background
                    wallpaperSaturation: 0.10
                }
            },
            State {
                name: "Hide"
                PropertyChanges {
                    target: background
                    scale: 1
                }
                PropertyChanges {
                    target: background
                    wallpaperBrightness: 0
                }
                PropertyChanges {
                    target: background
                   wallpaperSaturation: 0.0
                }
            }
        ]

        transitions: [
            Transition {
                from: "*"
                to: "Show"
                PropertyAnimation {
                    property: "scale"
                    duration: background.duration
                    easing.type: Easing.OutExpo
                }
                PropertyAnimation {
                    property: "wallpaperBrightness"
                    duration: background.duration
                    easing.type: Easing.InOutCubic
                }
               PropertyAnimation {
                   property: "wallpaperSaturation"
                    duration: background.duration
                   easing.type: Easing.InOutCubic
               }
            },
            Transition {
                from: "*"
                to: "ShowWithoutScale"
                PropertyAnimation {
                    property: "scale"
                    duration: background.duration
                    easing.type: Easing.OutExpo
                }
                PropertyAnimation {
                    property: "wallpaperBrightness"
                    duration: background.duration
                    easing.type: Easing.InOutCubic
                }
                PropertyAnimation {
                   property: "wallpaperSaturation"
                   duration: background.duration
                   easing.type: Easing.InOutCubic
                }
            },
            Transition {
                from: "*"
                to: "Hide"
                PropertyAnimation {
                    property: "scale"
                    duration: background.duration
                    easing.type: Easing.OutExpo
                }
                PropertyAnimation {
                    property: "wallpaperBrightness"
                    duration: background.duration
                    easing.type: Easing.InOutCubic
                }
                PropertyAnimation {
                    property: "wallpaperSaturation"
                  duration: background.duration
                   easing.type: Easing.InOutCubic
               }
            }
        ]
        onStateChanged: {
            if (state === "Hide") {
                wallpaper.play = false;
                Helper.showDesktop(root.output)
            } else {
                Helper.startLockscreen(root.output, state === "Show");
                wallpaper.play = true;
            }
        }

        Wallpaper {
            id: wallpaper

            anchors.fill: parent
            clip: true
            wallpaperRole: Wallpaper.Lockscreen
            output: root.output
            visible: false
        }

        MultiEffect {
            id: wallpaperEffect

            anchors.fill: parent
            clip: true
            visible: true

            source: wallpaper
            brightness: background.wallpaperBrightness
            saturation: background.wallpaperSaturation
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: true
    }

}
