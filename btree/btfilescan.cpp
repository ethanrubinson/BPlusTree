#include "minirel.h"
#include "bufmgr.h"
#include "db.h"
#include "new_error.h"
#include "btfile.h"
#include "btfilescan.h"

//-------------------------------------------------------------------
// BTreeFileScan::~BTreeFileScan
//
// Input   : None
// Output  : None
// Purpose : Clean Up the B+ tree scan.
//-------------------------------------------------------------------

BTreeFileScan::~BTreeFileScan ()
{
	// No cleanup needed
}

void BTreeFileScan::Init(const char *low, const char *high, PageID leftmostLeafPageID){
	lowKey = low;
	highKey = high;
	leftmostLeafID = leftmostLeafPageID;
	curPageID = NULL;
	scanStarted = false;
	scanFinished = false;

	if (leftmostLeafPageID == INVALID_PAGE) scanFinished = true;
}


//-------------------------------------------------------------------
// BTreeFileScan::GetNext
//
// Input   : None
// Output  : rid  - record id of the scanned record.
//           keyPtr - and a pointer to it's key value.
// Purpose : Return the next record from the B+-tree index.
// Return  : OK if successful, DONE if no more records to read.
//-------------------------------------------------------------------
Status BTreeFileScan::GetNext (RecordID & rid, char* keyPtr)
{

	if (scanFinished) return DONE;

	RecordID dataRid;
	BTLeafPage *curPage;

	if(!scanStarted) {
		// Find the record on the leftmost leaf page where we are supposed to start
		curPageID = leftmostLeafID;

		PIN(curPageID, curPage);
		
		// Find the first non-empty page
		while (curPage->GetFirst(curRid,keyPtr,dataRid) != OK) {
			PageID nextPageID = curPage->GetNextPage();
			UNPIN(curPageID, CLEAN);

			if (nextPageID == INVALID_PAGE) {
				scanFinished = true;
				return DONE;
			}
			
			curPageID = nextPageID;
			PIN(curPageID, curPage);
		}
		

		// Case where the lowKey is NULL (We want to start at the first spot)
		if (lowKey == NULL) {
			if (highKey == NULL || KeyCmp(keyPtr,highKey) <= 0) {
				rid = dataRid;
				scanStarted = true;
				UNPIN(curPageID, CLEAN);
				return OK;
			}
			else {
				scanFinished = true;
				UNPIN(curPageID, CLEAN);
				return DONE;
			}
		}
		// lowKey is not NULL
		else {
			if (KeyCmp(lowKey,keyPtr) <= 0) {
				if (highKey == NULL || KeyCmp(keyPtr,highKey) <= 0) {
					rid = dataRid;
					scanStarted = true;
					UNPIN(curPageID, CLEAN);
					return OK;
				}
				else {
					scanFinished = true;
					UNPIN(curPageID, CLEAN);
					return DONE;
				}
			}
			else{
				// Get the next record and try again
				while (curPage->GetNext(curRid,keyPtr,dataRid) != DONE){
					if (KeyCmp(lowKey,keyPtr) <= 0) {
						if (highKey == NULL || KeyCmp(keyPtr,highKey) <= 0) {
							rid = dataRid;
							scanStarted = true;
							UNPIN(curPageID, CLEAN);
							return OK;
						}
						else {
							scanFinished = true;
							UNPIN(curPageID, CLEAN);
							return DONE;
						}
					}
				}
				scanFinished = true;
				UNPIN(curPageID, CLEAN);
				return DONE;
			}	
		}
	}
	// The scan has allready been initialized
	else {
		// PIN our current page
		PIN(curPageID, curPage);
		Status s = curPage->GetNext(curRid, keyPtr, dataRid);

		// See if we have more records on that page, if not we need to get the next page
		while (s == DONE && curPage->GetNextPage() != INVALID_PAGE) {
			PageID nextPageID = curPage->GetNextPage();
			UNPIN(curPageID, CLEAN);
			curPageID = nextPageID;

			PIN(curPageID, curPage);
			s = curPage->GetFirst(curRid, keyPtr, dataRid);
		}

		// Check if we are done the scan
		if (s == DONE && curPage->GetNextPage() == INVALID_PAGE) {
				UNPIN(curPageID, CLEAN);
				scanFinished = true;
				return DONE;
		}
		// If we are not done, curRid, keyPtr, and dataRid are all propperly set
		else{
			if (highKey == NULL || KeyCmp(keyPtr,highKey) <= 0) {
				rid = dataRid;
				UNPIN(curPageID, CLEAN);
				return OK;
			}
			else {
				scanFinished = true;
				UNPIN(curPageID, CLEAN);
				return DONE;
			}
		}

	}
}
