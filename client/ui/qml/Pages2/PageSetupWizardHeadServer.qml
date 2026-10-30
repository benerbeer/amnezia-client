import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Connections {
        target: HeadServerController

        function onCheckFinished(updated, message) {
            PageController.showBusyIndicator(false)

            if (!updated && message !== "") {
                PageController.showErrorMessage(message)
                return
            }

            PageController.goToPageHome()
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Head server")
                description: qsTr("Enter the head server address, your phone number and the PIN issued by the administrator")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 12

                TextFieldType {
                    id: urlField

                    Layout.fillWidth: true

                    headerText: qsTr("Head server address")
                    textField.placeholderText: "example.com"
                    textField.text: HeadServerController.url
                    textField.inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                    checkEmptyText: true
                }

                TextFieldType {
                    id: phoneField

                    Layout.fillWidth: true

                    headerText: qsTr("Phone number")
                    textField.placeholderText: "+7..."
                    textField.text: HeadServerController.phone
                    textField.inputMethodHints: Qt.ImhDialableCharactersOnly
                    checkEmptyText: true
                }

                TextFieldType {
                    id: pinField

                    Layout.fillWidth: true

                    headerText: qsTr("PIN")
                    textField.text: HeadServerController.pin
                    textField.echoMode: TextInput.Password
                    textField.inputMethodHints: Qt.ImhDigitsOnly
                    checkEmptyText: true
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonType {
            id: continueButton

            Layout.fillWidth: true

            text: qsTr("Continue")
            enabled: !HeadServerController.isBusy

            onClicked: {
                var url = urlField.textField.text.trim()
                var phone = phoneField.textField.text.trim()
                var pin = pinField.textField.text.trim()

                if (url === "" || phone === "" || pin === "") {
                    PageController.showErrorMessage(qsTr("Fill in all the fields"))
                    return
                }

                HeadServerController.url = url
                HeadServerController.phone = phone
                HeadServerController.pin = pin

                PageController.showBusyIndicator(true)
                HeadServerController.checkNow()
            }
        }
    }
}
