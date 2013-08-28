/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#ifndef _TelepathyQt_examples_contact_messenger_sender_h_HEADER_GUARD_
#define _TelepathyQt_examples_contact_messenger_sender_h_HEADER_GUARD_

#include <TelepathyQt/Types>

#include <QObject>
#include <QString>

namespace Tp
{
class PendingOperation;
}

class Sender : public QObject
{
    Q_OBJECT

public:
    Sender(const QString &accountPath, const QString &contactIdentifier, const QString &message);
    ~Sender();

private Q_SLOTS:
    void onSendMessageFinished(Tp::PendingOperation *op);

private:
    Tp::ContactMessengerPtr messenger;
};

#endif
