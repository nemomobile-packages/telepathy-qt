/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <TelepathyQt/BaseChannel>
#include "TelepathyQt/base-channel-internal.h"

#include "TelepathyQt/_gen/base-channel.moc.hpp"
#include "TelepathyQt/_gen/base-channel-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>
#include <TelepathyQt/Utils>
#include <TelepathyQt/AbstractProtocolInterface>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseChannel::Private {
    Private(BaseChannel *parent, const QDBusConnection &dbusConnection, BaseConnection* connection,
            const QString &channelType, uint targetHandle, uint targetHandleType)
        : parent(parent),
          connection(connection),
          channelType(channelType),
          targetHandle(targetHandle),
          targetHandleType(targetHandleType),
          adaptee(new BaseChannel::Adaptee(dbusConnection, parent)) {
        static uint s_channelIncrementalId = 0;

        QString baseName;
        static const QString s_channelTypePrefix = TP_QT_IFACE_CHANNEL + QLatin1String(".Type.");
        if (channelType.startsWith(s_channelTypePrefix)) {
            baseName = channelType.mid(s_channelTypePrefix.length());
        }

        uniqueName = baseName + QLatin1String("Channel") + QString::number(s_channelIncrementalId);
        ++s_channelIncrementalId;
    }

    BaseChannel *parent;
    BaseConnection* connection;
    QString channelType;
    QHash<QString, AbstractChannelInterfacePtr> interfaces;
    QString uniqueName;
    uint targetHandle;
    QString targetID;
    uint targetHandleType;
    bool requested;
    uint initiatorHandle;
    QString initiatorID;
    BaseChannel::Adaptee *adaptee;
};


BaseChannel::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
                              BaseChannel *channel)
    : QObject(channel),
      mChannel(channel)
{
    debug() << "Creating service::channelAdaptor for " << channel->dbusObject();
    mAdaptor = new Service::ChannelAdaptor(dbusConnection, this, channel->dbusObject());
}

BaseChannel::Adaptee::~Adaptee()
{
}

QStringList BaseChannel::Adaptee::interfaces() const
{
    QStringList ret;
    foreach(const AbstractChannelInterfacePtr & iface, mChannel->interfaces()) {
        if (iface->interfaceName().contains(QLatin1String(".Type.")))
            continue; //Do not include "Type"
        ret << iface->interfaceName();
    }
    return ret;
}

void BaseChannel::Adaptee::close(const Tp::Service::ChannelAdaptor::CloseContextPtr &context)
{
    mChannel->close();
    context->setFinished();
}

/**
 * \class BaseChannel
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for channel implementations.
 *
 */

BaseChannel::BaseChannel(const QDBusConnection &dbusConnection,
                         BaseConnection* connection,
                         const QString &channelType, uint targetHandle,
                         uint targetHandleType)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, connection,
                        channelType, targetHandle, targetHandleType))
{
}

/**
 * Class destructor.
 */
BaseChannel::~BaseChannel()
{
    delete mPriv;
}

/**
 * Return a unique name for this channel.
 *
 * \return A unique name for this channel.
 */
QString BaseChannel::uniqueName() const
{
    return mPriv->uniqueName;
}

bool BaseChannel::registerObject(DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    QString name = uniqueName();
    QString busName = mPriv->connection->busName();
    //QString busName = QString(QLatin1String("%1.%2"))
    //        .arg(mPriv->connection->busName(),name);
    QString objectPath = QString(QLatin1String("%1/%2"))
                         .arg(mPriv->connection->objectPath(), name);
    debug() << "Registering channel: busName: " << busName << " objectName: " << objectPath;
    DBusError _error;

    debug() << "Channel: registering interfaces  at " << dbusObject();
    foreach(const AbstractChannelInterfacePtr & iface, mPriv->interfaces) {
        if (!iface->registerInterface(dbusObject())) {
            // lets not fail if an optional interface fails registering, lets warn only
            warning() << "Unable to register interface" << iface->interfaceName();
        }
    }

    bool ret = registerObject(busName, objectPath, &_error);
    if (!ret && error) {
        error->set(_error.name(), _error.message());
    }
    return ret;
}

/**
 * Reimplemented from DBusService.
 */
