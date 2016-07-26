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


#include "conversationmodel.h"

ConversationModel::ConversationModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

void ConversationModel::addConversation(Conversation c)
{
    conversations.append(c);
}

void ConversationModel::deleteMsg(QNetworkReply *id)
{
    foreach (ConversationElement *ce, myList) {
        if (ce->id == id) {
            myList.removeOne(ce);
        }
    }
    for (int i=0; i<myList.size(); i++) {
        QModelIndex r1 = index(i);
        emit dataChanged(r1, r1);
    }
}

void ConversationModel::addErrorMessage(QNetworkReply *id, QString convId, Event evt)
{
    qDebug() << "Signaling the message as error!";
    foreach (ConversationElement *ce, myList) {
        if (ce->id == id) {
            ce->timestamp = "Error, msg not sent!";
        }
    }
    for (int i=0; i<myList.size(); i++) {
        QModelIndex r1 = index(i);
        emit dataChanged(r1, r1);
    }
}

void ConversationModel::addSentMessage(QNetworkReply *id, QString convId, Event evt)
{
    qDebug() << "Signaling the message as sent!";
    foreach (ConversationElement *ce, myList) {
        if (ce->id == id) {
            ce->timestamp = "Sent!";
            ce->uniqueId = evt.uniqueId;
        }
    }
    for (int i=0; i<myList.size(); i++) {
        QModelIndex r1 = index(i);
        emit dataChanged(r1, r1);
    }
}

void ConversationModel::addOutgoingMessage(QNetworkReply *id, QString convId, Event evt)
{
    qDebug() << "Adding outgoing message " << id;
    addConversationElement(id, "", evt.sender.chat_id, "Sending...", evt.value.segments[0].value, "", "", "", false, evt.timestamp, 0, true, evt.uniqueId);
}

bool ConversationModel::addEventToConversation(QString convId, Event e, bool bottom)
{
    int i=0;
    for (i=0; i<conversations.size(); i++)
        if (conversations.at(i).id==convId)
            break;
    if (bottom)
        conversations[i].events.append(e);
    else
        conversations[i].events.prepend(e);
    if (convId == id) {
        //It's the active one, should update model
        QString snd = getSenderName(e.sender.chat_id, conversations[i].participants);

        QString text = "";
        foreach (EventValueSegment evs, e.value.segments)
            text += evs.value;
        QString fImage = "";
        foreach (EventAttachmentSegment eas, e.value.attachments)
            fImage += eas.fullImage;
        QString pImage = "";
        foreach (EventAttachmentSegment eas, e.value.attachments)
            pImage += eas.previewImage;
        QString uVideo = "";
        foreach (EventAttachmentSegment eas, e.value.attachments)
            uVideo += eas.video;


        //Show full date if event has not happened today; otherwise show only clock time
        QString ts_string;
        QDateTime now = QDateTime::currentDateTime();
        if (now.date().day() != e.timestamp.date().day())
            ts_string = e.timestamp.toString();
        else
            ts_string = e.timestamp.time().toString();

        //Where to add this message? Bottom -> new msg / Top -> old msg
        //If it is added to the bottom, I also wanna know whether it's a duplicate
        if (bottom)
            return addConversationElement(NULL, snd, e.sender.chat_id, ts_string, text, fImage, pImage, uVideo, false, e.timestamp, e.type, e.isMine, e.uniqueId);
        else
            prependConversationElement(snd, e.sender.chat_id, ts_string, text, fImage, pImage, uVideo, false, e.timestamp, e.type, e.uniqueId);
    }
    return true;
}

QHash<int, QByteArray> ConversationModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
        roles.insert(IdRole, QByteArray("id"));
        roles.insert(SenderRole, QByteArray("sender"));
        roles.insert(SenderIdRole, QByteArray("senderId"));
        roles.insert(TextRole, QByteArray("msgtext"));
        roles.insert(FullImageRole, QByteArray("fullimage"));
        roles.insert(PreviewImageRole, QByteArray("previewimage"));
        roles.insert(VideoRole, QByteArray("video"));
        roles.insert(TimestampRole, QByteArray("timestamp"));
        roles.insert(ReadRole, QByteArray("read"));
        return roles;
}

