import QtQuick 2.12
import QtQuick.Window 2.12
import Qt.labs.settings 1.0

Window {
    id: app
    flags: Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint | Qt.WA_TranslucentBackground
    visible: true
    color: "#80000000"
    title: qsTr("Hello World")
    property real previousBalance: 0
    property real balance: 0
    width: 200
    height: 200

    onBalanceChanged: {
        console.log("balance : " + balance)
        let diff = balance - previousBalance;
        previousBalance = balance;
        diffText.setDiff(diff);
    }

    function checkWindowBoundaries(){
        if(x > Screen.desktopAvailableWidth - width){
            x = Screen.desktopAvailableWidth - width
        }
        else if(x < 0){
            x = 0
        }

        if(y > Screen.desktopAvailableHeight - height){
            y = Screen.desktopAvailableHeight - height
        }
        else if(y < 0){
            y = 0
        }
    }

    onXChanged: {
        if(!mouseArea.pressed){
            checkWindowBoundaries()
        }
    }

    onYChanged: {
        if(!mouseArea.pressed){
            checkWindowBoundaries()
        }
    }



    Text {
        id: balanceTxt
        x: 76
        y: 160
        width: 122
        height: 40
        color: "#ffffff"
        text: (balance).toFixed(2) + " EUR"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        font.pixelSize: 30
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignBottom
        lineHeight: 1
        anchors.bottomMargin: 2
        anchors.rightMargin: 2
    }

    Text {
        id: diffText
        x: 94
        y: 143
        text: qsTr("Text")
        anchors.right: parent.right

        font.pixelSize: 18
        anchors.rightMargin: 71
        anchors.bottomMargin: 0
        NumberAnimation on opacity {
            id: fadeAnimation
            from: 1
            to: 0
            duration: 3000
        }
        NumberAnimation on y {
            id: moveAnimation
            from: balanceTxt.y
            to: app.height /2
            duration: 3000
        }
        function setDiff(diff){
            if(diff > 0){
                diffText.text = "+"+(diff).toFixed(2)
                diffText.color = "#00FF00"
            }
            else{
                diffText.text = (diff).toFixed(2)
                diffText.color = "#FF0000"
            }

            moveAnimation.start();
            fadeAnimation.start();
        }
    }
    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        property real anchorX
        property real anchorY
        onPressed: { anchorX = mouse.x; anchorY = mouse.y }
        onMouseXChanged: if (pressed) app.x += mouse.x-anchorX
        onMouseYChanged: if (pressed) app.y += mouse.y-anchorY
        onPressedChanged: if (!pressed) app.checkWindowBoundaries()
    }

    Settings {
      property alias x: app.x
      property alias y: app.y
      property alias balance: app.balance
    }
}
