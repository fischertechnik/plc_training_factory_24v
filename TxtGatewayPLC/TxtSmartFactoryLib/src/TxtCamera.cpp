/*
 * TxtCamera.cpp
 *
 *  Created on: 11.01.2018
 *      Author: steiger-a
 */

#include "TxtCamera.h"

#include "base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <thread>	// For sleep
#include <chrono>

//#define USE_YUYV


namespace ft {


TxtCamera::TxtCamera(double w, double h) :
	overlay(w,h), doGrab(false), cap(), w(w), h(h), stride(0), fps(15.0), yuyv_buffer(0), m_stoprequested(false),
	m_running(false), m_mutex(), m_thread()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtCamera w:{} h:{}", w, h);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &attr);
}

TxtCamera::~TxtCamera() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtCamera");
	if (m_running) {
		stop();
	}
	pthread_mutex_destroy(&m_mutex);
}

bool TxtCamera::init() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "init");
	bool ret = cap.open(0);
	if (!ret) {
        std::cout << "error cap.open(0)" << std::endl;
	}
#ifndef USE_YUYV
    bool r = cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
    if (!r) {
        std::cout << "error cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'))" << std::endl;
    	return false;
    }
#else
    bool r = cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y','U','Y','V'));
    if (!r) {
        std::cout << "error cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y','U','Y','V'))" << std::endl;
    	return false;
    }
#endif
    //cap.set(cv::CAP_PROP_FPS, 15);
    //cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 3);
    r = cap.set(cv::CAP_PROP_FRAME_WIDTH, w);//640);
    if (!r) {
        std::cout << "error cap.set(cv::CAP_PROP_FRAME_WIDTH, w)" << std::endl;
    	return false;
    }
    r = cap.set(cv::CAP_PROP_FRAME_HEIGHT, h);//480);
    if (!r) {
        std::cout << "error cap.set(cv::CAP_PROP_FRAME_HEIGHT, h)" << std::endl;
    	return false;
    }
    std::cout << "init frame " << cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH);
    std::cout << "x" << cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT);
    std::cout << " Format: " << cap.get(cv::VideoCaptureProperties::CAP_PROP_FORMAT) << std::endl;
    return cap.isOpened();
}

bool TxtCamera::startThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "start");
	if (m_running) return true; //already running
	if (!init()) {
    	return false;
	}
    //go
    assert(m_running == false);
    m_running = true;
    /*pthread_attr_t tattr;
    int ret;
    sched_param param;
    int newprio = 20;
    ret = pthread_attr_init(&tattr);
    ret = pthread_attr_getschedparam (&tattr, &param);
    LOG("old priority %d", param.sched_priority);
    param.sched_priority = newprio;
    ret = pthread_attr_setschedparam (&tattr, &param);
    LOG("new priority %d",newprio);*/
    //return pthread_create(&m_thread, &tattr, start_thread, this) == 0;
    return pthread_create(&m_thread, 0, start_thread, this) == 0;
}

bool TxtCamera::stopThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stop");
	if (!m_running) return true; //already stopped
	//stop
    assert(m_running == true);
    m_running = false;
    m_stoprequested = true;
    return pthread_join(m_thread, 0) == 0;
}

cv::Mat TxtCamera::getFrame() {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getFrame");
	cv::Mat frame_ret;
	//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock getFrame");
	pthread_mutex_lock(&m_mutex);
	frame.copyTo(frame_ret);
	pthread_mutex_unlock(&m_mutex);
	//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock getFrame");
	return frame_ret;

}

void TxtCamera::run() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run");
    while (!m_stoprequested)
    {
    	if (doGrab)
    	{
            if (!cap.isOpened()) {
        		spdlog::get("console")->warn("cap.isOpened() == false. Will try to reopen capture.");
            	if (!init()) {
                    std::cout << "error init" << std::endl;
            	}
            }

#ifdef CAM_TEST
    		spdlog::get("console")->info("CAM 0: --- get frame");
#endif
    		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock run");
    		pthread_mutex_lock(&m_mutex);
#ifdef USE_YUYV
    		cap >> frame;
    		cvtColor(frame, frame, cv::COLOR_YUV2BGR_YUYV);
#else
    		cap >> frame;
#endif
    		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock run");
    		//cv::flip(frame,frame,0);
    		//bool rg = cap.grab();
    		//bool rr = cap.retrieve(frame);

    		if (!frame.empty()) {
    			Notify(); // new frame
    		}
    		pthread_mutex_unlock(&m_mutex);

    	}
		std::this_thread::sleep_for(std::chrono::milliseconds(67));
	}
    frame.release();
}

std::string TxtCamera::getDataString() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getDataString");
	std::string s;
	pthread_mutex_lock(&m_mutex);
	if (!frame.empty()) {
		bool ret = false;
		try {
#ifdef CAM_TEST
			spdlog::get("console")->info("CAM 1: --- imencode jpg");
#endif
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock getDataString");

			overlay.draw(frame);

			ret = cv::imencode(".jpg", frame, buf);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock getDataString");
		} catch (const cv::Exception& exc) {
			std::cout << "Error: " << exc.what() << std::endl;
			return "";
		}
		if (!buf.empty()) {
			if (ret) {
#ifdef CAM_TEST
				spdlog::get("console")->info("CAM 2: --- base64_encode");
#endif
				s = "data:image/jpeg;base64," + base64_encode(buf.data(), buf.size());
			}
		}
	}
	pthread_mutex_unlock(&m_mutex);
	return s;
}

bool TxtCamera::writeFile(const std::string& filename) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "writeFile filename:{}", filename.c_str());
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_lock writeFile");
	pthread_mutex_lock(&m_mutex);
	bool ret = cv::imwrite(filename, frame);
	pthread_mutex_unlock(&m_mutex);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pthread_mutex_unlock writeFile");
	return ret;
}


} /* namespace ft */
