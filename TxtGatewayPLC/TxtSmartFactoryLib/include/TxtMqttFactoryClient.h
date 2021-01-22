/*
 * TxtMqttFactoryClient.h
 *
 *  Created on: 22.01.2018
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#ifndef TxtMqttFactoryClient_H_
#define TxtMqttFactoryClient_H_

#include <mqtt/client.h>
#include <json/json.h>

#include "Utils.h"
#include "TxtFactoryTypes.h"

#include "spdlog/spdlog.h"


#define TIMEOUT_MS_PUBLISH 5000

template<class T> std::string toString(const T& t)
{
     std::ostringstream stream;
     stream << t;
     return stream.str();
}

template<class T> T fromString(const std::string& s)
{
     std::istringstream stream (s);
     T t;
     stream >> t;
     return t;
}


namespace ft {


typedef enum
{
	MPO_EXIT=0,
	MPO_STARTED=1,
	MPO_PRODUCED=2,
	MPO_TRANSPORT=3
} TxtMpoAckCode_t;

typedef enum
{
	VGR_EXIT=0,
	VGR_HBW_FETCHCONTAINER=1,
	VGR_HBW_STORE_WP=2,
	VGR_HBW_FETCH_WP=3,
	VGR_HBW_STORECONTAINER=4,
	VGR_HBW_RESETSTORAGE=5,
	VGR_HBW_CALIB=6,
	VGR_MPO_PRODUCE=7,
	VGR_SLD_START=8,
	VGR_SLD_CALIB=9
} TxtVgrDoCode_t;

typedef enum
{
	HBW_EXIT=0,
	HBW_FETCHED=1,
	HBW_STORED=2,
	HBW_CALIB_NAV=3,
	HBW_CALIB_END=4
} TxtHbwAckCode_t;


typedef enum
{
	SLD_EXIT=0,
	SLD_STARTED=1,
	SLD_SORTED=2,
	SLD_CALIB_END=3
} TxtSldAckCode_t;


class action_listener_subscribe : public virtual mqtt::iaction_listener
{
	void on_failure(const mqtt::token& tok) override {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "on_failure", 0);
		if (tok.get_message_id() != 0)
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  for token: [{}]", tok.get_message_id());
	}

	void on_success(const mqtt::token& tok) override {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "on_success", 0);
		if (tok.get_message_id() != 0)
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  for token: [{}]", tok.get_message_id());
		auto topics_ = tok.get_topics();
		if (topics_ && !topics_->empty()) {
			for(unsigned int i = 0; i < topics_->size(); i++) {
				std::string topic0 = (*topics_)[i];
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  token topic: '{}'", topic0);
			}
		}
	}

public:
	action_listener_subscribe() {};
};


class action_listener_publish : public virtual mqtt::iaction_listener
{
	void on_failure(const mqtt::token& tok) override {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "on_failure", 0);
		if (tok.get_message_id() != 0)
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  for token: [{}]", tok.get_message_id());
	}

	void on_success(const mqtt::token& tok) override {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "on_success", 0);
		if (tok.get_message_id() != 0)
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  for token: [{}]", tok.get_message_id());
		auto topics_ = tok.get_topics();
		if (topics_ && !topics_->empty()) {
			for(unsigned int i = 0; i < topics_->size(); i++) {
				std::string topic0 = (*topics_)[i];
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  token topic: '{}'", topic0);
			}
		}
	}

public:
	action_listener_publish() {};
};


//smart home
#define TOPIC_INPUT_BME680      "i/bme680"
#define TOPIC_INPUT_LDR         "i/ldr"
#define TOPIC_INPUT_CAM         "i/cam"
#define TOPIC_INPUT_PTUPOS      "i/ptu/pos"
#define TOPIC_INPUT_ALERT       "i/alert"
#define TOPIC_INPUT_BROADCAST   "i/broadcast"

#define TOPIC_OUTPUT_PTU        "o/ptu"

#define TOPIC_CONFIG_LINK       "c/link"
#define TOPIC_CONFIG_BME680     "c/bme680"
#define TOPIC_CONFIG_LDR        "c/ldr"
#define TOPIC_CONFIG_CAM        "c/cam"

//factory remote
#define TOPIC_INPUT_STATE_       "f/i/state/" // hbw, vgr, mpo, sld, dsi, dso
#define TOPIC_INPUT_STATE_HBW    "f/i/state/hbw"
#define TOPIC_INPUT_STATE_VGR    "f/i/state/vgr"
#define TOPIC_INPUT_STATE_MPO    "f/i/state/mpo"
#define TOPIC_INPUT_STATE_SLD    "f/i/state/sld"
#define TOPIC_INPUT_STATE_DSI    "f/i/state/dsi"
#define TOPIC_INPUT_STATE_DSO    "f/i/state/dso"
#define TOPIC_INPUT_STOCK        "f/i/stock"
#define TOPIC_INPUT_STATE_ORDER  "f/i/order" //local+remote
#define TOPIC_INPUT_NFC_DS       "f/i/nfc/ds"

#define TOPIC_OUTPUT_STATE_ACK   "f/o/state/ack"
#define TOPIC_OUTPUT_ORDER       "f/o/order"
#define TOPIC_OUTPUT_NFC_DS      "f/o/nfc/ds"


//factory local
#define TOPIC_OUTPUT_WM_CHANGE	 "fl/o/wm/change"
#define TOPIC_OUTPUT_PICKUP      "fl/o/pickup"
#define TOPIC_OUTPUT_STORE       "fl/o/store"
#define TOPIC_LOCAL_NFC_DS_ACK   "fl/o/nfc/ds"

#define TOPIC_INPUT_WM			 "fl/i/wm/state"
#define TOPIC_INPUT_QUEUE_ORDERS "fl/i/orders"
#define TOPIC_INPUT_QUEUE_PICKUPS "fl/i/pickups"
#define TOPIC_INPUT_STATE_PICKUP "fl/i/pickup"
#define TOPIC_INPUT_STATE_STORE  "fl/i/store"
#define TOPIC_LOCAL_NFC_DS_CMD   "fl/i/nfc/ds"

#define TOPIC_LOCAL_BROADCAST    "fl/broadcast"
#define TOPIC_LOCAL_SSC_JOY      "fl/ssc/joy"
#define TOPIC_LOCAL_MPO_ACK      "fl/mpo/ack"
#define TOPIC_LOCAL_MPO_FAULT    "fl/mpo/fault"
#define TOPIC_LOCAL_VGR_DO       "fl/vgr/do"
#define TOPIC_LOCAL_HBW_ACK      "fl/hbw/ack"
#define TOPIC_LOCAL_HBW_FAULT    "fl/hbw/fault"
#define TOPIC_LOCAL_SLD_ACK      "fl/sld/ack"
#define TOPIC_LOCAL_SLD_FAULT	 "fl/sld/fault"


class TxtMqttFactoryClient {
public:
	TxtMqttFactoryClient(std::string clientname, std::string clientNamePrefix, std::string host, std::string port,
			std::string mqtt_user, mqtt::binary_ref mqtt_pass, bool bretained=false, int iqos=1);
	virtual ~TxtMqttFactoryClient();

	bool is_connected() { return cli.is_connected(); }

	bool connect(long int timeout);
	void disconnect(long int timeout);

	void set_callback(mqtt::callback& cb) { cli.set_callback(cb); }

	bool start_consume(long int timeout);
	//mqtt::const_message_ptr consume_message() { return cli.consume_message(); }

	//Smart Home remote
	void publishLDR(double timestamp_s, int16_t ldr, long timeout);
	void publishPtuPos(float pan, float tilt, long timeout);
	void publishCam(const std::string sdata, long timeout);
	void publishBme680(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
		float pressure, float raw_temperature, float raw_humidity, float gas, long timeout);
	void publishAlert(bool st, const std::string id, const std::string sdata, int code, long timeout);
	void publishBroadcast(double timestamp_s, const std::string sw, const std::string ver, const std::string message, long timeout);

	//Factory remote
	void publishStateSLD(TxtLEDSCode_t code, const std::string desc, long timeout, int active=-1, const std::string target="") { publishStateStation("sld",code,desc,timeout,active,target); }
	void publishStateVGR(TxtLEDSCode_t code, const std::string desc, long timeout, int active=-1, const std::string target="") { publishStateStation("vgr",code,desc,timeout,active,target); }
	void publishStateDSI(TxtLEDSCode_t code, const std::string desc, long timeout, int active=-1, const std::string target="") { publishStateStation("dsi",code,desc,timeout,active,target); }
	void publishStateDSO(TxtLEDSCode_t code, const std::string desc, long timeout, int active=-1, const std::string target="") { publishStateStation("dso",code,desc,timeout,active,target); }
	void publishStateMPO(TxtLEDSCode_t code, const std::string desc, long timeout, int active=-1, const std::string target="") { publishStateStation("mpo",code,desc,timeout,active,target); }
	void publishStateHBW(TxtLEDSCode_t code, const std::string desc, long timeout, int active=-1, const std::string target="") { publishStateStation("hbw",code,desc,timeout,active,target); }
	void publishStock(Stock_map_t map_wps, long timeout);
	void publishStateOrder(TxtOrderState ord_state, long timeout);
	void publishStatePickup(TxtPickupState pickup_state, long timeout);
	void publishStateStore(TxtStoreState store_state, long timeout);
	void publishVGROrderQueue(std::queue<TxtOrderState> q, long timeout);
	void publishVGRPickupQueue(std::queue<TxtPickupState> q, long timeout);
	void publishNfcDS(TxtWorkpiece* wp, HistoryCode_map_t map_hist, long timeout);
	bool getNfcRemote() { return nfc_remote; }
	void setNfcRemote(bool r) { nfc_remote = r; }

	//Factory local
	void publishStationBroadcast(const std::string station, double timestamp_s, const std::string sw, const std::string ver, const std::string message, long timeout);
	void publishSSC_Joy(TxtJoysticksData jd, long timeout);
	void publishMPO_Ack(TxtMpoAckCode_t code, TxtWorkflowState* request, long timeout);
	void publishMPO_Fault(TxtFaultCodes_t code, TxtWorkflowState* request, long timeout);
	void publishVGR_Do(TxtVgrDoCode_t code, TxtWorkflowState* state, long timeout);
	void publishVGR_Order(TxtWPType_t t, long timeout);
	void pusblishWorkingModeChanged(TxtVgrWorkingModes_t wm, long timeout);
	void publishHBW_Ack(TxtHbwAckCode_t code, TxtWorkpiece* wp, long timeout);
	void publishHBW_Fault(TxtFaultCodes_t code, TxtWorkpiece wp, long timeout);
	void publishSLD_Ack(TxtSldAckCode_t code, TxtWorkflowState* request, long timeout);
	void publishSLD_Fault(TxtFaultCodes_t code,TxtWorkflowState* request, long timeout);

protected:
	//Factory remote
	void publishStateStation(const std::string station, TxtLEDSCode_t code, const std::string desc, long timeout, int active=-1, const std::string target="");

	void subTopic(const std::string& topicFilter, long int timeout);
	void unsubTopic(const std::string& topicFilter, long int timeout);

	std::string clientname;
	mqtt::string host;
	mqtt::string port;
	mqtt::string mqtt_user;
	mqtt::binary_ref mqtt_pass;
	bool bretained;
	int iqos;
	mqtt::async_client cli;
	mqtt::connect_options connOpts;

	pthread_mutex_t m_mutex;

	action_listener_subscribe aListSub;
	action_listener_publish aListPub;

	bool nfc_remote;
};


} /* namespace ft */


#endif /* TxtMqttFactoryClient_H_ */
