/*
 * TxtConveyorBelt.h
 *
 *  Created on: 18.02.2019
 *      Author: steiger-a
 */

#ifndef TxtConveyorBelt_H_
#define TxtConveyorBelt_H_

#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>
#include <thread>

#include "TxtAxis.h"
#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


class TxtConveyorBelt {
public:
	TxtConveyorBelt(TxtTransfer* pT, uint8_t chM);
	virtual ~TxtConveyorBelt();

	void setSpeed(int16_t s);
	int16_t getSpeed() { return speed; }

	void moveLeft();
	void moveRight();
	void stop();

protected:
	TxtTransfer* pT;
	uint8_t chM;
	int16_t speed;
};


class TxtConveyorBeltLightBarriers : public TxtConveyorBelt {
public:
	TxtConveyorBeltLightBarriers(TxtTransfer* pT, uint8_t chM, int chL1, int chL2);
	virtual ~TxtConveyorBeltLightBarriers();

	void moveIn();
	void moveOut();

protected:
	uint8_t chL1;
	uint8_t chL2;
};


} /* namespace ft */


#endif /* TxtConveyorBelt_H_ */
