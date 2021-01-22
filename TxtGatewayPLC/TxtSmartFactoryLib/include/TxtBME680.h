/*
 * TxtBME680.h
 *
 *  Created on: 20.12.2017
 *      Author: steiger-a
 */

#ifndef TXTBME680_H_
#define TXTBME680_H_

#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>
#include <set>
#include <algorithm>
#include <pthread.h>
#include <assert.h>

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "TxtBME680BSECLib.h"
#include "Observer.h"

#include "spdlog/spdlog.h"


namespace ft {


void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
     float pressure, float raw_temperature, float raw_humidity, float gas, bsec_library_return_t bsec_status);


class TxtBME680 : public SubjectObserver {
public:
	int64_t _timestamp = 0;
	float _iaq = 0.f;
	uint8_t _iaq_accuracy = 0;
	float _temperature = 0.f;
	float _humidity = 0.f;
	float _pressure = 0.f;
	float _raw_temperature = 0.f;
	float _raw_humidity = 0.f;
	float _gas = 0.f;

	TxtBME680(float sample_rate = BSEC_SAMPLE_RATE_LP, float temp_offset = 3.1f);
    //temp_offset = 3.1f; // Deckel gr�n
    //temp_offset = 4.2f; // Deckel gr�n mit 3D-Drucker alt
    //temp_offset = 1.9f; // ohne Deckel, liegend
    //temp_offset = 2.9f; // ohne Deckel, stehend

	virtual ~TxtBME680();

	int init();
	int exit();

private:
	float sample_rate;
	float temp_offset;

	//Thread
    volatile bool m_stoprequested;
    volatile bool m_running;
    pthread_mutex_t m_mutex;
    pthread_t m_thread;

	void run();

    // This is the static class function that serves as a C style function pointer
	// for the pthread_create call
	static void* start_thread(void *obj)
	{
		//All we do here is call the do_work() function
		reinterpret_cast<TxtBME680*>(obj)->run();
		return 0;
	}
};


} /* namespace ft */


#endif /* TXTBME680_H_ */
