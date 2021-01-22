/*
 * TxtAlert.h
 *
 *  Created on: 26.04.2018
 *      Author: steiger-a
 */

#ifndef TxtAlert_H_
#define TxtAlert_H_

#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <stdint.h>
#include <time.h>

#include "Observer.h"
#include "TxtBME680.h"

#include "spdlog/spdlog.h"


namespace ft {


template<typename T>
struct Bitset
{
public:
	Bitset(T value)
		: m_Data(value)
	{ }

	void Modify(int index, bool bit)
	{
		if (bit)
			m_Data |= 1 << index;
		else
			m_Data &= ~(1 << index);
	}

	bool Get(int index) const
	{
		return m_Data & (1 << index);
	}

	T GetValue() const
	{
		return m_Data;
	}

private:
	T m_Data{0};
};


class TxtFlapping {
	public:
	TxtFlapping();
		virtual ~TxtFlapping();

		void UpdateFlappingStatus(bool stateChange);
		bool IsFlapping() const { return flapping; }

private:
	Bitset<unsigned long> stateChangeBuf;
	int flappingIndex;
	bool flapping;
	double flappingThresholdLow;
	double flappingThresholdHigh;
	double flappingValue;
	time_t rawtimeLastChange;
};


} /* namespace ft */


#endif /* TxtAlert_H_ */
