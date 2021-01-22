#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "Utils.h"
#include "Observer.h"

#include "TxtAlert.h"
#include "TxtBME680.h"
#include "TxtCamera.h"
#include "TxtMotionDetection.h"
#include "TxtFactoryTypes.h"
#include "TxtMqttFactoryClient.h"
#include "TxtSound.h"
#include "TxtNfcDevice.h"

#include <stdio.h>          // for printf()
#include <string.h>         // for memset()
#include <unistd.h>         // for sleep()
#include <cmath>			// for pow()
#include <chrono>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

bool first_message_arrived = false;
bool first_message_subscribe = false;
bool first_message_cam_on_arrived = false;

// Version info
#define VERSION_HEX ((0<<16)|(9<<8)|(5<<0))
char TxtAppVer[32];

unsigned int DebugFlags;
FILE *DebugFile;

FISH_X1_TRANSFER* pTArea = NULL;

ft::TxtMqttFactoryClient* pcli = NULL;
ft::TxtBME680* pBme680 = NULL; // extern in TxtBME680.cpp
ft::TxtCamera* pCam = NULL;
ft::TxtNfcDevice* pNfc = NULL;

//uint16_t u16CountState1000ms = 0;
int16_t ldr_last = 0;

double delta = 0.1;

double force_max_rate = -1.0;
double period_ldr = 60.0; //Default: 1 Min
double period_bme680 = 60.0; //Default: 1 Min
int64_t timestamp_bme680 = 0; //ns
double timestamp_ldr = 0.; //s

std::chrono::system_clock::time_point tsLastDetectedTemp;
std::chrono::system_clock::time_point tsLastDetectedHum;
#ifdef DEBUG
std::chrono::system_clock::time_point startLast;
#endif

#define TIMEOUT_CONNECTION_MS 60000 //60 s


class TxtCameraObserver : public ft::Observer {
public:
	TxtCameraObserver(ft::TxtCamera* s)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtCameraObserver",0);
		_subject = s;
		_subject->Attach(this);
	}
	virtual ~TxtCameraObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtCameraObserver",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtCameraObserver Update 1",0);
#ifdef DEBUG
			auto start = std::chrono::system_clock::now();
#endif

			std::string sdata = _subject->getDataString();
			if (!sdata.empty()) {
#ifdef CAM_TEST
					spdlog::get("console")->info("CAM 3: --- publish");
#endif
				assert(pcli);
				long timeout_ms = TIMEOUT_CONNECTION_MS;//TODO _subject->getPeriod(); //67 max 15fps
				//if (mleds==1) {
					//setLED(4, 512);
					//TODO send MQTT red LED
				//}
				pcli->publishCam(sdata, timeout_ms);
			}

#ifdef DEBUG
			auto dur = start-startLast;
			auto secs = std::chrono::duration_cast< std::chrono::duration<float> >(dur);
			double period_ms = _subject->getPeriod();
			double wait_ms = period_ms - secs.count()/1000.0;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "fps:{} diff_s:{} period_ms:{} wait_ms:{}",
					1./secs.count(), secs.count(), period_ms, wait_ms);
			startLast = start;
#endif
			//if (mleds==1) {
				//setLED(4, 0);
				//TODO send MQTT red LED
			//}
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtCameraObserver Update 2",0);
		}
	}
private:
	ft::TxtCamera *_subject;
};

class TxtMotionDetectionObserver : public ft::Observer {
public:
	TxtMotionDetectionObserver(ft::TxtMotionDetection* s)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtMotionDetectionObserver",0);
		_subject = s;
		_subject->Attach(this);
	}
	virtual ~TxtMotionDetectionObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtMotionDetectionObserver",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtMotionDetectionObserver Update 1",0);
			std::string sdata = _subject->getDataString();
			if (!sdata.empty()) {
				assert(pcli);
				long timeout_ms = TIMEOUT_CONNECTION_MS;//TODO 1000
				pcli->publishAlert(true, "cam", sdata, 100, timeout_ms);
				std::cout << "Alert: cam" << std::endl;
			}
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtMotionDetectionObserver Update 2",0);
		}
	}
private:
	ft::TxtMotionDetection *_subject;
};


