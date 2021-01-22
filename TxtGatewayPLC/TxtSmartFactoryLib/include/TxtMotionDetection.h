/*
 * TxtMotionDetection.h
 *
 *  Created on: 15.02.2018
 *      Author: steiger-a
 */

#ifndef TXTMOTIONDETECTION_H_
#define TXTMOTIONDETECTION_H_

#include <chrono>
#include <thread>
#include <vector>
#include <pthread.h>

#include "opencv2/opencv.hpp"

#include "Observer.h"
#include "TxtCamera.h"

#define TIMEWAIT_S_MAX 5.0


namespace ft {


class TxtMotionDetection : public SubjectObserver {
public:
	TxtMotionDetection(ft::TxtCamera* cam, double max_limit_Area);
	virtual ~TxtMotionDetection();

	bool init();
	bool startThread();
	bool stopThread();

	std::string getDataString();

protected:
	ft::TxtCamera* cam;
	cv::Mat gray_last;
	std::chrono::system_clock::time_point tsLastDetected;
	double max_limit_Area;

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
		reinterpret_cast<TxtMotionDetection*>(obj)->run();
		return 0;
	}
};


} /* namespace ft */


#endif /* TXTMOTIONDETECTION_H_ */
