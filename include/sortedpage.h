#ifndef SORTED_PAGE_H
#define SORTED_PAGE_H


#include "minirel.h"
#include "page.h"
#include "heappage.h"
#include "bt.h"


class SortedPage : public HeapPage {
	
private:
	
	// No private variables should be declared.
	
public:
		
	Status InsertRecord(char * recPtr, int recLen, RecordID& rid);	
	Status DeleteRecord(const RecordID& rid);
	
	void  SetType(NodeType t)  { type = (short)t; }

	NodeType GetType()         { return (NodeType)type; }
	int   GetNumOfRecords() { return numOfSlots; }
};

#endif
