/*
 * TxtFactoryTypes.h
 *
 *  Created on: 15.03.2019
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#ifndef TxtFactoryTypes_H_
#define TxtFactoryTypes_H_

#include <string>
#include <vector>
#include <pthread.h>
#include <assert.h>
#include <map>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "Utils.h"

namespace ft {


typedef enum
{
	LEDS_OFF = 0,
	LEDS_READY = 1,
	LEDS_BUSY = 2,
	LEDS_ERROR = 4,
	LEDS_CALIB = 7,
	LEDS_WAIT_ERROR =  6,
	LEDS_WAIT_READY = 3
} TxtLEDSCode_t;

class EncPos3 {
public:
	EncPos3(uint16_t x=0, uint16_t y=0, uint16_t z=0) : x(x), y(y), z(z) {}

	uint16_t x, y, z;
};

class EncPos2{
public:
	EncPos2(uint16_t x=0, uint16_t y=0) : x(x), y(y) {}

	uint16_t x, y;
};

typedef enum
{
	WP_TYPE_NONE,
	WP_TYPE_WHITE,
	WP_TYPE_RED,
	WP_TYPE_BLUE
} TxtWPType_t;

inline const char * toString(TxtWPType_t v)
{
	switch(v) {
	case WP_TYPE_NONE: return "NONE";
	case WP_TYPE_WHITE: return "WHITE";
	case WP_TYPE_RED: return "RED";
	case WP_TYPE_BLUE: return "BLUE";
	default: return "";
	}
}

typedef enum
{
	WP_STATE_NONE,
	WP_STATE_RAW,
	WP_STATE_PROCESSED,
	WP_STATE_REJECTED
} TxtWPState_t;

inline const char * toString(TxtWPState_t v)
{
	switch(v) {
	case WP_STATE_NONE: return "NONE";
	case WP_STATE_RAW: return "RAW";
	case WP_STATE_PROCESSED: return "PROCESSED";
	case WP_STATE_REJECTED: return "REJECTED";
	default: return "";
	}
}

class TxtWorkpiece {
public:
	TxtWorkpiece()
		: tag_uid(""), type(WP_TYPE_NONE), state(WP_STATE_RAW) {}
	TxtWorkpiece(const TxtWorkpiece& wp)
		: tag_uid(wp.tag_uid), type(wp.type), state(wp.state) {};
	TxtWorkpiece(std::string tag_uid, TxtWPType_t type, TxtWPState_t state)
		: tag_uid(tag_uid), type(type), state(state) {}
	virtual ~TxtWorkpiece() {}

	void printDebug() {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "WP uid:{} type:{} state:{}",tag_uid,toString(type), toString(state));
	}

	std::string tag_uid;
	TxtWPType_t type;
	TxtWPState_t state;
};

struct HbwFetchResult {
	HbwFetchResult(bool result): result(result) {}
	HbwFetchResult(bool result, TxtWorkpiece wp): result(result), wp(wp) {}
	bool result = false;
	TxtWorkpiece wp;
};

class TxtJoysticksData {
public:
	TxtJoysticksData(int aX1,int aY1,bool b1,int aX2,int aY2,bool b2)
		: aX1(aX1), aY1(aY1), b1(b1), aX2(aX2), aY2(aY2), b2(b2) {};
	TxtJoysticksData()
		: aX1(0), aY1(0), b1(false), aX2(0), aY2(0), b2(false) {};

	//copy constructor
	TxtJoysticksData(const TxtJoysticksData& jd) :
		aX1(jd.aX1),aY1(jd.aY1),b1(jd.b1), aX2(jd.aX2),aY2(jd.aY2),b2(jd.b2) {};

	int aX1;
	int aY1;
	bool b1;
	int aX2;
	int aY2;
	bool b2;
};

typedef std::map<std::string,TxtWorkpiece*> Stock_map_t;

//interface to fischertechnik Cloud:
typedef enum
{
	WAITING_FOR_ORDER_REMOTE,
	ORDERED_REMOTE,
	IN_PROCESS_REMOTE,
	SHIPPED_REMOTE
} TxtOrderStateRemote_t;

typedef enum
{
	WAITING_FOR_ORDER, //!
	ORDER_ACKNOWLEDGED, //!
	FETCHING_FROM_STORAGE,
	FETCHED_FROM_STORAGE,
	TRANSPORT_TO_PROCESS_STATION,
	IN_PROCESS,
	ORDER_PROCESSED, //!
	PROCESS_FAULT,
	WAITING_FOR_QA,
	TRANSPORT_WP_TO_DSO,
	SHIPPED, //!
	QA_FAULT,
	ORDER_FAULT
} TxtOrderState_t;

typedef enum
{
	WAITING_FOR_PICKUP,
	DSO_BLOCKED,
	PICKUP_ACKNOWLEDGED,
	FETCHING_WP,
	MOVE_WP_TO_DSO,
	WORKPIECE_READY,
	PICKUP_FAULT
} TxtPickupState_t;

typedef enum
{
	WAITING_FOR_STORE,
	BAD_WORKPIECE,
	STORE_ACKNOWLEDGED,
	DSO_EMPTY,
	IDENTIFICATION,
	STORING,
	STORE_DONE,
	STORE_FAULT
} TxtStoreState_t;

inline const char * toString(TxtOrderStateRemote_t v)
{
	switch(v) {
		//do not change the strings!
		case WAITING_FOR_ORDER_REMOTE: return "WAITING_FOR_ORDER";
		case ORDERED_REMOTE: return "ORDERED";
		case IN_PROCESS_REMOTE: return "IN_PROCESS";
		case SHIPPED_REMOTE: return "SHIPPED";
		default: return "[Unknown TxtOrderStateRemote_t]";
	}
}

inline const char * toString(TxtOrderState_t v)
{
	switch(v) {
		case WAITING_FOR_ORDER: return "WAITING FOR ORDER";
		case ORDER_ACKNOWLEDGED: return "ORDER ACKNOWLEDGED";
		case FETCHING_FROM_STORAGE: return "FETCHING WORKPIECE FROM STORAGE";
		case FETCHED_FROM_STORAGE: return "WORKPIECE FETCHED FROM STORAGE";
		case TRANSPORT_TO_PROCESS_STATION: return "TRANSPORT TO PROCESSING STATION";
		case IN_PROCESS: return "WORKPIECE IN PROCESS";
		case ORDER_PROCESSED: return "WORKPIECE PROCESSED";
		case PROCESS_FAULT: return "PROCESSING FAULT, ORDER IS RESCHEDULED";
		case WAITING_FOR_QA: return "QUALITY ASSURANCE";
		case TRANSPORT_WP_TO_DSO: return "TRANSPORT WORKPIECE TO PICKUP LOCATION";
		case SHIPPED: return "ORDER SHIPPED";
		case ORDER_FAULT: return "ORDER FAULT";
		case QA_FAULT: return "QA FAULT";
		default: return "[Unknown TxtOrderState_t]";
	}
}

inline const char * toString(TxtPickupState_t v) {
	switch(v) {
		case WAITING_FOR_PICKUP: return "WAITING FOR PICKUP";
		case PICKUP_ACKNOWLEDGED: return "PICKUP ACKNOWLEDGED";
		case DSO_BLOCKED: return "DSO_BLOCKED";
		case FETCHING_WP: return "FETCHING WORKPIECE";
		case TxtPickupState_t::MOVE_WP_TO_DSO: return "MOVE WORKPIECE TO PICKUP LOCATION";
		case WORKPIECE_READY: return "WORKPIECE READY";
		case PICKUP_FAULT: return "PICKUP FAULT";
		default: return "[Unknown TxtPickupState_t]";
	}
}

inline const char * toString(TxtStoreState_t v) {
	switch(v) {
		case WAITING_FOR_STORE: return "WAITING FOR STORE";
		case BAD_WORKPIECE: return "BAD WORKPIECE";
		case STORE_ACKNOWLEDGED: return "STORE ACKNOWLEDGED";
		case DSO_EMPTY: return "DSO EMPTY";
		case IDENTIFICATION: return "WORKPIECE IDENTIFICATION";
		case STORING: return "STORING";
		case STORE_DONE: return "STORE DONE";
		case STORE_FAULT: return "STORE FAULT";
		default: return "[Unknown TxtStoreState_t]";
	}
}

struct TxtWorkflowState {
	TxtWorkflowState(): uid("") {}
	TxtWorkflowState(std::string uid): uid(uid) {}
	TxtWorkflowState(std::string uid, TxtWorkpiece wp): uid(uid), wp(wp) {}
	std::string uid;
	TxtWorkpiece wp;
};

struct TxtOrderState: TxtWorkflowState {
	TxtOrderState(): TxtWorkflowState() {}
	TxtOrderState(TxtOrderState_t state, TxtWorkpiece wp): TxtWorkflowState("order_" + std::to_string(getnowtimestamp_s()), wp), state(state) {}
	TxtOrderState_t state;
};

struct TxtPickupState: TxtWorkflowState {
	TxtPickupState(): TxtWorkflowState() {}
	TxtPickupState(TxtPickupState_t state, TxtWorkpiece wp): TxtWorkflowState("pickup_" + std::to_string(getnowtimestamp_s()), wp), state(state) {}
	TxtPickupState_t state;
};

struct TxtStoreState: TxtWorkflowState {
	TxtStoreState(): TxtWorkflowState() {}
	TxtStoreState(TxtStoreState_t state): TxtWorkflowState("store_" + std::to_string(getnowtimestamp_s())), state(state) {}
	TxtStoreState_t state;
};

struct TxtRequestQuit {
	TxtRequestQuit(): resume(true), timestamp_s(ft::getnowtimestamp_s() + 10) {} //10 Seconds time window, where faults are ignored, unlikely to happen
	TxtRequestQuit(bool b): resume(b) {}
	bool resume = false;
	double timestamp_s = 0;
};

typedef enum
{
	UNDEFINED = 0,
	NORMAL = 1,
	STORE_AFTER_PRODUCE = 2,
	ENDLESS = 3
} TxtVgrWorkingModes_t;

typedef enum
{
	INVALID_INDEX = -1,
	DELIVERY_RAW_INDEX = 0,
	INSPECTION_INDEX = 1,
	WAREHOUSING_INDEX = 2,
	OUTSOURCING_INDEX = 3,
	PROCESSING_OVEN_INDEX = 4,
	PROCESSING_MILLING_INDEX = 5,
	SORTING_INDEX = 6,
	SHIPPING_INDEX = 7,
	NUM_INDEX_MAX
} TxtHistoryIndex_t;

typedef enum
{
	INVALID = 0,
	DELIVERY_RAW = 100,       // = "Anlieferung Rohware",
	INSPECTION = 200,         // = "Qualitaetskontrolle",
	WAREHOUSING = 300,        // = "Einlagerung",
	OUTSOURCING = 400,        // = "Auslagerung",
	PROCESSING_OVEN = 500,    // = "Bearbeitung Brennofen",
	PROCESSING_MILLING = 600, // = "Bearbeitung Fraese",
	SORTING = 700,            // = "Sortierung",
	SHIPPING = 800            // = "Versand Ware"
} TxtHistoryCode_t;

typedef enum {
	UNDEFINED_FAULT = 0,
	INVALID_WP_TYPE,
	WRONG_WP_TYPE,
	REQUEST_TIMEOUT,
	TRANSPORT_FAULT,
	CNT_NOT_AVAILABLE,
	WP_NOT_AVAILABLE,
	STORAGE_FAULT,

} TxtFaultCodes_t;

inline const TxtHistoryCode_t toCode(TxtHistoryIndex_t v)
{
	switch(v)
	{
	case DELIVERY_RAW_INDEX: return DELIVERY_RAW;
	case INSPECTION_INDEX: return INSPECTION;
	case WAREHOUSING_INDEX: return WAREHOUSING;
	case OUTSOURCING_INDEX: return OUTSOURCING;
	case PROCESSING_OVEN_INDEX: return PROCESSING_OVEN;
	case PROCESSING_MILLING_INDEX: return PROCESSING_MILLING;
	case SORTING_INDEX: return SORTING;
	case SHIPPING_INDEX: return SHIPPING;
	default: return INVALID;
	}
}

inline const TxtHistoryIndex_t toIndex(TxtHistoryCode_t v)
{
	switch(v)
	{
	case DELIVERY_RAW: return DELIVERY_RAW_INDEX;
	case INSPECTION: return INSPECTION_INDEX;
	case WAREHOUSING: return WAREHOUSING_INDEX;
	case OUTSOURCING: return OUTSOURCING_INDEX;
	case PROCESSING_OVEN: return PROCESSING_OVEN_INDEX;
	case PROCESSING_MILLING: return PROCESSING_MILLING_INDEX;
	case SORTING: return SORTING_INDEX;
	case SHIPPING: return SHIPPING_INDEX;
	default: return INVALID_INDEX;
	}
}

inline const char * toString(TxtHistoryCode_t v)
{
	switch(v)
	{
	case DELIVERY_RAW: return "DELIVERY_RAW";
	case INSPECTION: return "INSPECTION";
	case WAREHOUSING: return "WAREHOUSING";
	case OUTSOURCING: return "OUTSOURCING";
	case PROCESSING_OVEN: return "PROCESSING_OVEN";
	case PROCESSING_MILLING: return "PROCESSING_MILLING";
	case SORTING: return "SORTING";
	case SHIPPING: return "SHIPPING";
	default: return "[Unknown TxtHistoryCode_t]";
	}
}

typedef std::map<TxtHistoryCode_t,int64_t> HistoryCode_map_t;


} /* namespace ft */


#endif /* TxtFactoryTypes_H_ */
