/*
 * TxtHighBayWarehouseStorage.h
 *
 *  Created on: 18.02.2019
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#ifndef TXTHIGHBAYWAREHOUSESTORAGE_H_
#define TXTHIGHBAYWAREHOUSESTORAGE_H_


#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>
#include <thread>
#include <map>

#include "TxtMqttFactoryClient.h"
#include "TxtFactoryTypes.h"
#include "Observer.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


struct StoragePos2 {
	int x, y;
};


class TxtHighBayWarehouseStorage : public SubjectObserver {
public:
	TxtHighBayWarehouseStorage();
	virtual ~TxtHighBayWarehouseStorage();

	bool loadStorageState();
	bool saveStorageState();
	void resetStorageState();

	bool store(TxtWorkpiece _wp);
	bool storeContainer();
	HbwFetchResult fetchByType(TxtWorkpiece txt_wp);
	HbwFetchResult fetch(TxtWorkpiece txt_wp);
	bool fetchContainer();

	StoragePos2 getNextStorePos() { return nextFetchPos; } //nextStorePos; }
	StoragePos2 getNextFetchPos() { return nextFetchPos; }
	StoragePos2 getCurrentPos() { return currentPos; }

	bool isValidPos(StoragePos2 p);
	bool canColorBeStored(TxtWPType_t c);

	Stock_map_t getStockMap();

protected:
	std::string filename;

	char charType(int x, int y);
	void print();

	TxtWorkpiece * wp[3][3]; //workpiece
	bool wpc[3][3]; //container
	double storageTimeStamps[3][3];

	StoragePos2 currentPos;
	//StoragePos2 nextStorePos;
	StoragePos2 nextFetchPos;
};


class TxtHighBayWarehouseStorageObserver : public ft::Observer {
public:
	TxtHighBayWarehouseStorageObserver(ft::TxtHighBayWarehouseStorage* s, ft::TxtMqttFactoryClient* mqttclient)
		: _subject(s), _mqttclient(mqttclient)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtHighBayWarehouseStorageObserver",0);
		_subject->Attach(this);
	}
	virtual ~TxtHighBayWarehouseStorageObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtHighBayWarehouseStorageObserver",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 1",0);

			Stock_map_t map_wps = _subject->getStockMap();
			_mqttclient->publishStock(map_wps, TIMEOUT_MS_PUBLISH);

			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 2",0);
		}
	}
private:
	ft::TxtHighBayWarehouseStorage *_subject;
	ft::TxtMqttFactoryClient* _mqttclient;
};


} /* namespace ft */


#endif /* TXTHIGHBAYWAREHOUSESTORAGE_H_ */
