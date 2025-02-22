/*
  The MIT License (MIT)

  Copyright (c) 2014-2022 Andrea Scarpino <andrea@scarpino.dev>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Amber.Mpris 1.0

Page {
    allowedOrientations: Orientation.All

    property bool userTyping: false

    Connections {
        target: manager

        onSearchResult: {
            console.log("Got a lyric response, found: " + found);
            artist.enabled = true;
            song.enabled = true;
            busy.running = false;
            busy.visible = false;
            search.enabled = true;

            if (found) {
                songText.text = lyric.text;
                copy.enabled = true;
            } else {
                songText.text = "Not found :-("
            }
        }
    }

    Connections {
        target: mprisController.metaData

        onTitleChanged: mpris()
    }

    MprisController {
        id: mprisController
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {

            MenuItem {
                text: qsTr("Settings")

                onClicked: pageStack.push(Qt.resolvedUrl("Settings.qml"))
            }

            MenuItem {
                text: qsTr("Clear")

                onClicked: {
                    artist.text = "";
                    artist.forceActiveFocus();
                    song.text = "";
                    songText.text = "";
                    copy.enabled = false;
                }
            }

            MenuItem {
                id: copy
                text: qsTr("Copy to clipboard")
                enabled: songText.text.length > 0

                onClicked: {
                    Clipboard.text = songText.text;
                }
            }
        }

        Column {
            id: column
            x: Theme.horizontalPageMargin
            width: parent.width - Theme.horizontalPageMargin * 2
            spacing: Theme.paddingMedium

            PageHeader {
                title: "Lyrics"
            }

            Label {
                id: poweredBy
                width: parent.width
                font.pixelSize: Theme.fontSizeTiny
                horizontalAlignment: Text.AlignRight
            }

            TextField {
                id: artist
                width: parent.width
                focus: true
                KeyNavigation.tab: song
                placeholderText: qsTr("Artist")

                onTextChanged: search.enabled = (text.length > 0 && song.text.length > 0)
                onActiveFocusChanged: userTyping = artist.activeFocus

                EnterKey.enabled: text.length > 0 && song.text.length > 0
                EnterKey.onClicked: searchLyric();
            }

            TextField {
                id: song
                width: parent.width
                placeholderText: qsTr("Song")

                onTextChanged: search.enabled = (text.length > 0 && artist.text.length > 0)
                onActiveFocusChanged: userTyping = song.activeFocus

                EnterKey.enabled: text.length > 0 && artist.text.length > 0
                EnterKey.onClicked: searchLyric();
            }

            BusyIndicator {
                id: busy
                visible: false
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Button {
                id: search
                text: qsTr("Search")
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: false

                onClicked: searchLyric()
            }

            Text {
                id: songText
                width: parent.width
                color: Theme.secondaryColor
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeMedium
            }
        }

        VerticalScrollDecorator {}
    }

    function searchLyric() {
        songText.text = "";
        artist.enabled = false;
        song.enabled = false;
        busy.visible = true;
        busy.running = true;
        search.enabled = false;
        manager.search(artist.text.trim(), song.text.trim());
    }

    function mpris() {
        if (manager.getMediaPlayerScanner()) {
            console.log("Fetched song informations from " + mprisController.currentService);

            if (!userTyping && mprisController.metaData) {
                artist.text = mprisController.metaData.contributingArtist;
                song.text = mprisController.metaData.title;
                searchLyric();
            }
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            poweredBy.text = qsTr("Powered by %1").arg(manager.getProvider());

            mpris();
        }
    }
}
