#ifndef BTLEAF_PAGE_H
#define BTLEAF_PAGE_H

#include "minirel.h"
#include "page.h"
#include "sortedpage.h"
#include "bt.h"
#include "btindex.h"


class BTLeafPage : public SortedPage {
	
private:
	
	// No private variables should be declared.
	
public:
		
	Status Insert (const char *key, RecordID dataRid, RecordID& rid);
	
	Status GetFirst (RecordID& rid, char* key, RecordID & dataRid);
	Status GetNext  (RecordID& rid, char* key, RecordID & dataRid);
	Status GetCurrent (RecordID rid, char* key, RecordID & dataRid);
	
	Status Delete (const char* key, const RecordID& dataRid);
};

#endif
