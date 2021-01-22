/*
 * TxtMqttFactoryClient.cpp
 *
 *  Created on: 22.01.2018
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

/* DEBUG
 *  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/knobloch/libs/
 *  export MQTT_C_CLIENT_TRACE=ON
 *  export MQTT_C_CLIENT_TRACE_LEVEL=PROTOCOL
 */

#include "Utils.h"

#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <iomanip>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <TxtMqttFactoryClient.h>

#include "spdlog/spdlog.h"

#define FORCE_EXIT_ON_TIMEOUT

extern char TxtAppVer[32];


namespace ft {


/* Error codes greater than 0 are returned by the MQTT protocol:*/
std::string getMQTTReasonCodeString(int rc) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getMQTTReasonCodeString rc:{}", rc);
	std::string s;
	switch(rc) {
		case 0: s = "MQTTCLIENT_SUCCESS: No error. Indicates successful completion of an MQTT client operation."; break;
		case -1: s = "MQTTCLIENT_FAILURE: A generic error code indicating the failure of an MQTT client operation."; break;
		case -2: s = "MQTTCLIENT_PERSISTENCE_ERROR"; break;
		case -3: s = "MQTTCLIENT_DISCONNECTED: The client is disconnected."; break;
		case -4: s = "MQTTCLIENT_MAX_MESSAGES_INFLIGHT: The maximum number of messages allowed to be simultaneously in-flight has been reached."; break;
		case -5: s = "MQTTCLIENT_BAD_UTF8_STRING: An invalid UTF-8 string has been detected."; break;
		case -6: s = "MQTTCLIENT_NULL_PARAMETER: A NULL parameter has been supplied when this is invalid."; break;
		case -7: s = "MQTTCLIENT_TOPICNAME_TRUNCATED: The topic has been truncated (the topic string includes embedded NULL characters). String functions will not access the full topic. Use the topic length value to access the full topic."; break;
		case -8: s = "MQTTCLIENT_BAD_STRUCTURE: A structure parameter does not have the correct eyecatcher and version number."; break;
		case -9: s = "MQTTCLIENT_BAD_QOS: A QoS value that falls outside of the acceptable range (0,1,2)"; break;
		case -10: s = "MQTTCLIENT_SSL_NOT_SUPPORTED: Attempting SSL connection using non-SSL version of library"; break;
		case -14: s = "MQTTCLIENT_BAD_PROTOCOL: protocol prefix in serverURI should be tcp:// or ssl://"; break;
		case 1: s = "Connection refused: Unacceptable protocol version"; break;
		case 2: s = "Connection refused: Identifier rejected"; break;
		case 3: s = "Connection refused: Server unavailable"; break;
		case 4: s = "Connection refused: Bad user name or password"; break;
		case 5: s = "Connection refused: Not authorized"; break;
		case 6-255: s = "Reserved for future use"; break;
		default: s = "Unknown error.";
	}
	return s;
}

TxtMqttFactoryClient::TxtMqttFactoryClient(std::string clientname, std::string clientNamePrefix, std::string host, std::string port,
		std::string mqtt_user, mqtt::binary_ref mqtt_pass, bool bretained, int iqos)
	: clientname(clientname), host(host), port(port), mqtt_user(mqtt_user), mqtt_pass(mqtt_pass),
	  bretained(bretained), iqos(iqos),
	cli("tcp://" + host + ":" + port, clientNamePrefix+clientname+"V"+std::string(TxtAppVer)), aListSub(), aListPub(), nfc_remote(true)
	//client name exist only once!
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtMqttFactoryClient clientname:{} host:{} port:{} mqtt_user:{}", clientNamePrefix+clientname, host, port, mqtt_user);
	connOpts.set_connect_timeout(90);
	connOpts.set_keep_alive_interval(60);
	connOpts.set_clean_session(true);
	connOpts.set_automatic_reconnect(true);
	//connOpts.set_max_inflight(20);
	connOpts.set_mqtt_version(MQTTVERSION_3_1_1);
	connOpts.set_user_name(mqtt_user);
	connOpts.set_password(mqtt_pass);
	/*for(unsigned int i = 0; i < mqtt_pass.size(); i++ ) {
		printf("%02X ", mqtt_pass[i]);
	}
	std::cout << std::endl;*/

	if (clientname == "TxtFactoryMain")
	{
		char sts[25];
		ft::getnowstr(sts);
		Json::Value js_pos;
		std::ostringstream sout_pos;
		try {
			js_pos["ts"] = sts;
			js_pos["pan"] = -100.f;
			js_pos["tilt"] = -100.f;
			sout_pos << js_pos;
			try {
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_PTUPOS);
				mqtt::message willmsg_pos(TOPIC_INPUT_PTUPOS, sout_pos.str(), 1, true);
				mqtt::will_options will_pos(willmsg_pos);
				connOpts.set_will(will_pos);
				spdlog::get("console")->info("willmsg_ptupos: ", sout_pos.str());
			} catch (const mqtt::exception& exc) {
				std::cout << "publishPtuPos: " << exc.what() << " "
						<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
			}
		} catch (const Json::RuntimeError& exc) {
			std::cout << "Error: " << exc.what() << std::endl;
		}
	}

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &attr);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_init",0);
}

TxtMqttFactoryClient::~TxtMqttFactoryClient() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtMqttFactoryClient",0);
	disconnect(1000);
	pthread_mutex_destroy(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_destroy",0);
}

bool TxtMqttFactoryClient::connect(long int timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "connect",0);
	try {
		mqtt::token_ptr conntok = cli.connect(connOpts);
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "conntok->wait_for {}", timeout);
		return conntok->wait_for(timeout);
	/*} catch (const mqtt::security_exception& exc) {
		std::cout << "security_exception: " << exc.what() << std::endl;
	} catch (const mqtt::persistence_exception& exc) {
		std::cout << "persistence_exception: " << exc.what() << std::endl;*/
	} catch (const mqtt::exception& exc) {
		std::cout << "connect: " << exc.what() << " "
				<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
	}
	return false;
}

