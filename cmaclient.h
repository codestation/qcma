#ifndef CMACLIENT_H
#define CMACLIENT_H

#include "baseworker.h"
#include "cmaobject.h"
#include "database.h"

#include <QObject>
#include <QString>

extern "C" {
#include <vitamtp.h>
}

class CmaClient : public BaseWorker
{
    Q_OBJECT
public:
    explicit CmaClient(QObject *parent = 0);
    explicit CmaClient(Database *database, vita_device_t *device, QObject *parent = 0);
    ~CmaClient();

private:
    void enterEventLoop();

    uint16_t processAllObjects(CMAObject *parent, uint32_t handle);
    void vitaEventSendObject(vita_event_t *event, int eventId);
    void vitaEventSendObjectMetadata(vita_event_t *event, int eventId);
    void vitaEventSendNumOfObject(vita_event_t *event, int eventId);
    void vitaEventCancelTask(vita_event_t *event, int eventId);
    void vitaEventSendHttpObjectFromURL(vita_event_t *event, int eventId);
    void vitaEventUnimplementated(vita_event_t *event, int eventId);
    void vitaEventSendObjectStatus(vita_event_t *event, int eventId);
    void vitaEventSendObjectThumb(vita_event_t *event, int eventId);
    void vitaEventDeleteObject(vita_event_t *event, int eventId);
    void vitaEventGetSettingInfo(vita_event_t *event, int eventId);
    void vitaEventSendHttpObjectPropFromURL(vita_event_t *event, int eventId);
    void vitaEventSendPartOfObject(vita_event_t *event, int eventId);
    void vitaEventOperateObject(vita_event_t *event, int eventId);
    void vitaEventGetPartOfObject(vita_event_t *event, int eventId);
    void vitaEventSendStorageSize(vita_event_t *event, int eventId);
    void vitaEventCheckExistance(vita_event_t *event, int eventId);
    void vitaEventGetTreatObject(vita_event_t *event, int eventId);
    void vitaEventSendCopyConfirmationInfo(vita_event_t *event, int eventId);
    void vitaEventSendObjectMetadataItems(vita_event_t *event, int eventId);
    void vitaEventSendNPAccountInfo(vita_event_t *event, int eventId);
    void vitaEventRequestTerminate(vita_event_t *event, int eventId);

    Database *db;
    vita_device_t *device;
    volatile bool connected;
    static const metadata_t g_thumbmeta;

signals:
    void deviceConnected(QString);
    void refreshDatabase();
    void terminate();

public slots:
    void close();

private slots:
    void process();
};

#endif // CMACLIENT_H
