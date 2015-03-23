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

#include "capability.h"
#include "cmautils.h"

#include <QDebug>
#include <QHostInfo>
#include <QSettings>

DeviceCapability::DeviceCapability() :
    vita_info()
{
}

bool DeviceCapability::exchangeInfo(vita_device_t *device)
{
    if(VitaMTP_GetVitaInfo(device, &vita_info) != PTP_RC_OK) {
        qWarning("Cannot retreve device information.");
        return false;
    }

    if(vita_info.protocolVersion > VITAMTP_PROTOCOL_MAX_VERSION) {
        qWarning("Vita wants protocol version %08d while we only support %08d. Attempting to continue.",
                 vita_info.protocolVersion, VITAMTP_PROTOCOL_MAX_VERSION);
    }

    QSettings settings;
    QString hostname = settings.value("hostName", QHostInfo::localHostName()).toString();

    int protocol_version = ::getVitaProtocolVersion();

    qDebug() << "Sending Qcma protocol version:" << protocol_version;
    qDebug() << "Identifying as" << hostname;

    const initiator_info_t *pc_info = VitaMTP_Data_Initiator_New(hostname.toUtf8().data(), protocol_version);

    // Next, we send the client's (this program) info (discard the const here)
    if(VitaMTP_SendInitiatorInfo(device, (initiator_info_t *)pc_info) != PTP_RC_OK) {
        qWarning("Cannot send host information.");
        return false;
    }

    if(vita_info.protocolVersion >= VITAMTP_PROTOCOL_FW_2_10) {
        // Get the device's capabilities
        capability_info_t *vita_capabilities;

        if(VitaMTP_GetVitaCapabilityInfo(device, &vita_capabilities) != PTP_RC_OK) {
            qWarning("Failed to get capability information from Vita.");
            return false;
        }

        // TODO: vitamtp needs to send the full metadata info to know the expected format
        // of thumbnails, for example. Until then lets discard the received info.

        VitaMTP_Data_Free_Capability(vita_capabilities);
        // Send the host's capabilities
        capability_info_t *pc_capabilities = generate_pc_capability_info();

        if(VitaMTP_SendPCCapabilityInfo(device, pc_capabilities) != PTP_RC_OK) {
            qWarning("Failed to send capability information to Vita.");
            free_pc_capability_info(pc_capabilities);
            return false;
        }

        free_pc_capability_info(pc_capabilities);
    }

    // Finally, we tell the Vita we are connected
    if(VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_Connected) != PTP_RC_OK) {
        qWarning("Cannot send host status.");
        return false;
    }

    VitaMTP_Data_Free_Initiator(pc_info);
    return true;
}

void DeviceCapability::free_pc_capability_info(capability_info_t *info)
{
    delete[] &info->functions.formats.next_item[-1];
    delete[] &info->functions.next_item[-1];
    delete info;
}

capability_info_t *DeviceCapability::generate_pc_capability_info()
{
    typedef capability_info::capability_info_function tfunction;
    typedef tfunction::capability_info_format tformat;

    // TODO: Actually do this based on QCMA capabilities
    capability_info_t *pc_capabilities = new capability_info_t;
    pc_capabilities->version = "1.0";
    tfunction *functions = new tfunction[3]();
    tformat *game_formats = new tformat[5]();
    game_formats[0].contentType = "vitaApp";
    game_formats[0].next_item = &game_formats[1];
    game_formats[1].contentType = "PSPGame";
    game_formats[1].next_item = &game_formats[2];
    game_formats[2].contentType = "PSPSaveData";
    game_formats[2].next_item = &game_formats[3];
    game_formats[3].contentType = "PSGame";
    game_formats[3].next_item = &game_formats[4];
    game_formats[4].contentType = "PSMApp";
    functions[0].type = "game";
    functions[0].formats = game_formats[0];
    functions[0].next_item = &functions[1];
    functions[1].type = "backup";
    functions[1].next_item = &functions[2];
    functions[2].type = "systemUpdate";
    pc_capabilities->functions = functions[0];
    return pc_capabilities;
}

DeviceCapability::~DeviceCapability()
{
    VitaMTP_Data_Free_VitaInfo(&vita_info);
}
