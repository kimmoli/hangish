/*

Hanghish
Copyright (C) 2015 Daniele Rogora

This file is part of Hangish.

Hangish is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Hangish is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>

*/


import QtQuick 2.2
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow
{
    initialPage: Component { FirstPage { } }

    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    Conversation {
            id: conversation
        }

    Roster {
            id: roster
    }

    FullscreenImage {
            id: fsImage
    }

    ImagePicker {
            id: imagepicker
    }

    About {
        id: about
    }

    Options {
        id: options
    }

    Contacts {
        id: contacts
    }

    ConvRenameInput {
        id: convRenameInput
    }
}