class TxtBme680Observer : public ft::Observer {
public:
	TxtBme680Observer(ft::TxtBME680* s)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtBme680Observer",0);
		_subject = s;
		_subject->Attach(this);
	}
	virtual ~TxtBme680Observer() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtBme680Observer",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtBme680Observer Update 1",0);
			//BME680
			double diff = ((_subject->_timestamp - timestamp_bme680) / 1000000000.) + delta;
			spdlog::get("console")->info("BME680 diff:{}", diff);
			if ((timestamp_bme680 == 0) || (diff >= period_bme680)) {
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "timestamp_bme680:{} period_bme680:{}", timestamp_bme680/1000000000., period_bme680);
				timestamp_bme680 = _subject->_timestamp;
				long timeout_ms = TIMEOUT_CONNECTION_MS;//TODO1000L*period_bme680;
				assert(pcli);
				pcli->publishBme680(
					_subject->_timestamp,
					_subject->_iaq,
					_subject->_iaq_accuracy,
					_subject->_temperature,
					_subject->_humidity,
					_subject->_pressure,
					_subject->_raw_temperature,
					_subject->_raw_humidity,
					_subject->_gas,
					timeout_ms);
			}
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtBme680Observer Update 2",0);
		}
	}
private:
	ft::TxtBME680 *_subject;
};

class TxtAlertBme680Observer : public ft::Observer {
public:
	TxtAlertBme680Observer(ft::TxtBME680* s)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtBme680Observer",0);
		_subject = s;
		_subject->Attach(this);
	}
	virtual ~TxtAlertBme680Observer() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtBme680Observer",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtAlertBme680Observer Update",0);
			//_subject->_timestamp
			long int timeout_ms = TIMEOUT_CONNECTION_MS;//TODO 1000;
			if (_subject->_temperature < 4.0) {
				auto tsDetected = std::chrono::system_clock::now();
				auto dur = tsDetected-tsLastDetectedTemp;
				auto secs = std::chrono::duration_cast< std::chrono::duration<float> >(dur);
				SPDLOG_LOGGER_TRACE(spdlog::get("console"), "elapsed_seconds {}", secs.count());
				if (secs.count() > TIMEWAIT_S_MAX) {
					tsLastDetectedTemp = tsDetected;
					assert(pcli);
					pcli->publishAlert(false, "bme680/t", ft::ftos(_subject->_temperature,1), 200, timeout_ms);
					std::cout << "Alert: bme680/t" << std::endl;
				}
			}
			if (_subject->_humidity > 80.) {
				auto tsDetected = std::chrono::system_clock::now();
				auto dur = tsDetected-tsLastDetectedHum;
				auto secs = std::chrono::duration_cast< std::chrono::duration<float> >(dur);
				SPDLOG_LOGGER_TRACE(spdlog::get("console"), "elapsed_seconds {}", secs.count());
				if (secs.count() > TIMEWAIT_S_MAX) {
					tsLastDetectedHum = tsDetected;
					assert(pcli);
					pcli->publishAlert(false, "bme680/h", ft::ftos(_subject->_humidity,1), 300, timeout_ms);
					std::cout << "Alert: bme680/h" << std::endl;
				}
			}
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtAlertBme680Observer Update 2",0);
		}
	}
private:
	ft::TxtBME680 *_subject;
};


class callback : public virtual mqtt::callback
{
	ft::TxtMqttFactoryClient& cli_;

