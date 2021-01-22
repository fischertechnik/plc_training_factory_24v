/*
 * TxtNfcDevice.h
 *
 *  Created on: 09.02.2019
 *      Author: steiger-a
 */

#ifndef TXTNFCDEVICE_H_
#define TXTNFCDEVICE_H_

#include <err.h>
#include <stdlib.h>
#include <nfc/nfc.h>
#include <freefare.h>

#include <map>
#include <string>
#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "Observer.h"
#include "TxtMqttFactoryClient.h"
#include "TxtFactoryTypes.h"

#include "nfc-utils.h"


#define MAX_DEVICE_COUNT 16
#define MAX_TARGET_COUNT 16


namespace ft {


typedef enum
{
	NFC_NONE=-1,
	NFC_ERROR=0,
	NFC_READ_UID=1,
	NFC_READ=2,
	NFC_DELETE=3,
	NFC_WRITE=4,
	NFC_WRITE_ERROR=5
} TxtNFCLastCmd_t;

inline const char * toString(TxtNFCLastCmd_t v)
{
	switch(v) {
	case NFC_NONE: return "NONE";
	case NFC_READ_UID: return "READ UID";
	case NFC_READ: return "READ";
	case NFC_DELETE: return "DELETE";
	case NFC_WRITE: return "WRITE";
	default: return "";
	}
}

typedef union ts_u
{
    uint8_t u8[8];
    int64_t s64;
} uTS;


class TxtNfcData {
public:
	TxtNfcData() : wp(), map_hist(), mask_ts(0) {}
	virtual ~TxtNfcData() {}

	TxtWorkpiece wp;
	HistoryCode_map_t map_hist;
	uint8_t mask_ts;
};


class TxtNfcDevice : public SubjectObserver {
public:
	TxtNfcDevice();
	virtual ~TxtNfcDevice();

	bool open();
	void close();

	std::string readTagsGetUID();

	bool eraseTags();
	std::string readTags();
	bool writeTags(TxtWorkpiece wp, HistoryCode_map_t map_uts, uint8_t mask_ts);

    TxtNfcData* getNfcData() { return nfcData; }
    void printNfcData();

	TxtNFCLastCmd_t lastCmd;

protected:
	void printRawData(uint8_t* buffer);

	bool opened;
	nfc_device *pnd;
	nfc_context *context;
	TxtNfcData* nfcData;
};


class TxtNfcDeviceObserver : public ft::Observer {
public:
	TxtNfcDeviceObserver(ft::TxtNfcDevice* s, ft::TxtMqttFactoryClient* mqttclient)
		: _subject(s), _mqttclient(mqttclient)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtNfcDeviceObserver",0);
		_subject->Attach(this);
	}
	virtual ~TxtNfcDeviceObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtNfcDeviceObserver",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 1",0);
			assert(_mqttclient);
			if (_subject->getNfcData() != NULL)
			{
				SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update nfcData",0);
				TxtNfcData nfcData = *(_subject->getNfcData()); //copy data
				HistoryCode_map_t map_hist = nfcData.map_hist;
				_mqttclient->publishNfcDS(&(nfcData.wp), map_hist, TIMEOUT_MS_PUBLISH);
			} else {
				SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update nfcData NULL",0);
				_mqttclient->publishNfcDS(0, HistoryCode_map_t(), TIMEOUT_MS_PUBLISH);
			}
			SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 2",0);
		}
	}
private:
	ft::TxtNfcDevice *_subject;
	ft::TxtMqttFactoryClient* _mqttclient;
};


} /* namespace ft */


#endif /* TXTNFCDEVICE_H_ */
