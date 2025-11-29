

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import Luminism
import QtQuick.Layouts

Rectangle {
    id: rectangle
    width: Constants.width
    height: Constants.height
    color: "#a4aeb4"

    radius: 0

    Pane {
        id: title_bar_pane
        height: 50
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0
        padding: 0

        Rectangle {
            id: rectangle1
            color: "#8ac1e1"
            anchors.fill: parent
            transformOrigin: Item.TopLeft
        }
    }

    Pane {
        id: status_bar_pane
        y: 1050
        height: 30
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        padding: 0
        transformOrigin: Item.BottomLeft

        Rectangle {
            id: rectangle2
            color: "#cae4f3"
            anchors.fill: parent
            transformOrigin: Item.TopLeft
        }
    }

    RowLayout {
        id: body_row_layout
        x: 0
        y: 50
        width: 1920
        height: 1000
        spacing: 0

        ColumnLayout {
            id: nav_area_column_layout
            width: 100
            height: 100
            spacing: 0

            Pane {
                id: project_settings_pane
                width: 600
                height: 200
                Layout.preferredHeight: 200
                Layout.preferredWidth: 384
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Pane {
                id: nave_area_separator_pane1
                width: 200
                height: 200
                rightPadding: 24
                leftPadding: 24
                Layout.preferredHeight: 2
                Layout.preferredWidth: 384
                padding: 0

                Rectangle {
                    id: nav_area_separator1
                    color: "#a4aeb4"
                    anchors.fill: parent
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.preferredWidth: parent.width - 48
                    Layout.preferredHeight: 2
                }
            }

            Pane {
                id: filter_pane
                width: 200
                height: 200
                Layout.preferredHeight: 300
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Pane {
                id: nave_area_separator_pane2
                width: 200
                height: 200
                rightPadding: 24
                padding: 0
                leftPadding: 24
                Rectangle {
                    id: nav_area_separator2
                    color: "#a4aeb4"
                    anchors.fill: parent
                    Layout.preferredWidth: parent.width - 48
                    Layout.preferredHeight: 2
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                }
                Layout.preferredWidth: 384
                Layout.preferredHeight: 2
            }

            Pane {
                id: tag_library_pane
                width: 200
                height: 200
                Layout.preferredHeight: 300
                Layout.fillHeight: true
                Layout.fillWidth: true

                Flow {
                    id: tag_library_flow
                    anchors.fill: parent
                    spacing: 5
                    padding: 0

                    Tag_pill {
                        id: tag_pill1
                    }

                    Tag_pill {
                        id: tag_pill
                    }
                }
            }
        }

        Pane {
            id: body_row_separator_pane1
            width: 1
            height: 1000
            padding: 0

            Rectangle {
                id: rectangle3
                color: "#a4aeb4"
                anchors.fill: parent
            }
        }

        ColumnLayout {
            id: file_list_area_column_layout
            width: 100
            height: 100
            spacing: 0

            Pane {
                id: file_list_header_pane
                width: 800
                height: 200
                Layout.preferredHeight: 100
                Layout.preferredWidth: 768
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Pane {
                id: file_list_pane
                width: 200
                height: 200
                Layout.preferredHeight: 800
                Layout.fillHeight: true
                Layout.fillWidth: true

                Grid {
                    id: file_list_grid
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 0
                    anchors.rightMargin: 0
                    anchors.topMargin: 0
                    anchors.bottomMargin: 0
                    spacing: 10

                    Image_preview {
                        id: image_preview_1
                    }

                    Image_preview {
                        id: image_preview_2
                    }
                }
            }

            Pane {
                id: tag_assignment_pane
                width: 200
                height: 200
                Layout.preferredHeight: 300
                Layout.fillHeight: true
                Layout.fillWidth: true

                Flow {
                    id: tag_assignment_flow
                    anchors.fill: parent
                    spacing: 5

                    Tag_pill {
                        id: tag_pill2
                    }

                    Tag_pill {
                        id: tag_pill3
                    }

                    Tag_pill {
                        id: tag_pill4
                    }
                }
            }
        }

        Pane {
            id: body_row_separator_pane2
            width: 1
            height: 1000
            Layout.preferredHeight: 1000
            Layout.preferredWidth: 1
            padding: 0

            Rectangle {
                id: rectangle4
                x: -385
                y: 0
                color: "#a4aeb4"
                anchors.fill: parent
            }
        }

        ColumnLayout {
            id: preview_area_column_layout
            width: 100
            height: 100
            spacing: 0

            Pane {
                id: preview_pane
                width: 800
                height: 200
                Layout.preferredHeight: 900
                Layout.preferredWidth: 768
                Layout.fillHeight: true
                Layout.fillWidth: true

                Image {
                    id: preview_image
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 0
                    anchors.rightMargin: 0
                    anchors.topMargin: 0
                    anchors.bottomMargin: 0
                    source: "../SampleFiles/Hilde at the Dragon.jpg"
                    fillMode: Image.PreserveAspectFit
                }
            }

            Pane {
                id: preview_properties_pane
                width: 200
                height: 200
                Layout.preferredHeight: 100
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
}
