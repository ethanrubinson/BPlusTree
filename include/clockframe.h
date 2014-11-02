#ifndef _CLOCKFRAME_H
#define _CLOCKFRAME_H

#include "frame.h"

class ClockFrame : public Frame 
{
	private :
		
		bool referenced;

 	public :

		ClockFrame();
		~ClockFrame();
	
		void Unpin();
		Status Free();
		void UnsetReferenced();
		bool IsReferenced();
		bool IsVictim();
};

#endif