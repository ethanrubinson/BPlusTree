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
	//TODO: add your code here
}

void BTreeFileScan::Init(const char *low, const char *high, PageID leftmostLeafPageID){
	lowKey = low;
	highKey = high;
	leftmostLeafID = leftmostLeafPageID;
	curPageID = NULL;
	scanStarted = false;
	scanFinished = false;

	/*if (lowKey == NULL) cout << "lowKey is NULL" << std::endl;
	else cout << "lowKey is " << lowKey << std::endl;

	if (highKey == NULL) cout << "highKey is NULL" << std::endl;
	else cout << "highKey is " << highKey << std::endl;*/

	if (leftmostLeafPageID == -1) scanFinished = true;
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
		// Find the record where we are supposed to start {MIDPOINT CHECK ONLY}
		curPageID = leftmostLeafID;

		PIN(curPageID, curPage);
		
		curPage->GetFirst(curRid,keyPtr,dataRid);
		
		// Case where the lowKey is NULL
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
	else {
		PIN(curPageID, curPage);

		if (curPage->GetNext(curRid,keyPtr,dataRid) == DONE) {
			scanFinished = true;
			UNPIN(curPageID, CLEAN);
			return DONE;
		}

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