void TxtMqttFactoryClient::unsubTopic(const std::string& topicFilter, long int timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "topic: {}", topicFilter);
	mqtt::token_ptr conntok = cli.unsubscribe(topicFilter);
	bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
	if (!r) exit(1);
#endif
}

void TxtMqttFactoryClient::disconnect(long int timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "disconnect",0);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock disconnect",0);
	try {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "unsubscribe",0);

		if (clientname == "TxtGatewayPLC")
		{
			//remote + local
			unsubTopic(TOPIC_CONFIG_LINK, timeout);
			unsubTopic(TOPIC_CONFIG_BME680, timeout);
			unsubTopic(TOPIC_CONFIG_LDR, timeout);
			unsubTopic(TOPIC_CONFIG_CAM, timeout);
			unsubTopic(TOPIC_OUTPUT_NFC_DS, timeout);
			unsubTopic(TOPIC_LOCAL_NFC_DS_ACK, timeout);
		}
		else if (clientname == "TxtFactoryMain")
		{
			//remote
			unsubTopic(TOPIC_CONFIG_LINK, timeout);
			unsubTopic(TOPIC_CONFIG_BME680, timeout);
			unsubTopic(TOPIC_CONFIG_LDR, timeout);
			unsubTopic(TOPIC_CONFIG_CAM, timeout);
			unsubTopic(TOPIC_OUTPUT_PTU, timeout);
			//local
			unsubTopic(TOPIC_LOCAL_BROADCAST, timeout);
			unsubTopic(TOPIC_INPUT_STATE_VGR, timeout);
			unsubTopic(TOPIC_INPUT_STATE_HBW, timeout);
			unsubTopic(TOPIC_INPUT_STATE_MPO, timeout);
			unsubTopic(TOPIC_INPUT_STATE_SLD, timeout);
			unsubTopic(TOPIC_INPUT_STATE_DSI, timeout);
			unsubTopic(TOPIC_INPUT_STATE_DSO, timeout);
		}
		else if (clientname == "TxtFactoryMPO")
		{
			//remote
			unsubTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			//local
			unsubTopic(TOPIC_LOCAL_VGR_DO, timeout);
		}
		else if (clientname == "TxtFactoryHBW")
		{
			//remote
			unsubTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			//local
			unsubTopic(TOPIC_LOCAL_VGR_DO, timeout);
			unsubTopic(TOPIC_LOCAL_SSC_JOY, timeout);
		}
		else if (clientname == "TxtFactoryVGR")
		{
			//remote
			unsubTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			unsubTopic(TOPIC_OUTPUT_ORDER, timeout);
			unsubTopic(TOPIC_OUTPUT_PICKUP, timeout);
			unsubTopic(TOPIC_OUTPUT_STORE, timeout);
			unsubTopic(TOPIC_OUTPUT_NFC_DS, timeout);
			unsubTopic(TOPIC_OUTPUT_WM_CHANGE, timeout);
			//local
			unsubTopic(TOPIC_LOCAL_SSC_JOY, timeout);
			unsubTopic(TOPIC_LOCAL_MPO_ACK, timeout);
			unsubTopic(TOPIC_LOCAL_MPO_FAULT, timeout);
			unsubTopic(TOPIC_LOCAL_HBW_ACK, timeout);
			unsubTopic(TOPIC_LOCAL_HBW_FAULT, timeout);
			unsubTopic(TOPIC_LOCAL_SLD_ACK, timeout);
			unsubTopic(TOPIC_LOCAL_SLD_FAULT, timeout);
			unsubTopic(TOPIC_INPUT_BME680, timeout);
		}
		else if (clientname == "TxtFactorySLD")
		{
			//remote
			unsubTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			//local
			unsubTopic(TOPIC_LOCAL_SSC_JOY, timeout);
			unsubTopic(TOPIC_LOCAL_MPO_ACK, timeout);
			unsubTopic(TOPIC_LOCAL_VGR_DO, timeout);
	}
		else
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error: unknown name: {}",clientname);
			exit(1);
		}

		//stop_consuming
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "stop_consuming",0);
		//cli.stop_consuming();

		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "disconnect",0);
		mqtt::token_ptr conntok = cli.disconnect();
		bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
		if (!r) exit(1);
#endif

	} catch (const mqtt::exception& exc) {
		std::cout << "disconnect main: " << exc.what() << " "
				<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock disconnect",0);
}

void TxtMqttFactoryClient::subTopic(const std::string& topicFilter, long int timeout)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", topicFilter);
	mqtt::token_ptr conntok_link = cli.subscribe(topicFilter, 1, nullptr, aListSub);
	/*bool rc_link = conntok_link->wait_for(timeout);
	if (!rc_link) {
		std::cout << "Timeout: " << std::endl;
	}*/
}

