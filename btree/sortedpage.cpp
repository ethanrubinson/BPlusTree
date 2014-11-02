/*
* sorted_page.cc - implementation of class SortedPage
*
* Johannes Gehrke & Gideon Glass  951016  CS564  UW-Madison
*/

#include "sortedpage.h"
#include "btindex.h"
#include "btleaf.h"

//-------------------------------------------------------------------
// SortedPage::InsertRecord
//
// Input   : recPtr  - pointer to the record to be inserted
//           recLen  - length of the record
// Output  : rid - record id of the inserted record
// Precond : There is enough space on this page to accomodate this
//           record.  The records on this page is sorted and the
//           slots directory is compact.
// Postcond: The records on this page is still sorted and the
//           slots directory is compact.
// Purpose : Insert the record into this page.
//-------------------------------------------------------------------

Status SortedPage::InsertRecord (char * recPtr,
                                 int recLen, RecordID& rid)
{
	Status status;
	int i;
	
	// ASSERTIONS:
	// - the slot directory is compressed -> inserts will occur at the end
	// - slotCnt gives the number of slots used
	
	// general plan:
	//    1. Insert the record into the page,
	//       which is then not necessarily any more sorted
	//    2. Sort the page by rearranging the slots (insertion sort)
	
	status = HeapPage::InsertRecord (recPtr, recLen, rid);
	if (status != OK)
		return FAIL;
	
	// performs a simple insertion sort

	for (i = numOfSlots - 1; i > 0; i--) 
	{
		char *x = data + slots[i].offset;
		char *y = data + slots[i-1].offset;
		
		if (KeyCmp((char*)x, (char*)y) < 0)
		{
			// switch slots

			Slot tmpSlot;
			tmpSlot   = slots[i];
			slots[i]   = slots[i-1];
			slots[i-1] = tmpSlot;
			
		} else {
			
			// end insertion sort

			break;
		}
	}
	
	// ASSERTIONS:
	// - record keys increase with increasing slot number (starting at slot 0)
	// - slot directory compacted
	
	rid.slotNo = i;
		
	return OK;
}


//-------------------------------------------------------------------
// SortedPage::DeleteRecord
//
// Input   : rid - record id of the record to be deleted.
// Output  : None
// Postcond: The slots directory is compact.
// Purpose : Delete a record from this page, and compact the slot
//           directory.
//-------------------------------------------------------------------

Status SortedPage::DeleteRecord (const RecordID& rid)
{
	Status status;
	
	status = HeapPage::DeleteRecord (rid);
	
	if (status == OK)
		HeapPage::CompactSlotDir();
	else
		return FAIL;
	
	// ASSERTIONS:
	// - slot directory is compacted
	
	return OK;
}

