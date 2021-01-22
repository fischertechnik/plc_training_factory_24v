/*
 * Observer.h
 *
 *  Created on: 20.01.2018
 *      Author: steiger-a
 */

#ifndef OBSERVER_H_
#define OBSERVER_H_

#include <string>
#include <vector>
#include <pthread.h>
#include <assert.h>
#include <iostream>


namespace ft {


//see: http://www.bogotobogo.com/DesignPatterns/observer.php
class SubjectObserver;

class Observer {
public:
	Observer() {};
	virtual ~Observer() {};
	virtual void Update(SubjectObserver* theChangeSubject) = 0;
};

class SubjectObserver {
public:
	SubjectObserver() {};
	virtual ~SubjectObserver() {};
	virtual void Attach(Observer*);
	virtual void Detach(Observer*);
	virtual void Notify();
private:
	std::vector<Observer*> _observers;
};


} /* namespace ft */


#endif /* OBSERVER_H_ */