bool BaseChannel::registerObject(const QString &busName,
                                 const QString &objectPath, DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

QString BaseChannel::channelType() const
{
    return mPriv->channelType;
}
QList<AbstractChannelInterfacePtr> BaseChannel::interfaces() const
{
    return mPriv->interfaces.values();
}
uint BaseChannel::targetHandle() const
{
    return mPriv->targetHandle;
}
QString BaseChannel::targetID() const
{
    return mPriv->targetID;
}
uint BaseChannel::targetHandleType() const
{
    return mPriv->targetHandleType;
}
bool BaseChannel::requested() const
{
    return mPriv->requested;
}
uint BaseChannel::initiatorHandle() const
{
    return mPriv->initiatorHandle;
}
QString BaseChannel::initiatorID() const
{
    return mPriv->initiatorID;
}

void BaseChannel::setInitiatorHandle(uint initiatorHandle)
{
    mPriv->initiatorHandle = initiatorHandle;
}

void BaseChannel::setInitiatorID(const QString &initiatorID)
{
    mPriv->initiatorID = initiatorID;
}

void BaseChannel::setTargetID(const QString &targetID)
{
    mPriv->targetID = targetID;
}

void BaseChannel::setRequested(bool requested)
{
    mPriv->requested = requested;
}

void BaseChannel::close()
{
    // Method is used in destructor, so (to be sure that adaptee is exists) we should use DirectConnection
    QMetaObject::invokeMethod(mPriv->adaptee, "closed", Qt::DirectConnection);
    emit closed();
}

/**
 * Return the immutable properties of this channel object.
 *
 * Immutable properties cannot change after the object has been registered
 * on the bus with registerObject().
 *
 * \return The immutable properties of this channel object.
 */
QVariantMap BaseChannel::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
               QVariant::fromValue(mPriv->adaptee->channelType()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
               QVariant::fromValue(mPriv->adaptee->targetHandle()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".Interfaces"),
               QVariant::fromValue(mPriv->adaptee->interfaces()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
               QVariant::fromValue(mPriv->adaptee->targetID()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
               QVariant::fromValue(mPriv->adaptee->targetHandleType()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"),
               QVariant::fromValue(mPriv->adaptee->requested()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle"),
               QVariant::fromValue(mPriv->adaptee->initiatorHandle()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorID"),
               QVariant::fromValue(mPriv->adaptee->initiatorID()));
    return map;
}

Tp::ChannelDetails BaseChannel::details() const
{
    Tp::ChannelDetails details;
    details.channel = QDBusObjectPath(objectPath());
    details.properties.unite(immutableProperties());

    foreach(const AbstractChannelInterfacePtr & iface, mPriv->interfaces) {
        details.properties.unite(iface->immutableProperties());
    }

    return details;
}

/**
 * Return a pointer to the interface with the given name.
 *
 * \param interfaceName The D-Bus name of the interface,
 * ex. TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING.
 * \return A pointer to the AbstractProtocolInterface object that implements
 * the D-Bus interface with the given name, or a null pointer if such an interface
 * has not been plugged into this object.
 * \sa plugInterface(), interfaces()
 */
AbstractChannelInterfacePtr BaseChannel::interface(const QString &interfaceName) const
{
    return mPriv->interfaces.value(interfaceName);
}

/**
 * Plug a new interface into this Protocol D-Bus object.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param interface An AbstractProtocolInterface instance that implements
 * the interface that is to be plugged.
 * \return \c true on success or \c false otherwise
 * \sa interfaces(), interface()
 */
bool BaseChannel::plugInterface(const AbstractChannelInterfacePtr &interface)
{
    if (isRegistered()) {
        warning() << "Unable to plug protocol interface " << interface->interfaceName() <<
                  "- protocol already registered";
        return false;
    }

    if (interface->isRegistered()) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
                  "- interface already registered";
        return false;
    }

    if (mPriv->interfaces.contains(interface->interfaceName())) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
                  "- another interface with same name already plugged";
        return false;
    }

    debug() << "Interface" << interface->interfaceName() << "plugged";
    mPriv->interfaces.insert(interface->interfaceName(), interface);
    return true;
}

/**
 * \class AbstractChannelInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for all the Channel object interface implementations.
 */

AbstractChannelInterface::AbstractChannelInterface(const QString &interfaceName)
    : AbstractDBusServiceInterface(interfaceName)
{
}

AbstractChannelInterface::~AbstractChannelInterface()
{
}

// Chan.T.Text
BaseChannelTextType::Adaptee::Adaptee(BaseChannelTextType *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelTextType::Adaptee::~Adaptee()
{
}

void BaseChannelTextType::Adaptee::acknowledgePendingMessages(const Tp::UIntList &IDs,
        const Tp::Service::ChannelTypeTextAdaptor::AcknowledgePendingMessagesContextPtr &context)
{
    qDebug() << "BaseConnectionContactsInterface::acknowledgePendingMessages " << IDs;
    DBusError error;
    mInterface->acknowledgePendingMessages(IDs, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

struct TP_QT_NO_EXPORT BaseChannelTextType::Private {
    Private(BaseChannelTextType *parent, BaseChannel* channel)
        : channel(channel),
          pendingMessagesId(0),
          adaptee(new BaseChannelTextType::Adaptee(parent)) {
    }

    BaseChannel* channel;
    /* maps pending-message-id to message part list */
    QMap<uint, Tp::MessagePartList> pendingMessages;
    /* increasing unique id of pending messages */
    uint pendingMessagesId;
    MessageAcknowledgedCallback messageAcknowledgedCB;
    BaseChannelTextType::Adaptee *adaptee;
};

/**
 * \class BaseChannelTextType
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Type.Text
 *
 */

/**
 * Class constructor.
 */
BaseChannelTextType::BaseChannelTextType(BaseChannel* channel)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_TYPE_TEXT),
      mPriv(new Private(this, channel))
{
}

/**
 * Class destructor.
 */
BaseChannelTextType::~BaseChannelTextType()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelTextType::immutableProperties() const
{
    return QVariantMap();
}

void BaseChannelTextType::createAdaptor()
{
    (void) new Service::ChannelTypeTextAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelTextType::addReceivedMessage(const Tp::MessagePartList &msg)
{
    MessagePartList message = msg;
    if (msg.empty()) {
        warning() << "empty message: not sent";
        return;
    }
    MessagePart &header = message.front();

    if (header.count(QLatin1String("pending-message-id")))
        warning() << "pending-message-id will be overwritten";

    /* Add pending-message-id to header */
    uint pendingMessageId = mPriv->pendingMessagesId++;
    header[QLatin1String("pending-message-id")] = QDBusVariant(pendingMessageId);
    mPriv->pendingMessages[pendingMessageId] = message;

    uint timestamp = 0;
    if (header.count(QLatin1String("message-received")))
        timestamp = header[QLatin1String("message-received")].variant().toUInt();

    uint handle = 0;
    if (header.count(QLatin1String("message-sender")))
        handle = header[QLatin1String("message-sender")].variant().toUInt();

    uint type = ChannelTextMessageTypeNormal;
    if (header.count(QLatin1String("message-type")))
        type = header[QLatin1String("message-type")].variant().toUInt();

    //FIXME: flags are not parsed
    uint flags = 0;

    QString content;
    for (MessagePartList::Iterator i = message.begin() + 1; i != message.end(); ++i)
        if (i->count(QLatin1String("content-type"))
                && i->value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain")
                && i->count(QLatin1String("content"))) {
            content = i->value(QLatin1String("content")).variant().toString();
            break;
        }
    if (content.length() > 0)
        QMetaObject::invokeMethod(mPriv->adaptee, "received",
                                  Qt::QueuedConnection,
                                  Q_ARG(uint, pendingMessageId),
                                  Q_ARG(uint, timestamp),
                                  Q_ARG(uint, handle),
                                  Q_ARG(uint, type),
                                  Q_ARG(uint, flags),
                                  Q_ARG(QString, content));

    /* Signal on ChannelMessagesInterface */
    BaseChannelMessagesInterfacePtr messagesIface = BaseChannelMessagesInterfacePtr::dynamicCast(
                mPriv->channel->interface(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES));
    if (messagesIface)
        QMetaObject::invokeMethod(messagesIface.data(), "messageReceived",
                                  Qt::QueuedConnection,
                                  Q_ARG(Tp::MessagePartList, message));
}

Tp::MessagePartListList BaseChannelTextType::pendingMessages()
{
    return mPriv->pendingMessages.values();
}

/*
 * Will be called with the value of the message-token field after a received message has been acknowledged,
 * if the message-token field existed in the header.
 */
void BaseChannelTextType::setMessageAcknowledgedCallback(const MessageAcknowledgedCallback &cb)
{
    mPriv->messageAcknowledgedCB = cb;
}

void BaseChannelTextType::acknowledgePendingMessages(const Tp::UIntList &IDs, DBusError* error)
{
    foreach(uint id, IDs) {
        QMap<uint, Tp::MessagePartList>::Iterator i = mPriv->pendingMessages.find(id);
        if (i == mPriv->pendingMessages.end()) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("id not found"));
            return;
        }

        MessagePart &header = i->front();
        if (header.count(QLatin1String("message-token")) && mPriv->messageAcknowledgedCB.isValid())
            mPriv->messageAcknowledgedCB(header[QLatin1String("message-token")].variant().toString());

        mPriv->pendingMessages.erase(i);
    }

    /* Signal on ChannelMessagesInterface */
    BaseChannelMessagesInterfacePtr messagesIface = BaseChannelMessagesInterfacePtr::dynamicCast(
                mPriv->channel->interface(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES));
    if (messagesIface) //emit after return
        QMetaObject::invokeMethod(messagesIface.data(), "pendingMessagesRemoved",
                                  Qt::QueuedConnection,
                                  Q_ARG(Tp::UIntList, IDs));
}



void BaseChannelTextType::sent(uint timestamp, uint type, QString text)
{
    QMetaObject::invokeMethod(mPriv->adaptee,"sent",Q_ARG(uint, timestamp), Q_ARG(uint, type), Q_ARG(QString, text)); //Can simply use emit in Qt5
}


// Chan.I.Messages
BaseChannelMessagesInterface::Adaptee::Adaptee(BaseChannelMessagesInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelMessagesInterface::Adaptee::~Adaptee()
{
}

void BaseChannelMessagesInterface::Adaptee::sendMessage(const Tp::MessagePartList &message, uint flags,
        const Tp::Service::ChannelInterfaceMessagesAdaptor::SendMessageContextPtr &context)
{
    DBusError error;
    QString token = mInterface->sendMessage(message, flags, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(token);
}

struct TP_QT_NO_EXPORT BaseChannelMessagesInterface::Private {
    Private(BaseChannelMessagesInterface *parent,
            BaseChannelTextType* textTypeInterface,
            QStringList supportedContentTypes,
            Tp::UIntList messageTypes,
            uint messagePartSupportFlags,
            uint deliveryReportingSupport)
        : textTypeInterface(textTypeInterface),
          supportedContentTypes(supportedContentTypes),
          messageTypes(messageTypes),
          messagePartSupportFlags(messagePartSupportFlags),
          deliveryReportingSupport(deliveryReportingSupport),
          adaptee(new BaseChannelMessagesInterface::Adaptee(parent)) {
    }

    BaseChannelTextType* textTypeInterface;
    QStringList supportedContentTypes;
    Tp::UIntList messageTypes;
    uint messagePartSupportFlags;
    uint deliveryReportingSupport;
    SendMessageCallback sendMessageCB;
    BaseChannelMessagesInterface::Adaptee *adaptee;
};

/**
 * \class BaseChannelMessagesInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Messages
 *
 */

/**
 * Class constructor.
 */
BaseChannelMessagesInterface::BaseChannelMessagesInterface(BaseChannelTextType *textType,
        QStringList supportedContentTypes,
        UIntList messageTypes,
        uint messagePartSupportFlags,
        uint deliveryReportingSupport)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES),
      mPriv(new Private(this, textType, supportedContentTypes, messageTypes,
                        messagePartSupportFlags, deliveryReportingSupport))
{
}

/**
 * Class destructor.
 */
BaseChannelMessagesInterface::~BaseChannelMessagesInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelMessagesInterface::immutableProperties() const
{
    QVariantMap map;

    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".SupportedContentTypes"),
               QVariant::fromValue(mPriv->adaptee->supportedContentTypes()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".MessageTypes"),
               QVariant::fromValue(mPriv->adaptee->messageTypes()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".MessagePartSupportFlags"),
               QVariant::fromValue(mPriv->adaptee->messagePartSupportFlags()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".DeliveryReportingSupport"),
               QVariant::fromValue(mPriv->adaptee->deliveryReportingSupport()));
    return map;
}

void BaseChannelMessagesInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceMessagesAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

QStringList BaseChannelMessagesInterface::supportedContentTypes()
{
    return mPriv->supportedContentTypes;
}

Tp::UIntList BaseChannelMessagesInterface::messageTypes()
{
    return mPriv->messageTypes;
}

uint BaseChannelMessagesInterface::messagePartSupportFlags()
{
    return mPriv->messagePartSupportFlags;
}

uint BaseChannelMessagesInterface::deliveryReportingSupport()
{
    return mPriv->deliveryReportingSupport;
}

Tp::MessagePartListList BaseChannelMessagesInterface::pendingMessages()
{
    return mPriv->textTypeInterface->pendingMessages();
}

void BaseChannelMessagesInterface::messageSent(const Tp::MessagePartList &content, uint flags, const QString &messageToken)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "messageSent", Q_ARG(Tp::MessagePartList, content), Q_ARG(uint, flags), Q_ARG(QString, messageToken)); //Can simply use emit in Qt5
}

void BaseChannelMessagesInterface::pendingMessagesRemoved(const Tp::UIntList &messageIDs)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "pendingMessagesRemoved", Q_ARG(Tp::UIntList, messageIDs)); //Can simply use emit in Qt5
}