	void connected(const std::string& cause) override {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "connected: {}", cause);
		long timeout_ms = TIMEOUT_CONNECTION_MS;
		std::cout << "Subscribe MQTTClient" << std::endl;
		pcli->start_consume(timeout_ms);
		first_message_subscribe = true;
	}

	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "connection_lost: {}", cause);
	}

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override {
		assert(msg);
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "message_arrived  message:{} payload:{}", msg->get_topic(), msg->to_string());
		//BUGFIX: msg->get_topic() is empty
		//FIX paho.mqtt.cpp: https://github.com/eclipse/paho.mqtt.c/issues/440#issuecomment-380161713

		first_message_arrived = true;

		if (msg->get_topic() == TOPIC_CONFIG_LINK) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "DETECTED link:{}", msg->get_topic());
			std::stringstream ssin(msg->to_string());
			Json::Value root;
			try {
				ssin >> root;
				std::string smessage = root["message"].asString();
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  message:{}", smessage);
				int code = -1;
				if (root.isMember("code")) {
					code = root["code"].asInt();
				}
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  code:{}", code);
			} catch (const Json::RuntimeError& exc) {
				std::cout << "Error: " << exc.what() << std::endl;
			}
		} else if (msg->get_topic() == TOPIC_LOCAL_BROADCAST) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "DETECTED local broadcast:{}", msg->get_topic());
			std::stringstream ssin(msg->to_string());
			Json::Value root;
			try {
				ssin >> root;
				std::string sts = root["ts"].asString();
				std::string station = root["station"].asString();
				std::string hardwareId = root["hardwareId"].asString();
				std::string softwareName = root["softwareName"].asString();
				std::string softwareVersion = root["softwareVersion"].asString();
				std::string message = root["message"].asString();
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  station:{} ts:{}", station, sts);
				//check time sync
				if (!ft::trycheckTimestampTTL(sts))
				{
					std::cout << "Please sync time!" << station << ", " << sts << std::endl;
					spdlog::get("file_logger")->error("Please sync time! {} ({})",station,sts);
					ft::TxtSound::play(pTArea,2);
					exit(1);
				}
				//check SW version
				if (TxtAppVer != softwareVersion)
				{
					std::cout << "Wrong SW Version!" << station << " " << TxtAppVer << "!=" << softwareVersion << std::endl;
					spdlog::get("file_logger")->error("Wrong SW Version! {} {}!={}",station,TxtAppVer,softwareVersion);
					ft::TxtSound::play(pTArea,3);
					exit(1);
				}
			} catch (const Json::RuntimeError& exc) {
				std::cout << "Error: " << exc.what() << std::endl;
			}
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "OK.", 0);
		} else if (msg->get_topic() == TOPIC_CONFIG_BME680) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "DETECTED bme680 config: {}", msg->get_topic());
			if (force_max_rate) {
				std::cout << "force_max_rate=true: ignoring bme680 config" << std::endl;
			} else {
				std::stringstream ssin(msg->to_string());
				Json::Value root;
				try {
					ssin >> root;
					double period = -1.0;
					if (root.isMember("period")) {
						period = root["period"].asDouble();
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  period: {}", period);
					}
					if (period >= 3.0) {
						period_bme680 = period;
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Setting period_bme680={}s",period_bme680);
					} else if (period >= 1.0) {
						period_bme680 = 3.0;
						spdlog::get("console")->warn("WRONG CONFIG: period should be >= 3.0. Setting period_bme680=3.0s",0);
					}
				} catch (const Json::RuntimeError& exc) {
					std::cout << "Error: " << exc.what() << std::endl;
				}
			}
		} else if (msg->get_topic() == TOPIC_CONFIG_LDR) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "DETECTED ldr config: {}", msg->get_topic());
			if (force_max_rate) {
				std::cout << "force_max_rate=true: ignoring ldr config" << std::endl;
			} else {
				std::stringstream ssin(msg->to_string());
				Json::Value root;
				try {
					ssin >> root;
					double period = -1.0;
					if (root.isMember("period")) {
						period = root["period"].asDouble();
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  period: {}", period);
					}
					if (period >= 1.0) {
						period_ldr = period;
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Setting period_ldr={}s",period_ldr);
					} else {
						spdlog::get("console")->warn("WRONG CONFIG: period >= 1.0. Setting period_ldr=1.0s",0);
					}
				} catch (const Json::RuntimeError& exc) {
					std::cout << "Error: " << exc.what() << std::endl;
				}
			}
		} else if (msg->get_topic() == TOPIC_CONFIG_CAM) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "DETECTED cam config: {}", msg->get_topic());
			if (force_max_rate) {
				std::cout << "force_max_rate=true: ignoring cam config" << std::endl;
			} else {
				std::stringstream ssin(msg->to_string());
				Json::Value root;
				try {
					ssin >> root;
					bool bon = root["on"].asBool();
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  on: {}", bon);
					double fps = -1.0;
					if (root.isMember("fps")) {
						fps = root["fps"].asDouble();
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  fps: {}", fps);
					}
					if (fps > 0.0) {
						//assert(pCam);
						if (pCam) pCam->setFps(fps);
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "CONFIG: fps:{}",fps);
					}
					if (bon) {
						//assert(pCam);
						if (pCam) pCam->start();
						first_message_cam_on_arrived = true;
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "CONFIG: start camera",0);
					} else {
						//assert(pCam);
						if (pCam) pCam->stop();
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "CONFIG: stop camera",0);
					}
				} catch (const Json::RuntimeError& exc) {
					std::cout << "Error: " << exc.what() << std::endl;
				}
			}
		} else if ((msg->get_topic() == TOPIC_LOCAL_NFC_DS_ACK) ||
				   (msg->get_topic() == TOPIC_OUTPUT_NFC_DS)) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "DETECTED nfc ds:{}", msg->get_topic());

			assert(pcli);
			pcli->setNfcRemote(msg->get_topic() == TOPIC_OUTPUT_NFC_DS);

			std::stringstream ssin(msg->to_string());
			Json::Value root;
			try {
				ssin >> root;
				std::string sts = root["ts"].asString();
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  ts:{}", sts);
				if (ft::trycheckTimestampTTL(sts))
				{
					std::string scmd = root["cmd"].asString();
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "  cmd:{}", scmd);
					if (scmd == "read_uid")
					{
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "read_uid",0);
						std::string sret = pNfc->readTagsGetUID();
						if (!sret.empty())
						{
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "readTagsGetUID OK",0);
						} else {
							std::cout << "Warning: readTagsGetUID is empty" << std::endl;
						}
					}
					else if (scmd == "read")
					{
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "read",0);
						std::string sret = pNfc->readTags();
						if (!sret.empty())
						{
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "readTags OK",0);
						} else {
							std::cout << "Warning: readTags is empty" << std::endl;
						}
					}
					else if (scmd == "write")
					{
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write",0);
						ft::TxtWorkpiece wp;
						Json::Value js_workpiece = root["workpiece"];
						if (js_workpiece != Json::Value::null) {
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write workpiece",0);
							wp.tag_uid = js_workpiece["id"].asString();
							std::string stype = js_workpiece["type"].asString();
							if (stype == "WHITE") {
								wp.type = ft::WP_TYPE_WHITE;
							} else if(stype == "RED") {
								wp.type = ft::WP_TYPE_RED;
							} else if (stype == "BLUE") {
								wp.type = ft::WP_TYPE_BLUE;
							} else {
								wp.type = ft::WP_TYPE_NONE;
							}
							std::string sstate = js_workpiece["state"].asString();
							if (sstate == "RAW") {
								wp.state = ft::WP_STATE_RAW;
							} else if(sstate == "PROCESSED") {
								wp.state = ft::WP_STATE_PROCESSED;
							} else if (sstate == "REJECTED") {
								wp.state = ft::WP_STATE_REJECTED;
							}
						}
						uint8_t mask_ts = 0x0;
						ft::HistoryCode_map_t map_hist;
						Json::Value js_history = root["history"];
						if (js_history != Json::Value::null) {
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write history",0);
							for (Json::Value::ArrayIndex i = 0; i != js_history.size(); i++)
							{
							    if (js_history[i].isMember("ts") && js_history[i].isMember("code"))
							    {
									ft::uTS uts;
									std::string sts = js_history[i]["ts"].asString();
									SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write ts: {}", sts);
									auto tp = ft::trygettimepoint(sts) + std::chrono::seconds(ft::time_offset()); // UTC time
									uts.s64 = tp.time_since_epoch().count();
									SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write uts.s64: {}",uts.s64);

									ft::TxtHistoryCode_t code = (ft::TxtHistoryCode_t)(js_history[i]["code"].asInt());
									SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "TxtHistoryCode_t: {}", code);
									ft::TxtHistoryIndex_t ind = toIndex(code);
									SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "TxtHistoryIndex_t: {}", ind);

									if(ind != ft::TxtHistoryIndex_t::INVALID_INDEX)
									{
										map_hist[code] = uts.s64;
										mask_ts |= 0x1 << ind;
										SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "mask_ts: {}", mask_ts);
									} else {
										std::cout << "Warning: code is not valid" << std::endl;
									}
							    } else {
									std::cout << "Warning: not both members exist for [" << i << "]: ts + code" << std::endl;
							    }
							}
						}
						bool bret = pNfc->writeTags(wp, map_hist, mask_ts);
						if (bret)
						{
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "writeTags OK",0);
						} else {
							std::cout << "Warning: readTags is empty" << std::endl;
						}
					}
					else if (scmd == "delete")
					{
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "delete",0);
						bool bret = pNfc->eraseTags();
						if (bret)
						{
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "eraseTags OK",0);
						} else {
							std::cout << "Warning: eraseTags is false" << std::endl;
						}
					}
					else
					{
						std::cout << "Error: Unknown command" << std::endl;
					}
				}
			} catch (const Json::RuntimeError& exc) {
				std::cout << "Error: " << exc.what() << std::endl;
			}
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "OK.", 0);
		}
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override {
		if (token) {
			SPDLOG_LOGGER_TRACE(spdlog::get("console"), "delivery_complete: {}: {}", token->get_message_id(), token->get_message()->get_topic());
		} else {
			SPDLOG_LOGGER_TRACE(spdlog::get("console"), "delivery token is NULL",0);
		}
	}

