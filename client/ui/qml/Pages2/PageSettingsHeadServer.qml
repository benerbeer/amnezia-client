import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool manualUpdateCheck: false

    Connections {
        target: HeadServerController

        function onUpdateCheckFinished(found, message) {
            if (!root.manualUpdateCheck) {
                return
            }
            root.manualUpdateCheck = false
            if (message !== "") {
                PageController.showNotificationMessage(message)
            } else if (!found) {
                PageController.showNotificationMessage(qsTr("The application is up to date"))
            }
        }

        function onCheckFinished(updated, message) {
            if (message !== "") {
                PageController.showNotificationMessage(message)
            } else if (updated) {
                PageController.showNotificationMessage(qsTr("VPN server settings updated"))
            } else {
                PageController.showNotificationMessage(qsTr("VPN server address is up to date"))
            }
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: buttons.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Head server")
                description: qsTr("The address of the VPN server is checked before every connection")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                spacing: 12

                TextFieldType {
                    id: urlField

                    Layout.fillWidth: true

                    headerText: qsTr("Head server address")

                    textField.text: HeadServerController.url
                    textField.placeholderText: "https://example.com"
                }

                TextFieldType {
                    id: phoneField

                    Layout.fillWidth: true

                    headerText: qsTr("Phone number")

                    textField.text: HeadServerController.phone
                    textField.placeholderText: "+7..."
                    textField.inputMethodHints: Qt.ImhDialableCharactersOnly
                }

                TextFieldType {
                    id: pinField

                    Layout.fillWidth: true

                    headerText: qsTr("PIN")

                    textField.text: HeadServerController.pin
                    textField.echoMode: TextInput.Password
                    textField.inputMethodHints: Qt.ImhDigitsOnly
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                spacing: 12

                visible: HeadServerController.updateSupported

                BodyTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12

                    text: qsTr("Application update")
                }

                SmallTextType {
                    Layout.fillWidth: true

                    visible: HeadServerController.updateAvailable
                    text: qsTr("Version %1 (%2) is available").arg(HeadServerController.updateVersion).arg(HeadServerController.updateSizeText)
                }

                SmallTextType {
                    Layout.fillWidth: true

                    visible: HeadServerController.updateAvailable && HeadServerController.updateNotes !== ""
                    text: HeadServerController.updateNotes
                }

                ButtonType {
                    Layout.fillWidth: true

                    variant: HeadServerController.updateAvailable ? ButtonType.Primary : ButtonType.Secondary
                    enabled: !HeadServerController.updateChecking && !HeadServerController.updateDownloading
                    text: HeadServerController.updateDownloading ? qsTr("Downloading… %1%").arg(HeadServerController.updateProgress)
                                                                 : (HeadServerController.updateAvailable ? qsTr("Update") : qsTr("Check for updates"))

                    onClicked: {
                        if (HeadServerController.updateAvailable) {
                            HeadServerController.installUpdate()
                        } else {
                            root.manualUpdateCheck = true
                            HeadServerController.checkForUpdate()
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: buttons

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        spacing: 12

        ButtonType {
            id: checkButton

            Layout.fillWidth: true

            variant: ButtonType.Secondary
            text: qsTr("Check now")
            enabled: !HeadServerController.isBusy

            onClicked: {
                var url = urlField.textField.text
                var phone = phoneField.textField.text
                var pin = pinField.textField.text

                HeadServerController.url = url
                HeadServerController.phone = phone
                HeadServerController.pin = pin
                HeadServerController.checkNow()
            }
        }

        ButtonType {
            id: saveButton

            Layout.fillWidth: true

            text: qsTr("Save")

            onClicked: {
                var url = urlField.textField.text
                var phone = phoneField.textField.text
                var pin = pinField.textField.text

                HeadServerController.url = url
                HeadServerController.phone = phone
                HeadServerController.pin = pin
                PageController.showNotificationMessage(qsTr("Settings saved"))
            }
        }
    }
}
