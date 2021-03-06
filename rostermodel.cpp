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


#include "rostermodel.h"

RosterModel::RosterModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

bool RosterModel::findIndex(QString convId, int &idx)
{
    int i = 0;
    bool found = false;
    foreach (ConvAbstract *ca, conversations) {
        if (ca->convId==convId) {
            found = true;
            break;
        }
        i++;
    }
    idx = i;
    return found;
}

QHash<int, QByteArray> RosterModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
        roles.insert(ConvIdRole, QByteArray("id"));
        roles.insert(NameRole, QByteArray("name"));
        roles.insert(PartnumRole, QByteArray("participantsNum"));
        roles.insert(UnreadRole, QByteArray("unread"));
        roles.insert(ImageRole, "imagePath");
        roles.insert(NotificationLevelRole, "notifLevel");
        return roles;
}

int RosterModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return conversations.size();
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    //Reverse the order so that the most recent conv is displayed on top
    ConvAbstract * conv = conversations.at(conversations.size() - index.row() - 1);
    if (role == NameRole)
        return QVariant::fromValue(conv->name);
    else if (role == PartnumRole)
        return QVariant::fromValue(conv->participantsNum);
    else if (role == ConvIdRole)
        return QVariant::fromValue(conv->convId);
    else if (role == UnreadRole)
        return QVariant::fromValue(conv->unread);
    else if (role == ImageRole) {
        if (conv->imagePaths.size()) { // TODO once we should support multiple images
            return conv->imagePaths;
        }
        else {
            return "";
        }
    }

    else if (role == NotificationLevelRole) {
            return QVariant::fromValue(conv->notificationLevel);
    }

    return QVariant();
}

void RosterModel::setMySelf(User pmyself)
{
   myself = pmyself;
}


void RosterModel::addConversationAbstract(Conversation pConv)
{
    //Simply skip archived conversations, or left group conversations, for now, but only for the UI; they are correctly parsed
    if (pConv.view == ARCHIVED_VIEW || pConv.status == LEFT)
        return;

    QString name = "";
    QStringList imagePaths;

    foreach (Participant p, pConv.participants) {
        if (p.user.chat_id!=myself.chat_id) {
            if (!p.user.photo.isEmpty()) {
                QString image = p.user.photo;
                if (!image.startsWith("https:")) image.prepend("https:");
                    imagePaths << image;
            }
            else {
                imagePaths << "qrc:///icons/unknown.png";
            }
        }
    }

    if (pConv.name.size() > 0) {
        name = pConv.name;
    } else {
        foreach (Participant p, pConv.participants) {
            if (p.user.chat_id!=myself.chat_id) {
                if ((pConv.participants.last().user.chat_id != p.user.chat_id && pConv.participants.last().user.chat_id != myself.chat_id) ||
                        ((pConv.participants.last().user.chat_id == myself.chat_id) && pConv.participants[pConv.participants.size() - 2].user.chat_id != p.user.chat_id)
                        )
                    name += QString(p.user.display_name + ", ");
                else
                    name += QString(p.user.display_name + " ");
            }
        }
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    conversations.append(new ConvAbstract(pConv.id, name, imagePaths, pConv.participants.size(), pConv.unread, pConv.notifLevel));
    endInsertRows();
}

bool RosterModel::conversationExists(QString convId)
{
    foreach (ConvAbstract *c, conversations) {
        if (c->convId == convId)
            return true;
    }
    return false;
}

void RosterModel::addUnreadMsg(QString convId)
{
    int i;
    bool found = findIndex(convId, i);
    if (found) {
        conversations[i]->unread += 1;
        //put the referenced conversation on top of the list
        if (i!=conversations.length()-1) {
            beginResetModel();
            conversations.move(i,conversations.length()-1);
            endResetModel();
        }
        else {
            //No need to reset model
            QModelIndex r1 = index(conversations.length() - 1 - i);
            emit dataChanged(r1, r1);
        }
    }
}

bool RosterModel::hasUnreadMessages(QString convId)
{
    foreach (ConvAbstract *c, conversations) {
        if (c->convId == convId)
        {
            return (c->unread > 0);
        }
     }
    return false;
}

void RosterModel::putOnTop(QString convId)
{
    int i;
    bool found = findIndex(convId, i);
    if (found && i!=conversations.length()-1) {
        beginResetModel();
        qDebug() << "Moving";
        conversations.move(i,conversations.length()-1);
        qDebug() << "Moved";
        endResetModel();
    }
}

void RosterModel::setReadConv(QString convId)
{
    int i;
    bool found = findIndex(convId, i);
    if (found) {
        conversations[i]->unread = 0;
        QModelIndex r1 = index(conversations.length() - 1 - i);
        emit dataChanged(r1, r1);
    }
}

QString RosterModel::getConversationName(QString convId) {
    foreach (ConvAbstract *ca, conversations) {
        if (ca->convId==convId)
            return ca->name;
    }
    return "Conv not found";
}

void RosterModel::renameConversation(QString convId, QString newName)
{
    int i;
    bool found = findIndex(convId, i);
    if (found) {
        qDebug() << "Found";
        if (newName.size() > 0) {
            conversations[i]->name = newName;
        } else {
            conversations[i]->name = "No name - restart hangish to get participants";
            /* TODO: retrieve contacts names to restore previous name
            foreach (Participant p, conversations[i]->participants) {
                if (p.user.chat_id!=myself.chat_id) {
                    if (conversations[i]->participants.last().user.chat_id != p.user.chat_id && conversations[i]->participants.last().user.chat_id != myself.chat_id)
                        conversations[i]-> += QString(p.user.display_name + ", ");
                    else
                        conversations[i]-> += QString(p.user.display_name + " ");
                }
            }
            */
        }

        //put the referenced conversation on top of the list
        if (i!=conversations.length()-1) {
            beginResetModel();
            conversations.move(i,conversations.length()-1);
            endResetModel();
        }
        else {
            //No need to reset model
            QModelIndex r1 = index(conversations.length() - 1 - i);
            emit dataChanged(r1, r1);
        }
    }

}

void RosterModel::deleteConversation(QString convId)
{
    qDebug() << "Del conv from list " << convId;
    int i;
    bool found = findIndex(convId, i);
    if (found) {
        qDebug() << "Found";
        beginRemoveRows(QModelIndex(), i, i);
        conversations.removeAt(i);
        endRemoveRows();
        //TODO: check why this is the only (wrong) way to make it work!
        for (int i=0; i<conversations.size(); i++) {
            QModelIndex r1 = index(i);
            emit dataChanged(r1, r1);
        }
    }
}

void RosterModel::updateNotificationLevel(QString convId, int newLevel)
{
    int i;
    bool found = findIndex(convId, i);
    if (found && conversations[i]->notificationLevel != newLevel) {
        qDebug() << "Found, updating";
        conversations[i]->notificationLevel = newLevel;
        QModelIndex r1 = index(conversations.length() - 1 - i);
        emit dataChanged(r1, r1);
    }
}