void BaseChannelMessagesInterface::messageReceived(const Tp::MessagePartList &message)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "messageReceived", Q_ARG(Tp::MessagePartList, message)); //Can simply use emit in Qt5
}

void BaseChannelMessagesInterface::setSendMessageCallback(const SendMessageCallback &cb)
{
    mPriv->sendMessageCB = cb;
}

QString BaseChannelMessagesInterface::sendMessage(const Tp::MessagePartList &message, uint flags, DBusError* error)
{
    if (!mPriv->sendMessageCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    const QString token = mPriv->sendMessageCB(message, flags, error);

    Tp::MessagePartList fixedMessage = message;

    MessagePart header = fixedMessage.front();

    uint timestamp = 0;
    if (header.count(QLatin1String("message-sent"))) {
        timestamp = header[QLatin1String("message-sent")].variant().toUInt();
    } else {
        timestamp = QDateTime::currentMSecsSinceEpoch() / 1000;
        header[QLatin1String("message-sent")] = QDBusVariant(timestamp);
    }

    fixedMessage.replace(0, header);

    //emit after return
    QMetaObject::invokeMethod(mPriv->adaptee, "messageSent",
                              Qt::QueuedConnection,
                              Q_ARG(Tp::MessagePartList, fixedMessage),
                              Q_ARG(uint, flags),
                              Q_ARG(QString, token));

    if (message.empty()) {
        warning() << "Sending empty message";
        return token;
    }

    uint type = ChannelTextMessageTypeNormal;
    if (header.count(QLatin1String("message-type")))
        type = header[QLatin1String("message-type")].variant().toUInt();

    QString content;
    for (MessagePartList::const_iterator i = message.begin() + 1; i != message.end(); ++i)
        if (i->count(QLatin1String("content-type"))
                && i->value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain")
                && i->count(QLatin1String("content"))) {
            content = i->value(QLatin1String("content")).variant().toString();
            break;
        }
    //emit after return
    QMetaObject::invokeMethod(mPriv->textTypeInterface, "sent",
                              Qt::QueuedConnection,
                              Q_ARG(uint, timestamp),
                              Q_ARG(uint, type),
                              Q_ARG(QString, content));
    return token;
}


//Chan.T.ServerAuthentication
BaseChannelServerAuthenticationType::Adaptee::Adaptee(BaseChannelServerAuthenticationType *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelServerAuthenticationType::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelServerAuthenticationType::Private {
    Private(BaseChannelServerAuthenticationType *parent, const QString& authenticationMethod)
        : authenticationMethod(authenticationMethod),
          adaptee(new BaseChannelServerAuthenticationType::Adaptee(parent)) {
    }
    QString authenticationMethod;
    BaseChannelServerAuthenticationType::Adaptee *adaptee;
};

QString BaseChannelServerAuthenticationType::Adaptee::authenticationMethod() const
{
    return mInterface->mPriv->authenticationMethod;
}

/**
 * \class BaseChannelServerAuthenticationType
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Type.ServerAuthentifcation
 *
 */

/**
 * Class constructor.
 */
BaseChannelServerAuthenticationType::BaseChannelServerAuthenticationType(const QString& authenticationMethod)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION),
      mPriv(new Private(this, authenticationMethod))
{
}

/**
 * Class destructor.
 */
BaseChannelServerAuthenticationType::~BaseChannelServerAuthenticationType()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelServerAuthenticationType::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION + QLatin1String(".AuthenticationMethod"),
               QVariant::fromValue(mPriv->adaptee->authenticationMethod()));
    return map;
}