public:
	callback(ft::TxtMqttFactoryClient& cli) : cli_(cli) {}
};


int main(int argc, char* argv[])
{
	sprintf(TxtAppVer, "%d.%d.%d", (VERSION_HEX >> 16) & 0xff,
	            (VERSION_HEX >> 8) & 0xff, (VERSION_HEX >> 0) & 0xff);
	fprintf( stdout, "TxtGatewayPLC V%d.%d.%d\n", (VERSION_HEX >> 16) & 0xff,
	            (VERSION_HEX >> 8) & 0xff, (VERSION_HEX >> 0) & 0xff);
	try
	{
		//can be set globaly or per logger(logger->set_error_handler(..))
		spdlog::set_error_handler([](const std::string& msg)
	    {
			std::cout << "err handler spdlog:" << msg << std::endl;
			spdlog::get("file_logger")->error("err handler spdlog: {}",msg);
			exit(1);
	    });

		auto file_logger = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger", "Data/TxtFactoryMain.log", true);
		spdlog::get("file_logger")->set_level(spdlog::level::trace);
		spdlog::get("file_logger")->info("TxtFactoryMain {}", TxtAppVer);

		// Console logger with color
		auto console = spdlog::stdout_color_mt("console");
		auto console_axes = spdlog::stdout_color_mt("console_axes");
		//spdlog::set_formatter();
		spdlog::set_pattern("[%t][%Y-%m-%d %T.%e][%L] %v");
		spdlog::set_level(spdlog::level::trace);
		console_axes->set_level(spdlog::level::off);//trace);
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Log initialization failed: " << ex.what() << std::endl;
	}

	//load config
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::ifstream test("/opt/knobloch/.TxtGatewayPLC.json", std::ifstream::binary);
    if (test.is_open()) {
        std::cout << "load file /opt/knobloch/.TxtGatewayPLC.json" << std::endl;
        bool ok = Json::parseFromStream(builder, test, &root, &errs);
        if ( !ok )
        {
            std::cout  << errs << "\n";
        }
    }
    bool sound_enable = root.get("sound", true ).asBool();
    std::string host = root.get("host", "localhost" ).asString();
    int port = root.get("port", 1883 ).asInt();
	std::string mqtt_prefix = root.get("mqtt_prefix", "").asString();
	std::string mqtt_client_id = root.get("mqtt_client_id", "TxtGatewayPLC").asString();
    std::string mqtt_user = root.get("mqtt_user", "txt" ).asString();
    mqtt::binary_ref mqtt_pass = root.get("mqtt_pass", "xtx" ).asString();
    int w = root.get("cam_w", 320. ).asDouble();
    int h = root.get("cam_h", 240. ).asDouble();
    force_max_rate = root.get("force_max_rate", false).asBool();
    double max_limit_Area_moveDetect = root.get("max_limit_Area_moveDetect", 10000.0).asDouble();
    std::cout << "sound:" << sound_enable
    	<< " host:" << host
		<< " port:" << port
		<< " mqtt_client_id:" << mqtt_prefix << mqtt_client_id 
		<< " mqtt_user:" << mqtt_user
		<< " mqtt_pass:" << mqtt_pass << std::endl
		<< " cam w,h:" << w << "," << h << std::endl
		<< " force_max_rate:" << force_max_rate
		<< " max_limit_Area_moveDetect:" << max_limit_Area_moveDetect
		<< std::endl;

    if (StartTxtDownloadProg() == KELIB_ERROR_NONE)
    {
        pTArea = GetKeLibTransferAreaMainAddress();

        if (pTArea)
        {
            //SetTransferAreaCompleteCallback(JoysticksTransferAreaCallbackFunction);
		    try {
				std::cout << "Init MQTTClient" << std::endl;
				std::stringstream sout_port;
				sout_port << port;
				ft::TxtMqttFactoryClient mqttclient(mqtt_client_id, mqtt_prefix, host, sout_port.str(), mqtt_user, mqtt_pass);
				pcli = &mqttclient;
				callback cb(mqttclient);
				mqttclient.set_callback(cb);

				std::cout << "Init TxtBME680" << std::endl;
				ft::TxtBME680 bme680;
				pBme680 = &bme680;

				std::cout << "Init TxtCamera" << std::endl;
				ft::TxtCamera cam(w, h);
				pCam = &cam;
				std::cout << "Init TxtMotionDetection" << std::endl;
				ft::TxtMotionDetection mdcam(&cam, max_limit_Area_moveDetect); //default 500
				std::cout << "Start TxtCamera Thread" << std::endl;
				bool rcam = cam.startThread();
				if (!rcam) {
					std::cerr << "Error: init TxtCamera" << std::endl;
				} else {
					if (force_max_rate) {
						std::cout << "cam force_max_rate" << std::endl;
						cam.setFps(15.0);
						cam.start();
					}

					std::cout << "Start TxtMotionDetection Thread" << std::endl;
					bool retcam = mdcam.startThread();
					if (!retcam) {
						std::cerr << "Error: init TxtMotionDetection" << std::endl;
						return retcam;
					}
				}

				std::cout << "Connect MQTTClient" << std::endl;
				assert(pcli);
		    	bool ret = pcli->connect(TIMEOUT_CONNECTION_MS);
		    	if (!ret) {
			        std::cerr << "Error: timeout connecting to MQTT broker: " << TIMEOUT_CONNECTION_MS << "s" << std::endl;
			        return 1;
		    	}

				std::cout << "Init BME680" << std::endl;
	        	int ret2 = bme680.init();
	        	if (ret2 != 0) {
	        		std::cerr << "Error: init bme680" << std::endl;
	        	}
				if (force_max_rate) {
					std::cout << "bme680 force_max_rate" << std::endl;
					period_bme680 = 3.0;
				}

				std::cout << "Init NFC module" << std::endl;
				ft::TxtNfcDevice nfc;
				bool suc = nfc.open();
	        	if (!suc) {
	        		std::cerr << "Error: Init NFC module" << std::endl;
	        	}
	        	pNfc = &nfc;
				assert(pNfc);

				std::cout << "Waiting is_connected ... ";
				while(!pcli->is_connected());
				std::cout << "OK" << std::endl;

				std::cout << "Waiting first_message_subscribe ... ";
				while(!first_message_subscribe);
				std::cout << "OK" << std::endl;

				//start observers
				std::cout << "Init TxtBme680Observer" << std::endl;
				TxtBme680Observer obs_bme680(&bme680);
				std::cout << "Init TxtCameraObserver" << std::endl;
				TxtCameraObserver obs_cam(&cam);
				std::cout << "Init TxtMotionDetectionObserver" << std::endl;
				TxtMotionDetectionObserver obs_cammd(&mdcam);
				std::cout << "Init TxtAlertBme680Observer" << std::endl;
				TxtAlertBme680Observer obs_alertBme680(&bme680);
				std::cout << "Init TxtNfcDeviceObserver" << std::endl;
				ft::TxtNfcDeviceObserver obs_nfc(pNfc, pcli); //run in separate thread!

				if (force_max_rate) {
					std::cout << "ldr force_max_rate" << std::endl;
					period_ldr = 1.0;
				}

				bool firstvalue = true;

				//LDR setup
				pTArea->ftX1config.uni[2].mode = MODE_R;
				pTArea->ftX1config.uni[2].digital = 0;
				pTArea->ftX1state.config_id ++; // Save the new Setup

				while(true) {
					if (pTArea) {

						//fischertechnik Cloud
						if (!first_message_cam_on_arrived) {
							std::cout << "Broadcast MQTTClient" << std::endl;
							//Broadcast: to trigger config messages
							long timeout_ms = TIMEOUT_CONNECTION_MS;
							double timestamp_s = ft::getnowtimestamp_s();
							pcli->publishBroadcast(timestamp_s, "TxtGatewayPLC", TxtAppVer, "init", timeout_ms);
							sleep(5.0);
							//TODO timeout instead of sleep
						}

						//LDR
						double timestamp_s = ft::getnowtimestamp_s();
						double diff = timestamp_s - timestamp_ldr;
						//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "LDR diff:{}s", diff);
						if ((timestamp_ldr == 0) || (diff >= period_ldr)) {
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "timestamp_ldr:{} period_ldr:{}", timestamp_ldr, period_ldr);
							timestamp_ldr = timestamp_s;

							//LDR I3
							int16_t ldr = pTArea->ftX1in.uni[2];
							firstvalue = false;
							SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "ldr: {}", ldr);
							if (!firstvalue) {//Den ersten Wert verwerfen, da immer ungültig.
								long timeout_ms = TIMEOUT_CONNECTION_MS;//TODO period_ldr*1000;
								mqttclient.publishLDR(timestamp_s, ldr, timeout_ms);
							}
						}

						//NFC local
						switch (nfc.lastCmd)
						{
						case ft::NFC_ERROR:
							std::cout << "lastCmd: " << toString(nfc.lastCmd) << std::endl;
							nfc.Notify();
							nfc.lastCmd = ft::NFC_NONE;
							break;
						case ft::NFC_WRITE_ERROR:
							std::cout << "lastCmd: " << toString(nfc.lastCmd) << std::endl;
							nfc.Notify();
							nfc.lastCmd = ft::NFC_NONE;
							break;
						case ft::NFC_READ_UID:
							std::cout << "lastCmd: " << toString(nfc.lastCmd) << std::endl;
							nfc.Notify();
							nfc.lastCmd = ft::NFC_NONE;
							if (sound_enable) ft::TxtSound::play(pTArea,1);
							break;
						case ft::NFC_READ:
							std::cout << "lastCmd: " << toString(nfc.lastCmd) << std::endl;
							nfc.Notify();
							nfc.lastCmd = ft::NFC_NONE;
							if (sound_enable) ft::TxtSound::play(pTArea,1);
							break;
						case ft::NFC_DELETE:
							std::cout << "lastCmd: " << toString(nfc.lastCmd) << std::endl;
							nfc.Notify();
							nfc.lastCmd = ft::NFC_NONE;
							break;
						case ft::NFC_WRITE:
							std::cout << "lastCmd: " << toString(nfc.lastCmd) << std::endl;
							nfc.Notify();
							nfc.lastCmd = ft::NFC_NONE;
							if (sound_enable) ft::TxtSound::play(pTArea,1);
							break;
						default:
							break;
						}

						std::this_thread::sleep_for(std::chrono::milliseconds(10));
					}
				}

			} catch (const std::invalid_argument& exc) {
				std::cerr << "invalid_argument Error: " << exc.what() << std::endl;
				spdlog::get("file_logger")->error("invalid_argument Error: {}", exc.what());
				ft::TxtSound::play(pTArea,1);
				return 1;
			} catch (const Json::RuntimeError& exc) {
				std::cerr << "Json Error: " << exc.what() << std::endl;
				spdlog::get("file_logger")->error("Json Error: {}", exc.what());
				ft::TxtSound::play(pTArea,1);
		        return 1;
		    } catch (const mqtt::exception& exc) {
		        std::cerr << "MQTT Error: " << exc.what() << " [" << exc.get_reason_code() << "]" << std::endl;
				spdlog::get("file_logger")->error("MQTT Error: {} [{}]", exc.what(), exc.get_reason_code());
				ft::TxtSound::play(pTArea,2);
		        return 1;
			} catch (const cv::Exception& exc) {
				std::cerr << "OpenCV Error: " << exc.what() << std::endl;
				spdlog::get("file_logger")->error("OpenCV Error: {}", exc.what());
				ft::TxtSound::play(pTArea,1);
				return 1;
			} catch (const spdlog::spdlog_ex& exc) {
			    std::cerr << "Spdlog Error: " << exc.what() << std::endl;
				spdlog::get("file_logger")->error("Spdlog Error: {}", exc.what());
				ft::TxtSound::play(pTArea,1);
				return 1;
			}
        }
        StopTxtDownloadProg();
    }
	return 0;
}
