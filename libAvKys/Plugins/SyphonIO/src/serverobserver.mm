/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include "serverobserver.h"

@implementation ServerObserver

- (id) initWithIOElement: (SyphonIOElement *) element
{
    self = [super init];

    if (!self)
        return nil;

    m_ioElement = element;

    return self;
}

- (void) serverAdded: (NSNotification *) notification
{
    Q_UNUSED(notification)

    m_ioElement->updateServers();
}

- (void) serverChanged: (NSNotification *) notification
{
    Q_UNUSED(notification)

    m_ioElement->updateServers();
}

- (void) serverRemoved: (NSNotification *) notification
{
    Q_UNUSED(notification)

    m_ioElement->updateServers();
}

@end
