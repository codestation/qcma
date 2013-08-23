#ifndef CMAEVENT_H
#define CMAEVENT_H

#include "cmaobject.h"
#include "database.h"
#include "baseworker.h"

#include <QObject>

#include <vitamtp.h>

class CmaEvent : public BaseWorker
{
    Q_OBJECT
public:
    explicit CmaEvent(vita_device_t *s_device, vita_event_t s_event, QObject *parent = 0);

    static Database db;
    
private:
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

    vita_device_t *device;
    vita_event_t t_event;

    static metadata_t g_thumbmeta;

signals:
    void finished();
    void finishedEventLoop();
    void refreshDatabase();

public slots:
    void process();
    
};

#endif // CMAEVENT_H