bool TxtMqttFactoryClient::start_consume(long int timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "start_consume",0);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock start_consume",0);
	bool ret = false;
	try {

		//start_consuming
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "start_consuming",0);
		//cli.start_consuming();

		//subscribe
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "subscribe",0);
		//cli.subscribe("#", 1, nullptr, aListSub);

		if (clientname == "TxtGatewayPLC")
		{
			//remote + local
			subTopic(TOPIC_CONFIG_LINK, timeout);
			subTopic(TOPIC_CONFIG_BME680, timeout);
			subTopic(TOPIC_CONFIG_LDR, timeout);
			subTopic(TOPIC_CONFIG_CAM, timeout);
			subTopic(TOPIC_OUTPUT_NFC_DS, timeout);
			subTopic(TOPIC_LOCAL_NFC_DS_ACK, timeout);
		}
		else if (clientname == "TxtFactoryMain")
		{
			//remote
			subTopic(TOPIC_CONFIG_LINK, timeout);
			subTopic(TOPIC_CONFIG_BME680, timeout);
			subTopic(TOPIC_CONFIG_LDR, timeout);
			subTopic(TOPIC_CONFIG_CAM, timeout);
			subTopic(TOPIC_OUTPUT_PTU, timeout);
			//local
			subTopic(TOPIC_LOCAL_BROADCAST, timeout);
			subTopic(TOPIC_INPUT_STATE_VGR, timeout);
			subTopic(TOPIC_INPUT_STATE_HBW, timeout);
			subTopic(TOPIC_INPUT_STATE_MPO, timeout);
			subTopic(TOPIC_INPUT_STATE_SLD, timeout);
			subTopic(TOPIC_INPUT_STATE_DSI, timeout);
			subTopic(TOPIC_INPUT_STATE_DSO, timeout);
		}
		else if (clientname == "TxtFactoryMPO")
		{
			//remote
			subTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			//local
			subTopic(TOPIC_LOCAL_VGR_DO, timeout);
		}
		else if (clientname == "TxtFactoryHBW")
		{
			//remote
			subTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			//local
			subTopic(TOPIC_LOCAL_VGR_DO, timeout);
			subTopic(TOPIC_LOCAL_SSC_JOY, timeout);
		}
		else if (clientname == "TxtFactoryVGR")
		{
			//remote
			subTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			subTopic(TOPIC_OUTPUT_ORDER, timeout);
			subTopic(TOPIC_OUTPUT_PICKUP, timeout);
			subTopic(TOPIC_OUTPUT_STORE, timeout);
			subTopic(TOPIC_OUTPUT_NFC_DS, timeout);
			subTopic(TOPIC_OUTPUT_WM_CHANGE, timeout);
			//local
			subTopic(TOPIC_LOCAL_SSC_JOY, timeout);
			subTopic(TOPIC_LOCAL_MPO_ACK, timeout);
			subTopic(TOPIC_LOCAL_MPO_FAULT, timeout);
			subTopic(TOPIC_LOCAL_HBW_ACK, timeout);
			subTopic(TOPIC_LOCAL_HBW_FAULT, timeout);
			subTopic(TOPIC_LOCAL_SLD_ACK, timeout);
			subTopic(TOPIC_LOCAL_SLD_FAULT, timeout);
			subTopic(TOPIC_INPUT_BME680, timeout);
		}
		else if (clientname == "TxtFactorySLD")
		{
			//remote
			subTopic(TOPIC_OUTPUT_STATE_ACK, timeout);
			//local
			subTopic(TOPIC_LOCAL_SSC_JOY, timeout);
			subTopic(TOPIC_LOCAL_MPO_ACK, timeout);
			subTopic(TOPIC_LOCAL_VGR_DO, timeout);
		}
		else
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error: unknown name: {}",clientname);
			exit(1);
		}

		ret = true;
	} catch (const mqtt::exception& exc) {
		std::cout << "subscribe: " << exc.what() << " "
				<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock start_consume",0);
	return ret;
}

void TxtMqttFactoryClient::publishLDR(double timestamp_s, int16_t ldr, long timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishLDR ldr:{} timeout:{}", ldr, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishLDR",0);
	if ((ldr < 0)||(ldr > 15000)) {
		std::cout << "Warning: LDR raw value out of range!" << std::endl;
	}
	char sts[25];
	ft::gettimestampstr(timestamp_s*1000000000., sts);
	Json::Value js_ldr;
	Json::Value js_br100;
	std::ostringstream sout_ldr;
	try {
		js_ldr["ts"] = sts;
		js_ldr["ldr"] = ldr;
		float br = (15000 - ldr)/150.f;
		js_ldr["br"] = ftos(br, 1);
		sout_ldr << js_ldr;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_LDR);
			auto msg_sldr = mqtt::make_message(TOPIC_INPUT_LDR, sout_ldr.str());//sldr.c_str());
			msg_sldr->set_qos(iqos);
			msg_sldr->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish ldr: {} ldr:{} br:{}", sts, ldr, br);
			mqtt::token_ptr conntok = cli.publish(msg_sldr, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishLDR: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishLDR",0);
}

void TxtMqttFactoryClient::publishPtuPos(float pan, float tilt, long timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishPtuPos pan:{} tilt:{} timeout:{}", pan, tilt, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishPtuPos",0);
	char sts[25];
	ft::getnowstr(sts);
	Json::Value js_pos;
	std::ostringstream sout_pos;
	try {
		js_pos["ts"] = sts;
		js_pos["pan"] = pan;
		js_pos["tilt"] = tilt;
		sout_pos << js_pos;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_PTUPOS);
			auto msg_spos = mqtt::make_message(TOPIC_INPUT_PTUPOS, sout_pos.str());//spos.c_str());
			msg_spos->set_qos(iqos);
			msg_spos->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish PTU pos: {} {} {}", sts, pan, tilt);
			mqtt::token_ptr conntok = cli.publish(msg_spos, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishPtuPos: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishPtuPos",0);
}

void TxtMqttFactoryClient::publishCam(const std::string sdata, long timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishCam timeout:{}",timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishCam",0);
	char sts[25];
	ft::getnowstr(sts);
	Json::Value js_cam;
	std::ostringstream sout_cam;
	try {
		js_cam["ts"] = sts;
		js_cam["data"] = sdata;
		sout_cam << js_cam;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_CAM);
			auto msg_im = mqtt::make_message(TOPIC_INPUT_CAM, sout_cam.str());//sim.c_str());
			msg_im->set_qos(0);
			msg_im->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish Cam: {}", sts);
			mqtt::token_ptr conntok = cli.publish(msg_im, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishCam: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishCam",0);
}