void BaseChannelServerAuthenticationType::createAdaptor()
{
    (void) new Service::ChannelTypeServerAuthenticationAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

//Chan.I.CaptchaAuthentication
BaseChannelCaptchaAuthenticationInterface::Adaptee::Adaptee(BaseChannelCaptchaAuthenticationInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelCaptchaAuthenticationInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelCaptchaAuthenticationInterface::Private {
    Private(BaseChannelCaptchaAuthenticationInterface *parent, bool canRetryCaptcha)
        : canRetryCaptcha(canRetryCaptcha),
          captchaStatus(CaptchaStatusLocalPending),
          adaptee(new BaseChannelCaptchaAuthenticationInterface::Adaptee(parent)) {
    }
    bool canRetryCaptcha;
    bool captchaStatus;
    QString captchaError;
    QVariantMap captchaErrorDetails;
    GetCaptchasCallback getCaptchasCB;
    GetCaptchaDataCallback getCaptchaDataCB;
    AnswerCaptchasCallback answerCaptchasCB;
    CancelCaptchaCallback cancelCaptchaCB;
    BaseChannelCaptchaAuthenticationInterface::Adaptee *adaptee;
};

bool BaseChannelCaptchaAuthenticationInterface::Adaptee::canRetryCaptcha() const
{
    return mInterface->mPriv->canRetryCaptcha;
}

uint BaseChannelCaptchaAuthenticationInterface::Adaptee::captchaStatus() const
{
    return mInterface->mPriv->captchaStatus;
}

QString BaseChannelCaptchaAuthenticationInterface::Adaptee::captchaError() const
{
    return mInterface->mPriv->captchaError;
}

QVariantMap BaseChannelCaptchaAuthenticationInterface::Adaptee::captchaErrorDetails() const
{
    return mInterface->mPriv->captchaErrorDetails;
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchas(const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::GetCaptchasContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchas";
    DBusError error;
    Tp::CaptchaInfoList captchaInfo;
    uint numberRequired;
    QString language;
    mInterface->mPriv->getCaptchasCB(captchaInfo, numberRequired, language, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(captchaInfo, numberRequired, language);
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchaData(uint ID, const QString& mimeType, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::GetCaptchaDataContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchaData " << ID << mimeType;
    DBusError error;
    QByteArray captchaData = mInterface->mPriv->getCaptchaDataCB(ID, mimeType, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(captchaData);
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::answerCaptchas(const Tp::CaptchaAnswers& answers, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::AnswerCaptchasContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::answerCaptchas";
    DBusError error;
    mInterface->mPriv->answerCaptchasCB(answers, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::cancelCaptcha(uint reason, const QString& debugMessage, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::CancelCaptchaContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::cancelCaptcha "
             << reason << " " << debugMessage;
    DBusError error;
    mInterface->mPriv->cancelCaptchaCB(reason, debugMessage, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelCaptchaAuthenticationInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.CaptchaAuthentication
 *
 */

/**
 * Class constructor.
 */
BaseChannelCaptchaAuthenticationInterface::BaseChannelCaptchaAuthenticationInterface(bool canRetryCaptcha)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION),
      mPriv(new Private(this, canRetryCaptcha))
{
}

/**
 * Class destructor.
 */
BaseChannelCaptchaAuthenticationInterface::~BaseChannelCaptchaAuthenticationInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelCaptchaAuthenticationInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION + QLatin1String(".CanRetryCaptcha"),
               QVariant::fromValue(mPriv->adaptee->canRetryCaptcha()));
    return map;
}

void BaseChannelCaptchaAuthenticationInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceCaptchaAuthenticationAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelCaptchaAuthenticationInterface::setGetCaptchasCallback(const GetCaptchasCallback &cb)
{
    mPriv->getCaptchasCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setGetCaptchaDataCallback(const GetCaptchaDataCallback &cb)
{
    mPriv->getCaptchaDataCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setAnswerCaptchasCallback(const AnswerCaptchasCallback &cb)
{
    mPriv->answerCaptchasCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setCancelCaptchaCallback(const CancelCaptchaCallback &cb)
{
    mPriv->cancelCaptchaCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setCaptchaStatus(uint status)
{
    mPriv->captchaStatus = status;
}

void BaseChannelCaptchaAuthenticationInterface::setCaptchaError(const QString& busName)
{
    mPriv->captchaError = busName;
}

void BaseChannelCaptchaAuthenticationInterface::setCaptchaErrorDetails(const QVariantMap& error)
{
    mPriv->captchaErrorDetails = error;
}

//Chan.I.SASLAuthentication
struct TP_QT_NO_EXPORT BaseChannelSASLAuthenticationInterface::Private {
    Private(BaseChannelSASLAuthenticationInterface *parent,
            const QStringList &availableMechanisms,
            bool hasInitialData,
            bool canTryAgain,
            const QString &authorizationIdentity,
            const QString &defaultUsername,
            const QString &defaultRealm,
            bool maySaveResponse)
        : availableMechanisms(availableMechanisms),
          hasInitialData(hasInitialData),
          canTryAgain(canTryAgain),
          saslStatus(0),
          authorizationIdentity(authorizationIdentity),
          defaultUsername(defaultUsername),
          defaultRealm(defaultRealm),
          maySaveResponse(maySaveResponse),
          adaptee(new BaseChannelSASLAuthenticationInterface::Adaptee(parent))
    {
    }

    QStringList availableMechanisms;
    bool hasInitialData;
    bool canTryAgain;
    uint saslStatus;
    QString saslError;
    QVariantMap saslErrorDetails;
    QString authorizationIdentity;
    QString defaultUsername;
    QString defaultRealm;
    bool maySaveResponse;
    StartMechanismCallback startMechanismCB;
    StartMechanismWithDataCallback startMechanismWithDataCB;
    RespondCallback respondCB;
    AcceptSASLCallback acceptSaslCB;
    AbortSASLCallback abortSaslCB;
    BaseChannelSASLAuthenticationInterface::Adaptee *adaptee;
};

BaseChannelSASLAuthenticationInterface::Adaptee::Adaptee(BaseChannelSASLAuthenticationInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelSASLAuthenticationInterface::Adaptee::~Adaptee()
{
}

QStringList BaseChannelSASLAuthenticationInterface::Adaptee::availableMechanisms() const
{
    return mInterface->availableMechanisms();
}

bool BaseChannelSASLAuthenticationInterface::Adaptee::hasInitialData() const
{
    return mInterface->hasInitialData();
}

bool BaseChannelSASLAuthenticationInterface::Adaptee::canTryAgain() const
{
    return mInterface->canTryAgain();
}

uint BaseChannelSASLAuthenticationInterface::Adaptee::saslStatus() const
{
    return mInterface->saslStatus();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::saslError() const
{
    return mInterface->saslError();
}

QVariantMap BaseChannelSASLAuthenticationInterface::Adaptee::saslErrorDetails() const
{
    return mInterface->saslErrorDetails();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::authorizationIdentity() const
{
    return mInterface->authorizationIdentity();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::defaultUsername() const
{
    return mInterface->defaultUsername();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::defaultRealm() const
{
    return mInterface->defaultRealm();
}

bool BaseChannelSASLAuthenticationInterface::Adaptee::maySaveResponse() const
{
    return mInterface->maySaveResponse();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::startMechanism(const QString &mechanism,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::StartMechanismContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::startMechanism";
    DBusError error;
    mInterface->startMechanism(mechanism, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::startMechanismWithData(const QString &mechanism, const QByteArray &initialData,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::StartMechanismWithDataContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::startMechanismWithData";
    DBusError error;
    mInterface->startMechanismWithData(mechanism, initialData, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::respond(const QByteArray &responseData,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::RespondContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::respond";
    DBusError error;
    mInterface->respond(responseData, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::acceptSasl(
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::AcceptSASLContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::acceptSasl";
    DBusError error;
    mInterface->acceptSasl(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::abortSasl(uint reason, const QString &debugMessage,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::AbortSASLContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::abortSasl";
    DBusError error;
    mInterface->abortSasl(reason, debugMessage, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelSASLAuthenticationInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.SASLAuthentication
 *
 */

/**
 * Class constructor.
 */
BaseChannelSASLAuthenticationInterface::BaseChannelSASLAuthenticationInterface(const QStringList &availableMechanisms,
                                                                               bool hasInitialData,
                                                                               bool canTryAgain,
                                                                               const QString &authorizationIdentity,
                                                                               const QString &defaultUsername,
                                                                               const QString &defaultRealm,
                                                                               bool maySaveResponse)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION),
      mPriv(new Private(this, availableMechanisms, hasInitialData, canTryAgain, authorizationIdentity, defaultUsername, defaultRealm, maySaveResponse))
{
}

/**
 * Class destructor.
 */
BaseChannelSASLAuthenticationInterface::~BaseChannelSASLAuthenticationInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelSASLAuthenticationInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".AvailableMechanisms"),
               QVariant::fromValue(mPriv->adaptee->availableMechanisms()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".HasInitialData"),
               QVariant::fromValue(mPriv->adaptee->hasInitialData()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".CanTryAgain"),
               QVariant::fromValue(mPriv->adaptee->canTryAgain()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".AuthorizationIdentity"),
               QVariant::fromValue(mPriv->adaptee->authorizationIdentity()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".DefaultUsername"),
               QVariant::fromValue(mPriv->adaptee->defaultUsername()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".DefaultRealm"),
               QVariant::fromValue(mPriv->adaptee->defaultRealm()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".MaySaveResponse"),
               QVariant::fromValue(mPriv->adaptee->maySaveResponse()));
    return map;
}

QStringList BaseChannelSASLAuthenticationInterface::availableMechanisms() const
{
    return mPriv->availableMechanisms;
}

bool BaseChannelSASLAuthenticationInterface::hasInitialData() const
{
    return mPriv->hasInitialData;
}

bool BaseChannelSASLAuthenticationInterface::canTryAgain() const
{
    return mPriv->canTryAgain;
}

uint BaseChannelSASLAuthenticationInterface::saslStatus() const
{
    return mPriv->saslStatus;
}

void BaseChannelSASLAuthenticationInterface::setSaslStatus(uint status, const QString &reason, const QVariantMap &details)
{
    mPriv->saslStatus = status;
    QMetaObject::invokeMethod(mPriv->adaptee, "saslStatusChanged", Q_ARG(uint, status), Q_ARG(QString, reason), Q_ARG(QVariantMap, details)); //Can simply use emit in Qt5
}

QString BaseChannelSASLAuthenticationInterface::saslError() const
{
    return mPriv->saslError;
}

void BaseChannelSASLAuthenticationInterface::setSaslError(const QString &saslError)
{
    mPriv->saslError = saslError;
}

QVariantMap BaseChannelSASLAuthenticationInterface::saslErrorDetails() const
{
    return mPriv->saslErrorDetails;
}

void BaseChannelSASLAuthenticationInterface::setSaslErrorDetails(const QVariantMap &saslErrorDetails)
{
    mPriv->saslErrorDetails = saslErrorDetails;
}

QString BaseChannelSASLAuthenticationInterface::authorizationIdentity() const
{
    return mPriv->authorizationIdentity;
}

QString BaseChannelSASLAuthenticationInterface::defaultUsername() const
{
    return mPriv->defaultUsername;
}

QString BaseChannelSASLAuthenticationInterface::defaultRealm() const
{
    return mPriv->defaultRealm;
}

bool BaseChannelSASLAuthenticationInterface::maySaveResponse() const
{
    return mPriv->maySaveResponse;
}

void BaseChannelSASLAuthenticationInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceSASLAuthenticationAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelSASLAuthenticationInterface::setStartMechanismCallback(const BaseChannelSASLAuthenticationInterface::StartMechanismCallback &cb)
{
    mPriv->startMechanismCB = cb;
}

void BaseChannelSASLAuthenticationInterface::startMechanism(const QString &mechanism, DBusError *error)
{
    if (!mPriv->startMechanismCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->startMechanismCB(mechanism, error);
}

void BaseChannelSASLAuthenticationInterface::setStartMechanismWithDataCallback(const BaseChannelSASLAuthenticationInterface::StartMechanismWithDataCallback &cb)
{
    mPriv->startMechanismWithDataCB = cb;
}

void BaseChannelSASLAuthenticationInterface::startMechanismWithData(const QString &mechanism, const QByteArray &initialData, DBusError *error)
{
    if (!mPriv->startMechanismWithDataCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->startMechanismWithDataCB(mechanism, initialData, error);
}

void BaseChannelSASLAuthenticationInterface::setRespondCallback(const BaseChannelSASLAuthenticationInterface::RespondCallback &cb)
{
    mPriv->respondCB = cb;
}

void BaseChannelSASLAuthenticationInterface::respond(const QByteArray &responseData, DBusError *error)
{
    if (!mPriv->respondCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->respondCB(responseData, error);
}

void BaseChannelSASLAuthenticationInterface::setAcceptSaslCallback(const BaseChannelSASLAuthenticationInterface::AcceptSASLCallback &cb)
{
    mPriv->acceptSaslCB = cb;
}

void BaseChannelSASLAuthenticationInterface::acceptSasl(DBusError *error)
{
    if (!mPriv->acceptSaslCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->acceptSaslCB(error);
}

void BaseChannelSASLAuthenticationInterface::setAbortSaslCallback(const BaseChannelSASLAuthenticationInterface::AbortSASLCallback &cb)
{
    mPriv->abortSaslCB = cb;
}

void BaseChannelSASLAuthenticationInterface::abortSasl(uint reason, const QString &debugMessage, DBusError *error)
{
    if (!mPriv->abortSaslCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->abortSaslCB(reason, debugMessage, error);
}

void BaseChannelSASLAuthenticationInterface::newChallenge(const QByteArray &challengeData)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "newChallenge", Q_ARG(QByteArray, challengeData)); //Can simply use emit in Qt5
}

// Chan.I.Securable
struct TP_QT_NO_EXPORT BaseChannelSecurableInterface::Private {
    Private(BaseChannelSecurableInterface *parent)
        : encrypted(false),
          verified(false),
          adaptee(new BaseChannelSecurableInterface::Adaptee(parent))
    {
    }

    bool encrypted;
    bool verified;
    BaseChannelSecurableInterface::Adaptee *adaptee;
};

BaseChannelSecurableInterface::Adaptee::Adaptee(BaseChannelSecurableInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelSecurableInterface::Adaptee::~Adaptee()
{
}

bool BaseChannelSecurableInterface::Adaptee::encrypted() const
{
    return mInterface->encrypted();
}

bool BaseChannelSecurableInterface::Adaptee::verified() const
{
    return mInterface->verified();
}

/**
 * \class BaseChannelSecurableInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Securable
 */

/**
 * Class constructor.
 */
BaseChannelSecurableInterface::BaseChannelSecurableInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_SECURABLE),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseChannelSecurableInterface::~BaseChannelSecurableInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelSecurableInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

bool BaseChannelSecurableInterface::encrypted() const
{
    return mPriv->encrypted;
}

void BaseChannelSecurableInterface::setEncrypted(bool encrypted)
{
    mPriv->encrypted = encrypted;
}

bool BaseChannelSecurableInterface::verified() const
{
    return mPriv->verified;
}

void BaseChannelSecurableInterface::setVerified(bool verified)
{
    mPriv->verified = verified;
}

void BaseChannelSecurableInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceSecurableAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

// Chan.I.ChatState
struct TP_QT_NO_EXPORT BaseChannelChatStateInterface::Private {
    Private(BaseChannelChatStateInterface *parent)
        : adaptee(new BaseChannelChatStateInterface::Adaptee(parent))
    {
    }

    Tp::ChatStateMap chatStates;
    SetChatStateCallback setChatStateCB;
    BaseChannelChatStateInterface::Adaptee *adaptee;
};

BaseChannelChatStateInterface::Adaptee::Adaptee(BaseChannelChatStateInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelChatStateInterface::Adaptee::~Adaptee()
{
}

Tp::ChatStateMap BaseChannelChatStateInterface::Adaptee::chatStates() const
{
    return mInterface->chatStates();
}

void BaseChannelChatStateInterface::Adaptee::setChatState(uint state,
        const Tp::Service::ChannelInterfaceChatStateAdaptor::SetChatStateContextPtr &context)
{
    qDebug() << "BaseChannelChatStateInterface::Adaptee::setChatState";
    DBusError error;
    mInterface->setChatState(state, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelChatStateInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Chat.State
 */

/**
 * Class constructor.
 */
BaseChannelChatStateInterface::BaseChannelChatStateInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_CHAT_STATE),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseChannelChatStateInterface::~BaseChannelChatStateInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelChatStateInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

Tp::ChatStateMap BaseChannelChatStateInterface::chatStates() const
{
    return mPriv->chatStates;
}

void BaseChannelChatStateInterface::setChatStates(const Tp::ChatStateMap &chatStates)
{
    mPriv->chatStates = chatStates;
}

void BaseChannelChatStateInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceChatStateAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelChatStateInterface::setSetChatStateCallback(const BaseChannelChatStateInterface::SetChatStateCallback &cb)
{
    mPriv->setChatStateCB = cb;
}

void BaseChannelChatStateInterface::setChatState(uint state, DBusError *error)
{
    if (!mPriv->setChatStateCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->setChatStateCB(state, error);
}

void BaseChannelChatStateInterface::chatStateChanged(uint contact, uint state)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "chatStateChanged", Q_ARG(uint, contact), Q_ARG(uint, state)); //Can simply use emit in Qt5
}

//Chan.I.Group
BaseChannelGroupInterface::Adaptee::Adaptee(BaseChannelGroupInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelGroupInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelGroupInterface::Private {
    Private(BaseChannelGroupInterface *parent, ChannelGroupFlags initialFlags, uint selfHandle)
        : flags(initialFlags),
          selfHandle(selfHandle),
          adaptee(new BaseChannelGroupInterface::Adaptee(parent)) {
    }
    ChannelGroupFlags flags;
    Tp::HandleOwnerMap handleOwners;
    Tp::LocalPendingInfoList localPendingMembers;
    Tp::UIntList members;
    Tp::UIntList remotePendingMembers;
    uint selfHandle;
    Tp::HandleIdentifierMap memberIdentifiers;
    RemoveMembersCallback removeMembersCB;
    AddMembersCallback addMembersCB;
    Tp::UIntList getLocalPendingMembers() const {
        Tp::UIntList ret;
        foreach(const LocalPendingInfo & info, localPendingMembers)
        ret << info.toBeAdded;
        return ret;
    }
    BaseChannelGroupInterface::Adaptee *adaptee;
};

uint BaseChannelGroupInterface::Adaptee::groupFlags() const
{
    return mInterface->mPriv->flags;
}

Tp::HandleOwnerMap BaseChannelGroupInterface::Adaptee::handleOwners() const
{
    return mInterface->mPriv->handleOwners;
}

Tp::LocalPendingInfoList BaseChannelGroupInterface::Adaptee::localPendingMembers() const
{
    return mInterface->mPriv->localPendingMembers;
}

Tp::UIntList BaseChannelGroupInterface::Adaptee::members() const
{
    return mInterface->mPriv->members;
}

Tp::UIntList BaseChannelGroupInterface::Adaptee::remotePendingMembers() const
{
    return mInterface->mPriv->remotePendingMembers;
}

uint BaseChannelGroupInterface::Adaptee::selfHandle() const
{
    return mInterface->mPriv->selfHandle;
}

Tp::HandleIdentifierMap BaseChannelGroupInterface::Adaptee::memberIdentifiers() const
{
    return mInterface->mPriv->memberIdentifiers;
}

void BaseChannelGroupInterface::Adaptee::addMembers(const Tp::UIntList& contacts,
        const QString& message,
        const Tp::Service::ChannelInterfaceGroupAdaptor::AddMembersContextPtr &context)
{
    debug() << "BaseChannelGroupInterface::Adaptee::addMembers";
    if (!mInterface->mPriv->addMembersCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->addMembersCB(contacts, message, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelGroupInterface::Adaptee::removeMembers(const Tp::UIntList& contacts, const QString& message,
        const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersContextPtr &context)
{
    debug() << "BaseChannelGroupInterface::Adaptee::removeMembers";
    if (!mInterface->mPriv->removeMembersCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->removeMembersCB(contacts, message, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelGroupInterface::Adaptee::removeMembersWithReason(const Tp::UIntList& contacts,
        const QString& message,
        uint reason,
        const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersWithReasonContextPtr &context)
{
    debug() << "BaseChannelGroupInterface::Adaptee::removeMembersWithReason";
    removeMembers(contacts, message, context);
}

void BaseChannelGroupInterface::Adaptee::getAllMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetAllMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->members, mInterface->mPriv->getLocalPendingMembers(), mInterface->mPriv->remotePendingMembers);
}

void BaseChannelGroupInterface::Adaptee::getGroupFlags(const Tp::Service::ChannelInterfaceGroupAdaptor::GetGroupFlagsContextPtr &context)
{
    context->setFinished(mInterface->mPriv->flags);
}

void BaseChannelGroupInterface::Adaptee::getHandleOwners(const Tp::UIntList& handles, const Tp::Service::ChannelInterfaceGroupAdaptor::GetHandleOwnersContextPtr &context)
{
    Tp::UIntList ret;
    foreach(uint handle, handles)
    ret.append(mInterface->mPriv->handleOwners.contains(handle) ? mInterface->mPriv->handleOwners[handle] : 0);
    context->setFinished(ret);
}

void BaseChannelGroupInterface::Adaptee::getLocalPendingMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetLocalPendingMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->getLocalPendingMembers());
}

void BaseChannelGroupInterface::Adaptee::getLocalPendingMembersWithInfo(const Tp::Service::ChannelInterfaceGroupAdaptor::GetLocalPendingMembersWithInfoContextPtr &context)
{
    context->setFinished(mInterface->mPriv->localPendingMembers);
}

void BaseChannelGroupInterface::Adaptee::getMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->members);
}

void BaseChannelGroupInterface::Adaptee::getRemotePendingMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetRemotePendingMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->remotePendingMembers);
}

void BaseChannelGroupInterface::Adaptee::getSelfHandle(const Tp::Service::ChannelInterfaceGroupAdaptor::GetSelfHandleContextPtr &context)
{
    context->setFinished(mInterface->mPriv->selfHandle);
}

/**
 * \class BaseChannelGroupInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Group
 *
 */

/**
 * Class constructor.
 */
BaseChannelGroupInterface::BaseChannelGroupInterface(ChannelGroupFlags initialFlags, uint selfHandle)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP),
      mPriv(new Private(this, initialFlags, selfHandle))
{
}

/**
 * Class destructor.
 */
BaseChannelGroupInterface::~BaseChannelGroupInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelGroupInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseChannelGroupInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceGroupAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelGroupInterface::setRemoveMembersCallback(const RemoveMembersCallback &cb)
{
    mPriv->removeMembersCB = cb;
}

void BaseChannelGroupInterface::setAddMembersCallback(const AddMembersCallback &cb)
{
    mPriv->addMembersCB = cb;
}

void BaseChannelGroupInterface::addMembers(const Tp::UIntList& handles, const QStringList& identifiers)
{
    if (handles.size() != identifiers.size()) {
        debug() << "BaseChannelGroupInterface::addMembers: handles.size() != identifiers.size()";
        return;
    }
    Tp::UIntList added;
    for (int i = 0; i < handles.size(); ++i) {
        uint handle = handles[i];
        if (mPriv->members.contains(handle))
            continue;

        mPriv->memberIdentifiers[handle] = identifiers[i];
        mPriv->members.append(handle);
        added.append(handle);
    }
    if (!added.isEmpty())
        QMetaObject::invokeMethod(mPriv->adaptee,"membersChanged",Q_ARG(QString, QString()), Q_ARG(Tp::UIntList, added), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(uint, 0), Q_ARG(uint, ChannelGroupChangeReasonNone)); //Can simply use emit in Qt5
}

void BaseChannelGroupInterface::removeMembers(const Tp::UIntList& handles)
{
    Tp::UIntList removed;
    foreach(uint handle, handles) {
        if (mPriv->members.contains(handle))
            continue;

        mPriv->memberIdentifiers.remove(handle);
        mPriv->members.removeAll(handle);
        removed.append(handle);
    }
    if (!removed.isEmpty())
        QMetaObject::invokeMethod(mPriv->adaptee,"membersChanged",Q_ARG(QString, QString()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, removed), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(uint, 0), Q_ARG(uint,ChannelGroupChangeReasonNone)); //Can simply use emit in Qt5 //Can simply use emit in Qt5
}

}
