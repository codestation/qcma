/*
 *  QCMA: Cross-platform content manager assistant for the PS Vita
 *
 *  Copyright (C) 2013  Codestation
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CAPABILITY_H
#define CAPABILITY_H

#include <vitamtp.h>

class DeviceCapability
{
public:
    explicit DeviceCapability();
    ~DeviceCapability();
    bool exchangeInfo(vita_device_t *device);

    const char *getVersion() {
        return vita_info.responderVersion;
    }
    int getProtocolVersion()  {
        return vita_info.protocolVersion;
    }
    const char *getOnlineId()  {
        return vita_info.onlineId;
    }
    const char *getModelInfo()  {
        return vita_info.modelInfo;
    }

private:
    capability_info_t *generate_pc_capability_info();
    void free_pc_capability_info(capability_info_t *info);

    vita_info_t vita_info;
};

#endif // CAPABILITY_H