void TxtMqttFactoryClient::publishBme680(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
	     float pressure, float raw_temperature, float raw_humidity, float gas, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishBme680 {} {} {} {} {} {} {} {} {} timeout:{}",
			timestamp,iaq,iaq_accuracy,temperature,humidity,
			pressure,raw_temperature,raw_humidity,gas,timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishBme680",0);
	char sts[25];
	ft::gettimestampstr(timestamp, sts);
	Json::Value js_bme680;
	std::ostringstream sout_bme680;
	try {
		js_bme680["ts"] = sts;
		js_bme680["t"] = fromString<double>(ft::ftos(temperature, 1));
		js_bme680["rt"] = fromString<double>(ft::ftos(raw_temperature, 2));
		js_bme680["h"] = fromString<double>(ft::ftos(humidity, 1));
		js_bme680["rh"] = fromString<double>(ft::ftos(raw_humidity, 2));
		js_bme680["p"] = fromString<double>(ft::ftos(pressure/100, 1));
		js_bme680["iaq"] = fromString<double>(ft::ftos(iaq, 0));
		js_bme680["aq"] = iaq_accuracy;
		js_bme680["gr"] = fromString<double>(ft::ftos(gas, 0));
		sout_bme680 << js_bme680;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_BME680);
			auto msg_bme680 = mqtt::make_message(TOPIC_INPUT_BME680, sout_bme680.str());//sbme680);
			msg_bme680->set_qos(iqos);
			msg_bme680->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish BME680: {} {} {} {} {} {} {} {} {}",
					sts, ft::ftos(temperature, 1), ft::ftos(raw_temperature, 2),
					ft::ftos(humidity, 1), ft::ftos(raw_humidity, 2),
					ft::ftos(pressure/100, 1), ft::ftos(iaq, 0),
					iaq_accuracy, ft::ftos(gas, 0));
			mqtt::token_ptr conntok = cli.publish(msg_bme680, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishBme680: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishBme680",0);
}

void TxtMqttFactoryClient::publishAlert(bool st, const std::string id, const std::string sdata, int code, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishAlert {} {} timeout:{}",
			id,code,timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishAlert",0);
	Json::Value js_alert;
	std::ostringstream sout_alert;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_alert["ts"] = sts;
		js_alert["id"] = id;
		if (st) {
			js_alert["data"] = sdata;
		} else {
			js_alert["data"] = fromString<double>(sdata);
		}
		js_alert["code"] = code;
		sout_alert << js_alert;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_ALERT);
			auto msg_alert = mqtt::make_message(TOPIC_INPUT_ALERT, sout_alert.str());
			msg_alert->set_qos(iqos);
			msg_alert->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish Alert: {} {} {}", sts, id, code);
			mqtt::token_ptr conntok = cli.publish(msg_alert, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishAlert: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishAlert",0);
}

std::string getMAC(const char* sdev)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getMAC {}",sdev);
	char MAC_str[13];
    #define HWADDR_len 6
    int sock, i;
    struct ifreq ifr;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, sdev);
    ioctl(sock, SIOCGIFHWADDR, &ifr);
    for (i=0; i<HWADDR_len; i++)
        sprintf(&MAC_str[i*2],"%02X",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
    MAC_str[12]='\0';
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "MAC_str {}",MAC_str);
    return std::string(MAC_str);
}

void TxtMqttFactoryClient::publishBroadcast(double timestamp_s, const std::string sw, const std::string ver, const std::string message, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishBroadcast {} {} {} timeout:{}",
			timestamp_s,ver,message, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishBroadcast",0);
	Json::Value js_broadcast;
	std::ostringstream sout_broadcast;
	char sts[25];
	ft::gettimestampstr(timestamp_s*1000000000., sts);
	try {
		js_broadcast["ts"] = sts;
	    // read device MAC id
	    std::string macWlan0 = getMAC("wlan0"); //MAC
		js_broadcast["hardwareId"] = macWlan0;
		js_broadcast["hardwareModel"] = "TXT";
		js_broadcast["softwareName"] = sw;
		js_broadcast["softwareVersion"] = ver;
		js_broadcast["message"] = message;
		sout_broadcast << js_broadcast;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "str: {}",sout_broadcast.str());
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_BROADCAST);
			auto msg_broadcast = mqtt::make_message(TOPIC_INPUT_BROADCAST, sout_broadcast.str());
			msg_broadcast->set_qos(iqos);
			msg_broadcast->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish Broadcast: {}", sts);
			mqtt::token_ptr conntok = cli.publish(msg_broadcast, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishBroascast: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishBroadcast",0);
}

void TxtMqttFactoryClient::pusblishWorkingModeChanged(TxtVgrWorkingModes_t wm, long timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "pusblishWorkingModeChanged wm:{}", (int)wm);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock pusblishWorkingModeChanged",0);
	Json::Value js_wmState;
	std::ostringstream sout_wmState;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_wmState["ts"] = sts;
		js_wmState["workingmode"] = (int)wm;
		sout_wmState << js_wmState;
		try {
	
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_WM);
			auto msg_stateStation = mqtt::make_message(TOPIC_INPUT_WM, sout_wmState.str());
			msg_stateStation->set_qos(iqos);
			msg_stateStation->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pusblishWorkingModeChanged {} wm:{}", sts, (int)wm);
			mqtt::token_ptr conntok = cli.publish(msg_stateStation, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "pusblishWorkingModeChanged: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock pusblishWorkingModeChanged",0);
}

void TxtMqttFactoryClient::publishStateStation(const std::string station, TxtLEDSCode_t code, const std::string desc, long timeout, int active, const std::string target)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishStateStation station:{} code:{} desc:{} timeout:{} active:{} target:{}", station, (int)code, desc, timeout, active, target);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishStateStation",0);
	Json::Value js_stateStation;
	std::ostringstream sout_stateStation;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_stateStation["ts"] = sts;
		js_stateStation["station"] = station;
		js_stateStation["code"] = (int)code;
		js_stateStation["description"] = desc;
		js_stateStation["active"] = active;
		if (station=="vgr")
		{
			js_stateStation["target"] = target;
		}
		sout_stateStation << js_stateStation;
		try {
			//HBW, VGR, MPO, SLD; DSI, DSO
			if ((station=="hbw")||(station=="vgr")||(station=="mpo")||(station=="sld")||
					(station=="dsi")||(station=="dso"))
			{
				mqtt::string topic = TOPIC_INPUT_STATE_ + station;
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", topic);
				auto msg_stateStation = mqtt::make_message(topic, sout_stateStation.str());
				msg_stateStation->set_qos(iqos);
				msg_stateStation->set_retained(bretained);
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish state station: {} {} {} {} {} {}", sts, station, (int)code, desc, active, target);
				mqtt::token_ptr conntok = cli.publish(msg_stateStation, nullptr, aListPub);
				bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
				if (!r) exit(1);
#endif
			}
		} catch (const mqtt::exception& exc) {
			std::cout << "publishStateStation: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishStateStation",0);
}

