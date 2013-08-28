/*
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2010-2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
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

#include "tube-initiator.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/Debug>
#include <TelepathyQt/OutgoingStreamTubeChannel>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Presence>
#include <TelepathyQt/StreamTubeChannel>
#include <TelepathyQt/StreamTubeServer>

#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>

TubeInitiator::TubeInitiator(const QString &accountName, const QString &receiver, QObject *parent)
    : QObject(parent),
      mReceiver(receiver),
      mTubeRequested(false)
{
    mServer = new QTcpServer(this);
    connect(mServer, SIGNAL(newConnection()), this, SLOT(onTcpServerNewConnection()));
    mServer->listen();

    AccountFactoryPtr accountFactory = AccountFactory::create(
            QDBusConnection::sessionBus(), Account::FeatureCore);
    // We only care about CONNECTED connections, so let's specify that in a Connection Factory
    ConnectionFactoryPtr connectionFactory = ConnectionFactory::create(
            QDBusConnection::sessionBus(), Connection::FeatureCore | Connection::FeatureConnected);
    ChannelFactoryPtr channelFactory = ChannelFactory::create(QDBusConnection::sessionBus());
    ContactFactoryPtr contactFactory = ContactFactory::create(Contact::FeatureAlias);

    mTubeServer = StreamTubeServer::create(
            QStringList() << QLatin1String("tp-qt-stube-example"), /* one peer-to-peer service */
            QStringList(), /* no Room tube services */
            QString(), /* autogenerated Client name */
            true, /* do monitor connections */
            accountFactory, connectionFactory, channelFactory, contactFactory);

    connect(mTubeServer.data(),
            SIGNAL(newTcpConnection(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr,Tp::OutgoingStreamTubeChannelPtr)),
            SLOT(onTubeNewConnection(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr)));
    connect(mTubeServer.data(),
            SIGNAL(tcpConnectionClosed(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr,QString,QString,Tp::OutgoingStreamTubeChannelPtr)),
            SLOT(onTubeConnectionClosed(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr,QString,QString)));

    mTubeServer->exportTcpSocket(mServer);
    Q_ASSERT(mTubeServer->isRegistered());

    connect(accountFactory->proxy(
                TP_QT_ACCOUNT_MANAGER_BUS_NAME,
                TP_QT_ACCOUNT_OBJECT_PATH_BASE + QLatin1Char('/') + accountName,
                connectionFactory,
                mTubeServer->registrar()->channelFactory(),
                mTubeServer->registrar()->contactFactory()),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onAccountReady(Tp::PendingOperation *)));
}

TubeInitiator::~TubeInitiator()
{
}

void TubeInitiator::onAccountReady(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Account cannot become ready - " <<
            op->errorName() << '-' << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    PendingReady *ready = qobject_cast<PendingReady *>(op);
    Q_ASSERT(ready != NULL);

    mAccount = AccountPtr::qObjectCast(ready->proxy());

    qDebug() << "Account ready";
    connect(mAccount.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));

    if (mAccount->connection().isNull()) {
        qDebug() << "The account given has no Connection. Please set it online to continue.";
    } else {
        onAccountConnectionChanged(mAccount->connection());
    }
}

void TubeInitiator::onAccountConnectionChanged(const ConnectionPtr &conn)
{
    if (!conn) {
        return;
    }

    // Connection::FeatureConnected being in the Connection Factory means that we shouldn't get
    // pre-Connected Connections
    Q_ASSERT(conn->isValid());
    Q_ASSERT(conn->status() == ConnectionStatusConnected);

    qDebug() << "Got a Connected Connection!";
    mConn = conn;

    qDebug() << "Creating contact object for receiver" << mReceiver;
    connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << mReceiver,
                Features(Contact::FeatureCapabilities)),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onContactRetrieved(Tp::PendingOperation *)));
}

