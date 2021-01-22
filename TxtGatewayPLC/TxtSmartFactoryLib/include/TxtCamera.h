/*
 * TxtCamera.h
 *
 *  Created on: 11.01.2018
 *      Author: steiger-a
 */

#ifndef TXTCAMERA_H_
#define TXTCAMERA_H_

#include <chrono>
#include <thread>

#include "opencv2/opencv.hpp"

#include "Observer.h"
#include "TxtCameraOverlay.h"

#include "spdlog/spdlog.h"


namespace ft {


class TxtCamera : public SubjectObserver {
public:
	TxtCamera(double w=320, double h=240);
	virtual ~TxtCamera();

	cv::Mat getFrame();

	void setFps(double f) { SPDLOG_LOGGER_TRACE(spdlog::get("console"), ""); fps = f; }
	double getPeriod() { return 1000./fps; }

	bool startThread();
	bool stopThread();

	void start() { doGrab = true; }
	void stop() { doGrab = false; }

	bool read();

	std::string getDataString();

	bool writeFile(const std::string& filename);

protected:
	bool init();

private:
	TxtCameraOverlay overlay;
	bool doGrab;
	cv::VideoCapture cap;
	double w;
	double h;
	int stride;
	double fps;
	cv::Mat frame;
	std::vector<uchar> buf;
    unsigned char * yuyv_buffer;


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
		reinterpret_cast<TxtCamera*>(obj)->run();
		return 0;
	}
};


} /* namespace ft */


#endif /* TXTCAMERA_H_ */
