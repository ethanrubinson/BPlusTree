#ifndef _INDEX_H
#define _INDEX_H

#include "minirel.h"

class IndexFileScan;

class IndexFile {

	friend IndexFileScan;
	
public:

    virtual ~IndexFile() {}; 
	
    virtual Status Insert (const char* data, const RecordID rid) = 0;
    virtual Status Delete (const char* data, const RecordID rid) = 0;
	
};


class IndexFileScan {

public:

	virtual ~IndexFileScan() {} 
	
	virtual Status GetNext (RecordID &rid, char* keyptr) = 0;
	
private:
	
};


#endif