void TxtMqttFactoryClient::publishVGROrderQueue(std::queue<TxtOrderState> q, long timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishVGROrderQueue timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishVGROrderQueue",0);
	Json::Value js_queue;
	std::ostringstream sout_queue;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_queue["ts"] = sts;
		Json::Value jsonArray;
		TxtOrderState queue_item;
		int pos = 1;
		while(!q.empty()) {
			Json::Value item;
			queue_item = q.front();
			q.pop();
			item["pos"] = pos++;
			item["id"] = queue_item.uid;
			item["type"] = toString(queue_item.wp.type);
			jsonArray.append(item);
		}
		js_queue["orders"] = jsonArray;
		sout_queue << js_queue;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_QUEUE_ORDERS);
			auto msg_queue = mqtt::make_message(TOPIC_INPUT_QUEUE_ORDERS, sout_queue.str());
			msg_queue->set_qos(iqos);
			msg_queue->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish queue: {} {}", sts, jsonArray.size());
			mqtt::token_ptr conntok = cli.publish(msg_queue, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishVGROrderQueue: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishVGROrderQueue", 0);
}

void TxtMqttFactoryClient::publishVGRPickupQueue(std::queue<TxtPickupState> q, long timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishVGRPickupQueue timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishVGRPickupQueue",0);
	Json::Value js_queue;
	std::ostringstream sout_queue;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_queue["ts"] = sts;
		Json::Value jsonArray;
		TxtPickupState queue_item;
		int pos = 1;
		while(!q.empty()) {
			Json::Value item;
			queue_item = q.front();
			q.pop();
			item["pos"] = pos++;
			item["id"] = !queue_item.uid.empty() ? queue_item.uid : "";
			item["wp_id"] = queue_item.wp.tag_uid;
			jsonArray.append(item); 
		}
		js_queue["pickups"] = jsonArray;
		sout_queue << js_queue;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_QUEUE_PICKUPS);
			auto msg_queue = mqtt::make_message(TOPIC_INPUT_QUEUE_PICKUPS, sout_queue.str());
			msg_queue->set_qos(iqos);
			msg_queue->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish queue: {} {}", sts, jsonArray.size());
			mqtt::token_ptr conntok = cli.publish(msg_queue, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishVGRPickupQueue: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishVGRPickupQueue", 0);
}

void TxtMqttFactoryClient::publishStock(Stock_map_t map_wps, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishStock timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishStock",0);
	Json::Value js_stock;
	std::ostringstream sout_stock;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_stock["ts"] = sts;
		Json::Value jsonArray;
		if (map_wps.size() > 0)
		{
		    for (Stock_map_t::const_iterator it=map_wps.begin(); it!=map_wps.end(); ++it)
		    {
		    	std::string loc(it->first);
		    	TxtWorkpiece* wp = it->second;
		    	Json::Value js_wpRoot;
				js_wpRoot["location"] = loc;
		    	if (wp) {
			    	Json::Value js_wp;
			    	js_wp["id"] = wp->tag_uid;
			    	js_wp["type"] = toString(wp->type);
			    	js_wp["state"] = toString(wp->state);
			    	js_wpRoot["workpiece"] = js_wp;
		    	} else {
			    	js_wpRoot["workpiece"] = Json::Value::null;
		    	}
				jsonArray.append(js_wpRoot);
		    }
		} else {
			jsonArray.append(Json::Value::null);
			jsonArray.clear();
		}
		js_stock["stockItems"] = jsonArray;
		sout_stock << js_stock;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_STOCK);
			auto msg_stock = mqtt::make_message(TOPIC_INPUT_STOCK, sout_stock.str());
			//printf("%s", sout_stock.str().c_str());
			msg_stock->set_qos(iqos);
			msg_stock->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish stock: {} {}", sts, jsonArray.size());
			mqtt::token_ptr conntok = cli.publish(msg_stock, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishStock: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishStock",0);
}

void TxtMqttFactoryClient::publishStateOrder(TxtOrderState ord_state, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishStateOrder timeout:{}", timeout);
	//local message
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishStateOrder",0);
	Json::Value js_order;
	std::ostringstream sout_order;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_order["ts"] = sts;
		Json::Value js_state;
		js_state["uid"] = ord_state.uid;
		js_state["status"] = toString(ord_state.state); //only local

		TxtOrderStateRemote_t ord_state_remote = WAITING_FOR_ORDER_REMOTE;
		switch(ord_state.state)
		{
		case WAITING_FOR_ORDER: //!
		case ORDER_ACKNOWLEDGED: //!
			ord_state_remote = WAITING_FOR_ORDER_REMOTE;
			break;
		case FETCHING_FROM_STORAGE:
		case FETCHED_FROM_STORAGE:
			ord_state_remote = ORDERED_REMOTE;
			break;
		case TRANSPORT_TO_PROCESS_STATION:
		case IN_PROCESS:
		case ORDER_PROCESSED: //!
		case PROCESS_FAULT:
		case WAITING_FOR_QA:
		case TRANSPORT_WP_TO_DSO:
			ord_state_remote = IN_PROCESS_REMOTE;
			break;
		case SHIPPED: //!
			ord_state_remote = SHIPPED_REMOTE;
			break;
		case QA_FAULT:
		case ORDER_FAULT:
			ord_state_remote = IN_PROCESS_REMOTE;
			break;
		default:
			break;
		}
		js_order["state"] = toString(ord_state_remote); //remote
		js_order["type"] = toString(ord_state.wp.type); //remote

		Json::Value js_wp;
		if (ord_state.state != WAITING_FOR_ORDER) {
			js_wp["wp_id"] = ord_state.wp.tag_uid;
			js_wp["wp_type"] = toString(ord_state.wp.type);
			js_wp["wp_state"] = toString(ord_state.wp.state);
		}
		js_state["workpiece"] = js_wp;
		js_order["request"] = js_state;
		sout_order << js_order;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_STATE_ORDER);
			auto msg_order = mqtt::make_message(TOPIC_INPUT_STATE_ORDER, sout_order.str());
			msg_order->set_qos(iqos);
			msg_order->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish state order: {} {} {}", sts, toString(ord_state.state), toString(ord_state.wp.type));
			mqtt::token_ptr conntok = cli.publish(msg_order, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishStateOrder: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishStateOrder",0);
}

