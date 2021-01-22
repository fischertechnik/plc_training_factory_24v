/*
 * TxtDeliveryPickupStation.h
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#ifndef TxtDeliveryPickupStation_H_
#define TxtDeliveryPickupStation_H_


#include "TxtSimulationModel.h"
#include "TxtCalibData.h"
#include "TxtNfcDevice.h"
#include "TxtAxis.h"


namespace ft {


class TxtMqttFactoryClient;


extern bool reqUpdateDIN;
extern bool reqUpdateDOUT;


class TxtDeliveryPickupStationCalibData : public ft::TxtCalibData {
public:
	TxtDeliveryPickupStationCalibData()
		: TxtCalibData("Data/Calib.DPS.json") {};
	virtual ~TxtDeliveryPickupStationCalibData() {}

	bool load();
	bool saveDefault();
	bool save();

	int color_th[2];
	std::string uid_actions[5];
};


class TxtDeliveryPickupStation : public ft::TxtSimulationModel {
public:
	TxtDeliveryPickupStationCalibData calibData;

	TxtDeliveryPickupStation(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient = 0);
	virtual ~TxtDeliveryPickupStation();

	std::string nfcDeviceDeleteWriteRawRead(ft::TxtWPType_t c, std::vector<int64_t> vts, uint8_t mask_ts);
	std::string nfcDeviceWriteProducedRead(ft::TxtWPType_t c, std::vector<int64_t> vts, uint8_t mask_ts);
	std::string nfcDeviceWriteRejectedRead(ft::TxtWPType_t c, std::vector<int64_t> vts, uint8_t mask_ts);

	bool is_DIN();
	bool is_DOUT();

	std::string  getUIDCalibMode() { return calibData.uid_actions[0]; }
	std::string  getUIDResetHBW() { return calibData.uid_actions[1]; }
	std::string  getUIDOrderWHITE() { return calibData.uid_actions[2]; }
	std::string  getUIDOrderRED() { return calibData.uid_actions[3]; }
	std::string  getUIDOrderBLUE() { return calibData.uid_actions[4]; }

	void saveUIDCalibMode(std::string uid) { calibData.uid_actions[0]= uid; calibData.save(); }
	void saveUIDResetHBW(std::string uid) { calibData.uid_actions[1]= uid; calibData.save(); }
	void saveUIDOrderWHITE(std::string uid) { calibData.uid_actions[2]= uid; calibData.save(); }
	void saveUIDOrderRED(std::string uid) { calibData.uid_actions[3]= uid; calibData.save(); }
	void saveUIDOrderBLUE(std::string uid) { calibData.uid_actions[4]= uid; calibData.save(); }

	int readColorValue();
	ft::TxtWPType_t getLastColor();

    TxtNfcDevice* getNfc() { return &nfc; }
    TxtNfcData* getNfcData() { return nfc.getNfcData(); }
	std::string nfcReadUID();
	bool nfcDelete();
	std::string nfcRead();
    bool nfcWrite(TxtWorkpiece wp, std::vector<int64_t> vts, uint8_t mask_ts);

	bool getActiveDSI() { return activeDSI; }
	bool getActiveDSO() { return activeDSO; }
	bool getErrorDSI() { return errorDSI; }
	bool getErrorDSO() { return errorDSO; }

	void setActiveDSI(bool a) { activeDSI = a; Notify(); }
	void setActiveDSO(bool a) { activeDSO = a; Notify(); }
	void setErrorDSI(bool e) { errorDSI = e; Notify(); }
	void setErrorDSO(bool e) { errorDSO = e; Notify(); }

    void publishNfc() { nfc.Notify(); }

protected:
    void configInputs();

    void run();

	TxtNfcDevice nfc;
	int lastColorValue;

	bool activeDSI;
	bool activeDSO;
	bool errorDSI;
	bool errorDSO;
};


class TxtDeliveryPickupStationObserver : public ft::Observer {
public:
	TxtDeliveryPickupStationObserver(ft::TxtDeliveryPickupStation* s, ft::TxtMqttFactoryClient* mqttclient)
		: _subject(s), _mqttclient(mqttclient)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtDeliveryPickupStationObserver",0);
		_subject->Attach(this);
	}
	virtual ~TxtDeliveryPickupStationObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtDeliveryPickupStationObserver",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 1",0);

			int stDSI = _subject->is_DIN()?1:0;
			if (_subject->getErrorDSI()) stDSI = 4;
			assert(_mqttclient);
			_mqttclient->publishStateDSI((ft::TxtLEDSCode_t)stDSI, "", TIMEOUT_MS_PUBLISH, _subject->getActiveDSI()?1:0, "");

			int stDSO = _subject->is_DOUT()?1:0;
			if (_subject->getErrorDSO()) stDSO = 4;
			assert(_mqttclient);
			_mqttclient->publishStateDSO((ft::TxtLEDSCode_t)stDSO, "", TIMEOUT_MS_PUBLISH, _subject->getActiveDSO()?1:0, "");

			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 2",0);
		}
	}
private:
	ft::TxtDeliveryPickupStation *_subject;
	ft::TxtMqttFactoryClient* _mqttclient;
};


} /* namespace ft */


#endif /* TxtDeliveryPickupStation_H_ */
