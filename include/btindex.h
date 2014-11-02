#ifndef BTINDEX_PAGE_H
#define BTINDEX_PAGE_H


#include "minirel.h"
#include "page.h"
#include "sortedpage.h"
#include "bt.h"



class BTIndexPage : public SortedPage {
	
private:

	// No private variables should be declared.

public:
	
	Status Insert (const char *key, PageID pageNo, RecordID& rid);
	Status Delete (const char *key, RecordID& curRid);
	Status GetPageID (const char *key, PageID & pageNo);
	Status GetSibling(const char *key, PageID & pageNo, int &left);
	Status GetFirst (RecordID& rid, char *key, PageID & pageNo);
	Status GetNext (RecordID& rid, char *key, PageID & pageNo);
	
	PageID GetLeftLink (void);
	void   SetLeftLink (PageID left);
	    
	Status FindKey (char *key, char *entry);
	Status AdjustKey (const char *newKey, const char *oldKey);
};

#endif