void TxtMqttFactoryClient::publishStatePickup(TxtPickupState pickup_state, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishOrder timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishOrder",0);
	Json::Value js_pickup;
	std::ostringstream sout_pickup;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_pickup["ts"] = sts;
		Json::Value js_state;
		js_state["uid"] = pickup_state.uid;
		js_state["status"] = toString(pickup_state.state);
		Json::Value js_wp;
		if(pickup_state.state != WAITING_FOR_PICKUP) {
			js_wp["wp_id"] = pickup_state.wp.tag_uid;
			js_wp["wp_type"] = toString(pickup_state.wp.type);
			js_wp["wp_state"] = toString(pickup_state.wp.state);
		}
		js_state["workpiece"] = js_wp;
		js_pickup["request"] = js_state;
		sout_pickup << js_pickup;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_STATE_PICKUP);
			auto msg_pickup = mqtt::make_message(TOPIC_INPUT_STATE_PICKUP, sout_pickup.str());
			msg_pickup->set_qos(iqos);
			msg_pickup->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish state pickup: {} {} {} {}", sts, pickup_state.wp.tag_uid, toString(pickup_state.wp.state), toString(pickup_state.wp.type));
			mqtt::token_ptr conntok = cli.publish(msg_pickup, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishStatePickup: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishPickup",0);
}

void TxtMqttFactoryClient::publishStateStore(TxtStoreState store_state, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishStore timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishpickup",0);
	Json::Value js_store;
	std::ostringstream sout_store;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_store["ts"] = sts;
		Json::Value js_state;
		js_state["uid"] = store_state.uid;
		js_state["status"] = toString(store_state.state);
		Json::Value js_wp;
		if(store_state.state != WAITING_FOR_STORE) {
			js_wp["wp_id"] = store_state.wp.tag_uid;
			js_wp["wp_type"] = toString(store_state.wp.type);
			js_wp["wp_state"] = toString(store_state.wp.state);
		}
		js_state["workpiece"] = js_wp;
		js_store["request"] = js_state;
		sout_store << js_store;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_INPUT_STATE_STORE);
			auto msg_store = mqtt::make_message(TOPIC_INPUT_STATE_STORE, sout_store.str());
			msg_store->set_qos(iqos);
			msg_store->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish state store: {}", sts);
			mqtt::token_ptr conntok = cli.publish(msg_store, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishStateStore: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishStore",0);
}

