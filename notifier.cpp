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
along with hangish.  If not, see <http://www.gnu.org/licenses/>

*/


#include "notifier.h"
#include "adaptor.h"

static const char *SERVICE = "harbour.hangish";
static const char *PATH = "/";

Notifier::Notifier(QObject *parent, ContactsModel *contacts) :
    QObject(parent)
{
    cModel = contacts;
    myParent = parent;
    lastId = 0;

    new HangishAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService(SERVICE)) {
    qDebug() << "Failed to register DBus service";
    return;
    }
    if (!connection.registerObject(PATH, this)) {
    qDebug() << "Failed to register DBus object";
    return;
    }
}

void Notifier::test()
{
    qDebug() << "test succeeded!";
}

void Notifier::notificationPushedIntf(const QString &convId)
{
    emit notificationPushed(convId);
}

void Notifier::closeAllNotifications()
{
    foreach (Notification *n, activeNotifications) {
        n->close();
        delete n;
    }
    activeNotifications.clear();
    emit deletedNotifications();
}

void Notifier::showNotification(QString preview, QString summary, QString body, QString sender, int num, QString convId)
{
    //if acs == 2 another client is active, don't fire notification!
    if (activeClientState == OTHER_CLIENT_IS_ACTIVE) {
        qDebug() << "There's another client active, dropping notification!";
        return;
    }
    //Check notification aspect
    Notification *n = new Notification(myParent);

    n->setCategory("x-nemo.messaging.im");
    //I may want to personalize the notification, need to create and publish a custom hangish.ini in /usr/share/ngfd/events.d/
    n->setHintValue("x-nemo-feedback", "chat");
    //n->setHintValue("x-nemo-feedback", "hangish");
    n->setHintValue("x-nemo-preview-icon", "harbour-hangish");
    //n->setHintValue("x-nemo-priority", 100);
    //n->setHintValue("lowPowerModeIconId", "hangish-notif-icon");
    //n->setHintValue("statusAreaIconId", "hangish-notif-icon");
    n->setItemCount(1);
    n->setBody(body);
    n->setPreviewBody(preview);

    n->setSummary(cModel->getContactDName(sender));

    n->setTimestamp(QDateTime::currentDateTime());

    n->setRemoteDBusCallServiceName("harbour.hangish");
    n->setRemoteDBusCallObjectPath("/");
    n->setRemoteDBusCallInterface("harbour.hangish");
    n->setRemoteDBusCallMethodName("notificationPushedIntf");
    n->setRemoteDBusCallArguments(QVariantList() << convId);
    emit showNotificationForCover(num);

    n->publish();
    lastId = n->replacesId();
    qDebug() << "pubbed " << n->replacesId();
    activeNotifications.append(n);
}

void Notifier::activeClientUpdate(int state)
{
    activeClientState = state;
    if (activeClientState == OTHER_CLIENT_IS_ACTIVE)
        closeAllNotifications();
}

bool Notifier::isAnotherClientActive()
{
    return activeClientState == OTHER_CLIENT_IS_ACTIVE;
}

bool Notifier::amITheActiveClient()
{
    return activeClientState == IS_ACTIVE_CLIENT;
}
