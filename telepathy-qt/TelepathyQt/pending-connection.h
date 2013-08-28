/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt_pending_connection_h_HEADER_GUARD_
#define _TelepathyQt_pending_connection_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Connection>
#include <TelepathyQt/PendingOperation>

#include <QString>
#include <QVariantMap>

class QDBusPendingCallWatcher;

namespace Tp
{

class ConnectionManager;

class TP_QT_EXPORT PendingConnection : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingConnection);

public:
    ~PendingConnection();

    ConnectionManagerPtr manager() const;

    ConnectionPtr connection() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onCallFinished(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onConnectionBuilt(Tp::PendingOperation *op);

private:
    friend class ConnectionManagerLowlevel;

    TP_QT_NO_EXPORT PendingConnection(const ConnectionManagerPtr &manager,
            const QString &protocol, const QVariantMap &parameters);
    TP_QT_NO_EXPORT PendingConnection(const QString &error, const QString &errorMessage);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