void TxtMqttFactoryClient::publishNfcDS(TxtWorkpiece* wp, HistoryCode_map_t map_hist, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishNfcDS timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishNfcDS",0);
	Json::Value js_nfcDS;
	std::ostringstream sout_nfcDS;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_nfcDS["ts"] = sts;

    	if (wp) {
	    	Json::Value js_wp;
	    	js_wp["id"] = wp->tag_uid;
	    	js_wp["type"] = toString(wp->type);
	    	js_wp["state"] = toString(wp->state);
			js_nfcDS["workpiece"] = js_wp;
    	} else {
    		js_nfcDS["workpiece"] = Json::Value::null;
    	}

		Json::Value jsonArray;
	    for (HistoryCode_map_t::const_iterator it=map_hist.begin(); it!=map_hist.end(); ++it)
	    {
	    	TxtHistoryCode_t hcode = it->first;
	    	int64_t hts = it->second;
			Json::Value js_elem;
			char hsts[25];
			ft::gettimestampstr(hts, hsts);
	    	js_elem["ts"] = hsts;
	    	js_elem["code"] = hcode;
		    jsonArray.append(js_elem);
	    }
		js_nfcDS["history"] = jsonArray;

		sout_nfcDS << js_nfcDS;
		try {
			std::cout << "  nfc_remote: " <<  nfc_remote << std::endl;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", nfc_remote?TOPIC_INPUT_NFC_DS:TOPIC_LOCAL_NFC_DS_CMD);
			auto msg_nfcDS = mqtt::make_message(nfc_remote?TOPIC_INPUT_NFC_DS:TOPIC_LOCAL_NFC_DS_CMD, sout_nfcDS.str());
			msg_nfcDS->set_qos(iqos);
			msg_nfcDS->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish NFC DS: {}", sts);
			mqtt::token_ptr conntok = cli.publish(msg_nfcDS, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishnfcDS: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishNfcDS",0);
}

void TxtMqttFactoryClient::publishStationBroadcast(const std::string station, double timestamp_s, const std::string sw, const std::string ver, const std::string message, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishStationBroadcast {} {} {} {} timeout:{}",
			station, timestamp_s, ver, message, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishStationBroadcast",0);
	Json::Value js_broadcast;
	std::ostringstream sout_broadcast;
	char sts[25];
	ft::gettimestampstr(timestamp_s*1000000000., sts);
	try {
		js_broadcast["ts"] = sts;
		js_broadcast["station"] = station;
	    // read device MAC id
	    std::string macWlan0 = getMAC("wlan0"); //MAC
		js_broadcast["hardwareId"] = macWlan0;
		js_broadcast["softwareName"] = sw;
		js_broadcast["softwareVersion"] = ver;
		js_broadcast["message"] = message;
		sout_broadcast << js_broadcast;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "str: {}",sout_broadcast.str());
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_BROADCAST);
			auto msg_broadcast = mqtt::make_message(TOPIC_LOCAL_BROADCAST, sout_broadcast.str());
			msg_broadcast->set_qos(iqos);
			msg_broadcast->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish Broadcast: {}", sts);
			mqtt::token_ptr conntok = cli.publish(msg_broadcast, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishStationBroadcast: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishStationBroadcast",0);
}

void TxtMqttFactoryClient::publishSSC_Joy(TxtJoysticksData jd, long timeout) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishSSC_Joy X1 Y1 b1 X2 Y2 b2: {} {} {} {} {} {} timeout:{}", jd.aX1, jd.aY1, jd.b1, jd.aX2, jd.aY2, jd.b2, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishSSC_Joy",0);
	char sts[25];
	ft::getnowstr(sts);
	Json::Value js_pos;
	std::ostringstream sout_pos;
	try {
		js_pos["ts"] = sts;
		js_pos["aX1"] = jd.aX1;
		js_pos["aY1"] = jd.aY1;
		js_pos["b1"] = jd.b1;
		js_pos["aX2"] = jd.aX2;
		js_pos["aY2"] = jd.aY2;
		js_pos["b2"] = jd.b2;
		sout_pos << js_pos;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_SSC_JOY);
			auto msg_spos = mqtt::make_message(TOPIC_LOCAL_SSC_JOY, sout_pos.str());
			msg_spos->set_qos(iqos);
			msg_spos->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish joysticks: {} {} {} {} {} {}", sts, jd.aX1, jd.aY1, jd.b1, jd.aX2, jd.aY2, jd.b2);
			mqtt::token_ptr conntok = cli.publish(msg_spos, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishSSC_Joy: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishSSC_Joy",0);
}

void TxtMqttFactoryClient::publishMPO_Ack(TxtMpoAckCode_t code, TxtWorkflowState* request, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishMPO_Ack timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishMPO_Ack",0);
	Json::Value js_ack;
	std::ostringstream sout_ack;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_ack["ts"] = sts;
		js_ack["code"] = (int)code;

		if (request)
		{
			Json::Value js_state;
			js_state["uid"] = request->uid;
			Json::Value js_wp;
	    	js_wp["wp_id"] = request->wp.tag_uid;
	    	js_wp["wp_type"] = toString(request->wp.type);
	    	js_wp["wp_state"] = toString(request->wp.state);
	    	js_state["workpiece"] = js_wp;
			js_ack["request"] = js_state;
		} else {
	    	js_ack["request"] = Json::Value::null;
		}
		sout_ack << js_ack;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_MPO_ACK);
			auto msg_ack = mqtt::make_message(TOPIC_LOCAL_MPO_ACK, sout_ack.str());
			msg_ack->set_qos(iqos);
			msg_ack->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {} {}", sts, (int)code);
			mqtt::token_ptr conntok = cli.publish(msg_ack, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishMPO_Ack: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishMPO_Ack",0);
}

void TxtMqttFactoryClient::publishMPO_Fault(TxtFaultCodes_t code, TxtWorkflowState* request, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishMPO_Fault timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishMPO_Fault",0);
	Json::Value js_fault;
	std::ostringstream sout_fault;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_fault["ts"] = sts;
		js_fault["code"] = (int)code;
		Json::Value js_request;
		if (request != nullptr) {
			js_request["uid"] = request->uid;
			Json::Value js_wp;
			js_wp["wp_id"] = request->wp.tag_uid;
			js_wp["wp_state"] = toString(request->wp.state);
			js_wp["wp_type"] = toString(request->wp.type);
			js_request["workpiece"] = js_wp;
		}
		js_fault["request"] = js_request;

		sout_fault << js_fault;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_MPO_FAULT);
			auto msg_fault = mqtt::make_message(TOPIC_LOCAL_MPO_FAULT, sout_fault.str());
			msg_fault->set_qos(iqos);
			msg_fault->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {}", sts);
			mqtt::token_ptr conntok = cli.publish(msg_fault, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishSLD_Fault: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishMPO_Fault",0);
}

void TxtMqttFactoryClient::publishVGR_Do(TxtVgrDoCode_t code, TxtWorkflowState* state, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishVGR_Do code:{} timeout:{}", (int)code, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishVGR_Do",0);
	Json::Value js_req;
	std::ostringstream sout_ack;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_req["ts"] = sts;
		js_req["code"] = (int)code;

		if (state != nullptr)
		{
			Json::Value js_state;
			js_state["uid"] = state->uid;
			Json::Value js_wp;
	    	js_wp["wp_id"] = state->wp.tag_uid;
	    	js_wp["wp_type"] = toString(state->wp.type);
	    	js_wp["wp_state"] = toString(state->wp.state);
	    	js_state["workpiece"] = js_wp;
			js_req["request"] = js_state;
		} else {
	    	js_req["request"] = Json::Value::null;
		}

		sout_ack << js_req;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_VGR_DO);
			auto msg_ack = mqtt::make_message(TOPIC_LOCAL_VGR_DO, sout_ack.str());
			msg_ack->set_qos(iqos);
			msg_ack->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {}", (int)code);
			mqtt::token_ptr conntok = cli.publish(msg_ack, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishVGR_Do: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishVGR_Do",0);
}