QString ConversationModel::getCid()
{
    return id;
}

void ConversationModel::conversationLoaded()
{
    qDebug() << "Conv load finished! ";
    qDebug() << myList.size();
}

QString ConversationModel::getSenderName(QString chatId, QList<Participant> participants)
{
    foreach (Participant p, participants)
        if (p.user.chat_id == chatId)
            return p.user.first_name;
    //If this sender left the conversation, I should still return something
    return "Unknown";
}

void ConversationModel::updateReadState(ReadState rs)
{
    qDebug() << "Updating read state: " << rs.convId << ", " << rs.userid.chat_id << " at " << rs.last_read.toString();
    bool found = false;
    int i=0, j=0;
    foreach (Conversation c, conversations) {
        if (found)
            break;
        if (c.id == rs.convId) {
            foreach (Participant p, c.participants) {
                if (p.user.chat_id == rs.userid.chat_id) {
                    //p.last_read_timestamp = rs.last_read;
                    conversations[i].participants[j].last_read_timestamp = rs.last_read;
                    found = true;
                    //EMIT SIGNAL?
                    break;
                }
                j++;
            }
        }
        i++;
    }

    //update data model if it refers to the active conv
    if (rs.convId == id) {
        bool modified = false;
        //Get a handle to the conv
        Conversation conv;
        foreach (Conversation c, conversations) {
            if (c.id == rs.convId) {
                conv = c;
                break;
            }
        }
        foreach (ConversationElement *ce, myList) {
            bool read = true;
            foreach (Participant p, conv.participants) {
                if (p.last_read_timestamp.toMSecsSinceEpoch()>0 && (p.last_read_timestamp < ce->ts)) {
                    read = false;
                    break;
                }
            }
            if (read && !ce->read) {
                ce->read = true;
                modified = true;
            }
        }
        if (modified) {
            //TODO: check why this is the only (wrong) way to make it work!
            for (int i=0; i<myList.size(); i++) {
                QModelIndex r1 = index(i);
                emit dataChanged(r1, r1);
            }
        }
    }
}

void ConversationModel::loadConversation(QString cId)
{
    id = cId;

    //In this case the user went back to the roster page; no need to really reset the model yet
    if (id == "")
        return;

    beginResetModel();
    qDeleteAll(myList);
    myList.clear();
    endResetModel();

    foreach (Conversation c, conversations) {
        if (c.id==cId) {
            name = c.name;
            id = c.id;
            foreach (Event e, c.events)
            {
                QString snd = getSenderName(e.sender.chat_id, c.participants);
                QString text = "";
                foreach (EventValueSegment evs, e.value.segments)
                    text += evs.value;
                QString fImage = "";
                foreach (EventAttachmentSegment eas, e.value.attachments)
                    fImage += eas.fullImage;
                QString pImage = "";
                foreach (EventAttachmentSegment eas, e.value.attachments)
                    pImage += eas.previewImage;
                QString uVideo = "";
                foreach (EventAttachmentSegment eas, e.value.attachments)
                    uVideo += eas.video;

                //Show full date if event has not happened today; otherwise show only clock time
                QString ts_string;
                QDateTime now = QDateTime::currentDateTime();
                if (now.date().day() != e.timestamp.date().day())
                    ts_string = e.timestamp.toString();
                else
                    ts_string = e.timestamp.time().toString();

                bool read = true;
                foreach (Participant p, c.participants) {
                    if (p.last_read_timestamp.toMSecsSinceEpoch() > 0 && p.last_read_timestamp < e.timestamp) {
                        read = false;
                        break;
                    }
                }
                addConversationElement(NULL, snd, e.sender.chat_id, ts_string, text, fImage, pImage, uVideo, read, e.timestamp, e.type, false, e.uniqueId);
            }
            break;
        }
    }
}