void TubeInitiator::onContactRetrieved(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create contact object for receiver" <<
            mReceiver << "-" << op->errorName() << ": " << op->errorMessage();
        return;
    }

    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    Q_ASSERT(pc->contacts().size() == 1);
    mContact = pc->contacts().first();

    qDebug() << "Checking contact capabilities...";
    connect(mContact.data(),
            SIGNAL(capabilitiesChanged(Tp::ContactCapabilities)),
            SLOT(onContactCapabilitiesChanged()));

    if (mContact->capabilities().streamTubes(QLatin1String("tp-qt-stube-example"))) {
        onContactCapabilitiesChanged();
    } else {
        qDebug() << "The remote contact needs to be online and have the receiver application running to continue";
    }
}

void TubeInitiator::onContactCapabilitiesChanged()
{
    if (mTubeRequested) {
        return;
    }

    if (mContact->capabilities().streamTubes(QLatin1String("tp-qt-stube-example"))) {
        qDebug() << "The remote contact is capable of receiving tubes with service tp-qt-stube-example now";

        mTubeRequested = true;
        connect(mAccount->createStreamTube(
                    mContact->id(),
                    QLatin1String("tp-qt-stube-example")),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onTubeRequestFinished(Tp::PendingOperation*)));
    }
}

void TubeInitiator::onTubeRequestFinished(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to request stream tube channel -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Stream tube channel request finished successfully!";
}

void TubeInitiator::onTubeNewConnection(
        const QHostAddress &srcAddr,
        quint16 srcPort,
        const Tp::AccountPtr &account,
        const Tp::ContactPtr &contact)
{
    qDebug() << "Source port" << srcPort << "is" << contact->alias();
    mConnAliases.insert(qMakePair(srcAddr, srcPort), contact->alias());
}

void TubeInitiator::onTubeConnectionClosed(
        const QHostAddress &srcAddr,
        quint16 srcPort,
        const Tp::AccountPtr &account,
        const Tp::ContactPtr &contact,
        const QString &error,
        const QString &message)
{
    qDebug() << "Connection from source port" << srcPort << "closed with" << error << ':' << message;
    mConnAliases.remove(qMakePair(srcAddr, srcPort));
}

void TubeInitiator::onTcpServerNewConnection()
{
    qDebug() << "Pending connection found";
    QTcpSocket *socket = mServer->nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(onDataFromSocket()));
}

void TubeInitiator::onDataFromSocket()
{
    QAbstractSocket *source = qobject_cast<QAbstractSocket *>(sender());
    QString data = QLatin1String(source->readLine());
    data.remove(QLatin1Char('\n'));

    // Look up the alias of the remote sending us data based on the tube connection tracking. Of
    // course, this is slightly overengineered; given that this example uses one-to-one tubes (not
    // group/MUC tubes), all remote connections are always from the same contact we opened the tube
    // to in the first place.
    QPair<QHostAddress, quint16> peerAddr(source->peerAddress(), source->peerPort());
    if (mConnAliases.contains(peerAddr)) {
        qDebug() << "New data from contact" << mConnAliases.value(peerAddr) << ':' << data;
    } else {
        // We haven't found out the identity for this connection yet. This may happen, because the
        // TCP server listen socket and the D-Bus connection are on separate sockets and have no
        // mutual ordering guarantees.
        //
        // A GUI application would likely use a placeholder item and lazily replace it with a
        // representation of the contact when newTcpConnection for this source address has been
        // received. A non-interactive daemon or alike might, in turn, queue incoming data until the
        // contact is known, or just carry on if the contact's details aren't needed (yet).
        qDebug() << "New data from source port" << peerAddr.second << ':' << data;
    }

    if (data == QLatin1String("Hi there!!")) {
        source->write(QByteArray("Hey back mate.\n"));
    } else {
        source->write(QByteArray("Sorry, I have no time for you right now.\n"));
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 3) {
        qDebug() << "usage:" << argv[0] << "<account name, as in mc-tool list> <receiver contact ID>";
        return 1;
    }

    Tp::registerTypes();

    new TubeInitiator(QLatin1String(argv[1]), QLatin1String(argv[2]), &app);

    return app.exec();
}

#include "_gen/tube-initiator.moc.hpp"
