/*
 * TxtBME680.cpp
 *
 *  Created on: 20.12.2017
 *      Author: steiger-a
 */

#include "TxtBME680.h"

#include "Utils.h"

extern ft::TxtBME680* pBme680;


namespace ft {


/*!
 * @brief           Handling of the ready outputs
 *
 * @param[in]       timestamp       time in nanoseconds
 * @param[in]       iaq             IAQ signal
 * @param[in]       iaq_accuracy    accuracy of IAQ signal
 * @param[in]       temperature     temperature signal
 * @param[in]       humidity        humidity signal
 * @param[in]       pressure        pressure signal
 * @param[in]       raw_temperature raw temperature signal
 * @param[in]       raw_humidity    raw humidity signal
 * @param[in]       gas             raw gas sensor signal
 * @param[in]       bsec_status     value returned by the bsec_do_steps() call
 *
 * @return          none
 */
void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
     float pressure, float raw_temperature, float raw_humidity, float gas, bsec_library_return_t bsec_status)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "{} {} {} {} {} {} {} {} {} {}",
			timestamp,iaq,iaq_accuracy,temperature,humidity,
			pressure,raw_temperature,raw_humidity,gas,bsec_status);

	if (pBme680) {
		pBme680->_timestamp = timestamp;
		pBme680->_iaq = iaq;
		pBme680->_iaq_accuracy = iaq_accuracy;
		pBme680->_temperature = temperature;
		pBme680->_humidity = humidity;
		pBme680->_pressure = pressure;
		pBme680->_raw_temperature = raw_temperature;
		pBme680->_raw_humidity = raw_humidity;
		pBme680->_gas = gas;
		pBme680->Notify();
	}
}


TxtBME680::TxtBME680(float sample_rate, float temp_offset)
	: _timestamp(0), _iaq(-1.f), _iaq_accuracy(0), _temperature(-1000.0),
	_humidity(-1.0), _pressure(-1000.0), _raw_temperature(-1000.0),
	_raw_humidity(-1.0), _gas(0.0),
	sample_rate(sample_rate), temp_offset(temp_offset),
	m_stoprequested(false), m_running(false), m_mutex(), m_thread()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "", 0);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &attr);
}

TxtBME680::~TxtBME680() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "", 0);
	if (m_running) {
		exit();
	}
	pthread_mutex_destroy(&m_mutex);
}

int TxtBME680::init() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "", 0);
	uint32_t reti2c = InitI2C();
	if (reti2c != 0) {
		std::cout << "Error InitI2C: return " << reti2c << std::endl;
		return -1;
	}

    bsec_version_t  version;
    bsec_get_version(&version);
    printf("BSEC version: %d.%d.%d.%d\r\n",version.major, version.minor, version.major_bugfix, version.minor_bugfix);

    /* Call to the function which initializes the BSEC library
     * Switch on low-power mode and provide no temperature offset */
	printf("bsec_iot_init bme680... sample_Rate=BSEC_SAMPLE_RATE_LP, temp_offset=%0.2f\r\n",temp_offset);
    return_values_init ret;
    ret = bsec_iot_init(sample_rate, temp_offset, bus_write, bus_read, bsecsleep, state_load, config_load);
    if (ret.bme680_status)
    {
        /* Could not intialize BME680 */
        return (int)ret.bme680_status;
    }
    else if (ret.bsec_status)
    {
        /* Could not intialize BSEC library */
        return (int)ret.bsec_status;
    }
    //go
    assert(m_running == false);
    m_running = true;
    pthread_create(&m_thread, 0, start_thread, this);
    return BSEC_OK;
}

int TxtBME680::exit() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "", 0);
	//stop
    assert(m_running == true);
    m_running = false;
    m_stoprequested = true;
    return pthread_join(m_thread, 0);
}

void TxtBME680::run() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "",0);
	//TODO
	//printf("bsec_reset_output(BSEC_OUTPUT_IAQ_ESTIMATE)\r\n");
    //bsec_reset_output(BSEC_OUTPUT_IAQ_ESTIMATE);
    /* Call to endless loop function which reads and processes data based on sensor settings */
    /* State is saved every 10.000 samples, which means every 10.000 * 3 secs = 500 minutes  */
	unsigned int samples = 1000;
	printf("bsec_iot_loop bme680... state_saved=%d\r\n", samples);
    //TODO
	//bsec_iot_loop(bsecsleep, get_timestamp_us, output_ready, state_save, samples);
    bsec_iot_loop_cond(!m_stoprequested, bsecsleep, get_timestamp_us, output_ready, state_save, samples);
}


} /* namespace ft */