bool ConversationModel::addConversationElement(QNetworkReply *id, QString sender, QString senderId, QString timestamp, QString text, QString fullimageUrl, QString previewimageUrl, QString pvideo, bool read, QDateTime pts, int type, bool isMine, QString uid)
{
    //if id is NULL this message comes from the channel; and if it is mine, I want to check if I have already the same message shown as outgoing/error/sent
    int i = 0;
    bool found = false;

    //Don't add duplicates, messages in outgoing/error/sent states have a different timestamp, so they shouldn't be affected
    foreach (ConversationElement *ce, myList) {
        if (ce->timestamp == timestamp) {
            //This must be a duplicate, skip it
            return false;
        }
    }

    if (id==NULL && isMine) {
        foreach (ConversationElement *ce, myList) {
            if (ce->uniqueId == uid /*||             if (ce->text == text && ce->senderId == senderId && ce->previewImageUrl == previewimageUrl && ce->fullimageUrl == fullimageUrl && ce->video == pvideo) {*/) {
                found = true;
                break;
            }
            i++;
        }
        if (found) {
            beginRemoveRows(QModelIndex(), i,i);
            myList.removeAt(i);
            endRemoveRows();
        }
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    myList.append(new ConversationElement(id, sender, senderId, text, timestamp, fullimageUrl, previewimageUrl, pvideo, read, pts, type, uid));
    endInsertRows();
    return true;
}

void ConversationModel::prependConversationElement(QString sender, QString senderId, QString timestamp, QString text, QString fullimageUrl, QString previewimageUrl, QString pvideo, bool read, QDateTime pts, int type, QString uid)
{
    beginInsertRows(QModelIndex(), 0, 0);
    myList.prepend(new ConversationElement(NULL, sender, senderId, text, timestamp, fullimageUrl, previewimageUrl, pvideo, read, pts, type, uid));
    endInsertRows();
}

void ConversationModel::deleteMsgWError(QString text)
{
    int i = 0;
    bool found = false;
    foreach (ConversationElement *ce, myList) {
        if (ce->text == text && ce->timestamp == "Error, msg not sent!") {
            found = true;
            break;
        }
        i++;
    }
    if (found) {
        beginRemoveRows(QModelIndex(), i,i);
        myList.removeAt(i);
        endRemoveRows();
    }
}

int ConversationModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return myList.size();
}

QVariant ConversationModel::data(const QModelIndex &index, int role) const {
    ConversationElement * fobj = myList.at(index.row());
    if (role == SenderRole)
        return QVariant::fromValue(fobj->sender.toHtmlEscaped());
    else if (role == TextRole) {
        //Escape the string only if it is a text message
        if (fobj->type == 0) {
            return QVariant::fromValue(fobj->text.toHtmlEscaped());
        }
        else {
            return QVariant::fromValue(fobj->text);
        }
    }
    else if (role == SenderIdRole)
        return QVariant::fromValue(fobj->senderId);
    else if (role == TimestampRole)
        return QVariant::fromValue(fobj->timestamp);
    else if (role == FullImageRole)
        return QVariant::fromValue(fobj->fullimageUrl);
    else if (role == PreviewImageRole)
        return QVariant::fromValue(fobj->previewImageUrl);
    else if (role == VideoRole)
        return QVariant::fromValue(fobj->video);
    else if (role == ReadRole)
        return QVariant::fromValue(fobj->read);

    return QVariant();
}

QDateTime ConversationModel::getFirstEventTs(QString convId) {
    foreach (Conversation c, conversations) {
        if (c.events.size() && c.id == convId)
            return c.events[0].timestamp;
    }
    return QDateTime::currentDateTime();
}
