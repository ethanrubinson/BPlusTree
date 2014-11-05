#ifndef _BTREE_FILESCAN_H
#define _BTREE_FILESCAN_H

#include "btfile.h"

class BTreeFile;

class BTreeFileScan : public IndexFileScan {
	
public:
	
	friend class BTreeFile;

    Status GetNext (RecordID & rid, char* keyptr);

	~BTreeFileScan();	

private:
	void Init(const char *lowKey, const char *highKey, PageID leftmostLeafPageID);
	const char *lowKey;
	const char *highKey;
	PageID leftmostLeafID;

	PageID curPageID;
	RecordID curRid;
	
	bool scanStarted;
	bool scanFinished;
};

#endif