void TxtMqttFactoryClient::publishVGR_Order(TxtWPType_t t, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishVGR_Order  timeout:{}", toString(t), timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishVGR_Order",0);
	Json::Value js_ack;
	std::ostringstream sout_ack;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_ack["ts"] = sts;
		js_ack["type"] = toString(t);

		sout_ack << js_ack;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_OUTPUT_ORDER);
			auto msg_ack = mqtt::make_message(TOPIC_OUTPUT_ORDER, sout_ack.str());
			msg_ack->set_qos(iqos);
			msg_ack->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {}", toString(t));
			mqtt::token_ptr conntok = cli.publish(msg_ack, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishVGR_Order: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishVGR_Order",0);
}

void TxtMqttFactoryClient::publishHBW_Ack(TxtHbwAckCode_t code, TxtWorkpiece* wp, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishHBW_Ack code:{} timeout:{}", (int)code, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishHBW_Ack",0);
	Json::Value js_ack;
	std::ostringstream sout_ack;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_ack["ts"] = sts;
		js_ack["code"] = (int)code;

		if (wp) {
			Json::Value js_wp;
			js_wp["id"] = wp->tag_uid;
			js_wp["type"] = toString(wp->type);
			js_wp["state"] = toString(wp->state);
			js_ack["workpiece"] = js_wp;
		} else {
			js_ack["workpiece"] = Json::Value::null;
		}

		sout_ack << js_ack;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_HBW_ACK);
			auto msg_ack = mqtt::make_message(TOPIC_LOCAL_HBW_ACK, sout_ack.str());
			msg_ack->set_qos(iqos);
			msg_ack->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {} {}", sts, (int)code);
			mqtt::token_ptr conntok = cli.publish(msg_ack, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishHBW_Ack: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishHBW_Ack",0);
}

void TxtMqttFactoryClient::publishHBW_Fault(TxtFaultCodes_t code, TxtWorkpiece wp, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishHBW_Fault code:{} timeout:{}", (int)code, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishHBW_Fault",0);
	Json::Value js_ack;
	std::ostringstream sout_ack;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_ack["ts"] = sts;
		js_ack["code"] = (int)code;

		if (wp.tag_uid != "" && wp.state != WP_STATE_NONE && wp.type != WP_TYPE_NONE) {
			Json::Value js_wp;
			js_wp["id"] = wp.tag_uid;
			js_wp["type"] = toString(wp.type);
			js_wp["state"] = toString(wp.state);
			js_ack["workpiece"] = js_wp;
		} else {
			js_ack["workpiece"] = Json::Value::null;
		}

		sout_ack << js_ack;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_HBW_FAULT);
			auto msg_ack = mqtt::make_message(TOPIC_LOCAL_HBW_FAULT, sout_ack.str());
			msg_ack->set_qos(iqos);
			msg_ack->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {} {}", sts, (int)code);
			mqtt::token_ptr conntok = cli.publish(msg_ack, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishHBW_Fault: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishHBW_Ack",0);
}

void TxtMqttFactoryClient::publishSLD_Ack(TxtSldAckCode_t code, TxtWorkflowState* request, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishSLD_Ack code:{} request {} timeout:{}", (int)code, request->uid, timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishSLD_Ack",0);
	Json::Value js_ack;
	std::ostringstream sout_ack;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_ack["ts"] = sts;
		js_ack["code"] = (int)code;
		
		if (request)
		{
			Json::Value js_state;
			js_state["uid"] = request->uid;
			Json::Value js_wp;
	    	js_wp["wp_id"] = request->wp.tag_uid;
	    	js_wp["wp_type"] = toString(request->wp.type);
	    	js_wp["wp_state"] = toString(request->wp.state);
	    	js_state["workpiece"] = js_wp;
			js_ack["request"] = js_state;
		} else {
	    	js_ack["request"] = Json::Value::null;
		}
		
		sout_ack << js_ack;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_SLD_ACK);
			auto msg_ack = mqtt::make_message(TOPIC_LOCAL_SLD_ACK, sout_ack.str());
			msg_ack->set_qos(iqos);
			msg_ack->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {} {} {}", sts, (int)code, request->uid);
			mqtt::token_ptr conntok = cli.publish(msg_ack, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishSLD_Ack: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishSLD_Ack",0);
}


void TxtMqttFactoryClient::publishSLD_Fault(TxtFaultCodes_t code, TxtWorkflowState* request, long timeout)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "publishSLD_Fault timeout:{}", timeout);
	pthread_mutex_lock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock publishSLD_Fault",0);
	Json::Value js_fault;
	std::ostringstream sout_fault;
	char sts[25];
	ft::getnowstr(sts);
	try {
		js_fault["ts"] = sts;
		js_fault["code"] = (int)code;
		Json::Value js_request;
		if (request != nullptr) {
			js_request["uid"] = request->uid;
			Json::Value js_wp;
			js_wp["wp_id"] = request->wp.tag_uid;
			js_wp["wp_state"] = toString(request->wp.state);
			js_wp["wp_type"] = toString(request->wp.type);
			js_request["workpiece"] = js_wp;
		}
		js_fault["request"] = js_request;

		sout_fault << js_fault;
		try {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "topic: {}", TOPIC_LOCAL_SLD_FAULT);
			auto msg_fault = mqtt::make_message(TOPIC_LOCAL_SLD_FAULT, sout_fault.str());
			msg_fault->set_qos(iqos);
			msg_fault->set_retained(bretained);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "publish: {}", sts);
			mqtt::token_ptr conntok = cli.publish(msg_fault, nullptr, aListPub);
			bool r = conntok->wait_for(timeout);
#ifdef FORCE_EXIT_ON_TIMEOUT
			if (!r) exit(1);
#endif
		} catch (const mqtt::exception& exc) {
			std::cout << "publishSLD_Fault: " << exc.what() << " "
					<< getMQTTReasonCodeString(exc.get_reason_code()) << std::endl;
		}
	} catch (const Json::RuntimeError& exc) {
		std::cout << "Error: " << exc.what() << std::endl;
	}
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock publishSLD_Ack",0);
}


} /* namespace ft */
