/*
 * TxtAxisNSwitch.h
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#ifndef TxtAxisNSwitch_H_
#define TxtAxisNSwitch_H_


#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>
#include <thread>
#include <vector>

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "TxtAxis.h"


namespace ft {


class TxtAxisNSwitch : public TxtAxis {
public:
	TxtAxisNSwitch(std::string name, TxtTransfer* pT, uint8_t chM, uint8_t chS1, uint8_t chS2);
	TxtAxisNSwitch(std::string name, TxtTransfer* pT, uint8_t chM, uint8_t chS1, uint8_t chS2, uint8_t chS3);
	virtual ~TxtAxisNSwitch();

	void moveS2X(int idx);
	void moveS1() { moveS2X(0); }
	std::thread moveS1Thread() {
		return std::thread(&TxtAxisNSwitch::moveS1, this);
	}
	void moveS2() { moveS2X(1); }
	std::thread moveS2Thread() {
		return std::thread(&TxtAxisNSwitch::moveS2, this);
	}
	void moveS3() { moveS2X(2); }
	std::thread moveS3Thread() {
		return std::thread(&TxtAxisNSwitch::moveS3, this);
	}

	bool isS2XValid(int idx) { return idx >= 0 && (size_t)idx < chS2X.size(); }

protected:
	std::vector<uint8_t> chS2X;
};


} /* namespace ft */


#endif /* TxtAxisNSwitch_H_ */
