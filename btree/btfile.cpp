#include "minirel.h"
#include "bufmgr.h"
#include "db.h"
#include "new_error.h"
#include "btfile.h"
#include "btfilescan.h"
#include <stack>

const bool DEBUG_MODE = true;

void BTreeFile::debugPrint(const char *msg){
	if(DEBUG_MODE) std::cout << msg << std::endl;
}

//-------------------------------------------------------------------
// BTreeFile::BTreeFile
//
// Input   : filename - filename of an index.
// Output  : returnStatus - status of execution of constructor.
//           OK if successful, FAIL otherwise.
// Purpose : Open the index file, if it exists.
//			 Otherwise, create a new index, with the specified
//           filename. You can use
//                MINIBASE_DB->GetFileEntry(filename, headerID);
//           to retrieve an existing index file, and
//                MINIBASE_DB->AddFileEntry(filename, headerID);
//           to create a new one. You should pin the header page
//           once you have read or created it. You will use the header
//           page to find the root node.
//-------------------------------------------------------------------
BTreeFile::BTreeFile (Status& returnStatus, const char *filename) {
	// Save the name of the file so we delete appropriately
	// when DestroyFile is called.
	dbname = strcpy(new char[strlen(filename) + 1], filename);

	Status stat = MINIBASE_DB->GetFileEntry(filename, headerID);
	Page *_headerPage;
	returnStatus = OK;

	// File does not exist, so we should create a new index file.
	if (stat == FAIL) {
		// Allocate a new header page.
		stat = MINIBASE_BM->NewPage(headerID, _headerPage, 1);

		if (stat != OK) {
			std::cerr << "Error allocating header page." << std::endl;
			headerID = INVALID_PAGE;
			header = NULL;
			returnStatus = FAIL;
			return;
		}

		header = (BTreeHeaderPage *)(_headerPage);
		header->Init(headerID);
		stat = MINIBASE_DB->AddFileEntry(filename, headerID);

		if (stat != OK) {
			std::cerr << "Error creating file" << std::endl;
			headerID = INVALID_PAGE;
			header = NULL;
			returnStatus = FAIL;
			return;
		}
	} else {
		stat = MINIBASE_BM->PinPage(headerID, _headerPage);

		if (stat != OK) {
			std::cerr << "Error pinning existing header page" << std::endl;
			headerID = INVALID_PAGE;
			header = NULL;
			returnStatus = FAIL;
		}

		header = (BTreeHeaderPage *) _headerPage;
	}
}


//-------------------------------------------------------------------
// BTreeFile::~BTreeFile
//
// Input   : None
// Return  : None
// Output  : None
// Purpose : Free memory and clean Up. You should be sure to
//           unpin the header page if it has not been unpinned
//           in DestroyFile.
//-------------------------------------------------------------------
BTreeFile::~BTreeFile ()
{
    delete [] dbname;
	
    if (headerID != INVALID_PAGE) 
	{
		Status st = MINIBASE_BM->UnpinPage (headerID, CLEAN);
		if (st != OK)
		{
		cerr << "ERROR : Cannot unpin page " << headerID << " in BTreeFile::~BTreeFile" << endl;
		}
    }
}


//-------------------------------------------------------------------
// BTreeFile::DestroyFile
//
// Input   : None
// Output  : None
// Return  : OK if successful, FAIL otherwise.
// Purpose : Free all pages and delete the entire index file. Once you have
//           freed all the pages, you can use MINIBASE_DB->DeleteFileEntry (dbname)
//           to delete the database file.
//-------------------------------------------------------------------
Status BTreeFile::DestroyFile ()
{
	if (header->GetRootPageID() != INVALID_PAGE && _DestroyFile(header->GetRootPageID()) != OK) {
		return FAIL;
	}

	FREEPAGE(headerID);
	headerID = INVALID_PAGE;
	header = NULL;

	if (MINIBASE_DB->DeleteFileEntry(dbname) != OK) {
		debugPrint("[ERROR] MINIBASE_DB->DeleteFileEntry failed in BTreeFile::DestroyFile()");
		return FAIL;
	}

	return OK;
}

Status BTreeFile::_DestroyFile (PageID pageID)
{
	SortedPage *page;
	BTIndexPage *index;

	Status s;
	PageID curPageID;

	RecordID curRid;
	KeyType key;

	PIN (pageID, page);
	NodeType type = page->GetType ();

	switch (type) {
	case INDEX_NODE:
		index = (BTIndexPage *)page;
		curPageID = index->GetLeftLink();
		if (_DestroyFile(curPageID) != OK) return FAIL;

		s=index->GetFirst(curRid, key, curPageID);
		if ( s == OK) {	
			if (_DestroyFile(curPageID) != OK) return FAIL;

			s = index->GetNext(curRid, key, curPageID);
			while ( s != DONE) {	
				if (_DestroyFile(curPageID) != OK) return FAIL;
				s = index->GetNext(curRid, key, curPageID);
			}
		}

		FREEPAGE(pageID);
		break;

	case LEAF_NODE:
		FREEPAGE(pageID);
		break;
	default:		
		assert (0);
	}

	return OK;
}

//-------------------------------------------------------------------
// BTreeFile::SplitLeafNode
//
// Input   : key - pointer to the value of the key to be inserted.
//           rid - RecordID of the record to be inserted.
//			 fullPage - pointer to the page that needs to be split
// Output  : newPageID - ID of the newly created page
//			 newPageFirstKey - pointer to the value of the first key on the new page
// Return  : OK if successful, FAIL otherwise.
// Purpose : Split a leafNode into two nodes 
//-------------------------------------------------------------------
Status BTreeFile::SplitLeafNode(const char *key, const RecordID rid, BTLeafPage *fullPage, PageID &newPageID, KeyType &newPageFirstKey) {
	
	// Create and initialize the page for the new leaf node
	Page *newPage;
	NEWPAGE(newPageID, newPage);
	BTLeafPage *newLeafPage = (BTLeafPage *) newPage;
	newLeafPage->Init(newPageID);
	newLeafPage->SetType(LEAF_NODE);

	// If the page of the full leaf node has a valid next page, set the next page's prev-pointer to point to the new page
	PageID nextPageID = fullPage->GetNextPage();
	if (nextPageID != INVALID_PAGE) {
		Page *nextPage;
		PIN(nextPageID, nextPage);
		BTLeafPage *nextLeafPage = (BTLeafPage *) nextPage;
		nextLeafPage->SetPrevPage(newPageID);
		UNPIN(nextPageID, DIRTY);
	}

	// Link the new pages together in the B+ tree
    newLeafPage->SetNextPage(nextPageID);
	newLeafPage->SetPrevPage(fullPage->PageNo());
    fullPage->SetNextPage(newPageID);

	// Move all of the records from the old page to the new page
	while (true) {
		KeyType movedKey;
		RecordID movedVal, firstRid, insertedRid;
		Status s = fullPage->GetFirst(firstRid, movedKey, movedVal);
		if (s == DONE) break;
		if (newLeafPage->Insert(movedKey, movedVal, insertedRid) != OK) {
			std::cerr << "Moving records failed on insert while splitting leaf node num=" << fullPage->PageNo() << std::endl;
			UNPIN(newPageID, DIRTY);
			return FAIL;
		}
		if (fullPage->DeleteRecord(firstRid) != OK) {
			std::cerr << "Moving records failed on delete while splitting leaf node num=" << fullPage->PageNo() << std::endl;
			UNPIN(newPageID, DIRTY);
			return FAIL;
		}
	}

	// Move the items back from the new page to the old page until the space is relatively equal
	// If we run across the insert point for our new value while this is occuring, insert it. This will
	// be taken care of later if it is not done during this step.
	bool didInsert = false;
	KeyType curKey;
	RecordID curRid, curVal, insertedRid;
	newLeafPage->GetFirst(curRid, curKey, curVal);
	while (fullPage->AvailableSpace() > newLeafPage->AvailableSpace()) {

		// Check if the key we are currently on is bigger than the one we are trying to insert
		// If so, this is our insert oppertunity so insert the new value into the old (first) page
		if(!didInsert && KeyCmp(curKey,key) > 0){
			if (fullPage->Insert(key, rid, insertedRid) != OK){
				UNPIN(newPageID, DIRTY);
				return FAIL;
			}
			didInsert = true;
		}
		else {
			if (fullPage->Insert(curKey,curVal,insertedRid) != OK){
				UNPIN(newPageID, DIRTY);
				return FAIL;
			}
			if (newLeafPage->DeleteRecord(curRid) != OK){
				UNPIN(newPageID, DIRTY);
				return FAIL;
			}

			newLeafPage->GetCurrent(curRid, curKey, curVal);
		}
	}

	// If we were unable to insert our new value in the previous step, do so now. However now it is inserted into the new (second) page.
	if (!didInsert) {
		if (newLeafPage->Insert(key, rid, insertedRid) != OK){
			UNPIN(newPageID, DIRTY);
			return FAIL;
		}
	}

	// Set the output which is the first key of the new (second) page
	KeyType newKey;
	newLeafPage->GetFirst(curRid, newPageFirstKey, curVal);
	UNPIN(newPageID, DIRTY);
	return OK;
}

//-------------------------------------------------------------------
// BTreeFile::SplitIndexNode
//
// Input   : key - pointer to the value of the key to be inserted.
//           rid - PageID of the record to be inserted.
//			 fullPage - pointer to the page that needs to be split
// Output  : newPageID - ID of the newly created page
//			 newPageFirstKey - pointer to the value of the extra key to be added to the indexnode one level up
// Return  : OK if successful, FAIL otherwise.
// Purpose : Split an indexnode into two nodes 
//-------------------------------------------------------------------
Status BTreeFile::SplitIndexNode(const char *key, const PageID pid, BTIndexPage *fullPage, PageID &newPageID, KeyType &newPageFirstKey) {
	
	// Create and initialize the page for the new index node
	Page *newPage;
	NEWPAGE(newPageID, newPage);
	BTIndexPage *newIndexPage = (BTIndexPage *) newPage;
	newIndexPage->Init(newPageID);
	newIndexPage->SetType(INDEX_NODE);


	// Move all the records from the old page to the new page
	while (true) {
		KeyType movedKey;
		RecordID firstRid, insertedRid;
		PageID movedVal;
		Status s = fullPage->GetFirst(firstRid, movedKey, movedVal);
		if (s == DONE) break;
		if (newIndexPage->Insert(movedKey, movedVal, insertedRid) != OK) {
			std::cerr << "Moving records failed on insert while splitting index node num=" << fullPage->PageNo() << std::endl;
			UNPIN(newPageID, DIRTY);
			return FAIL;
		}
		if (fullPage->DeleteRecord(firstRid) != OK) {
			std::cerr << "Moving records failed on delete while splitting index node num=" << fullPage->PageNo() << std::endl;
			UNPIN(newPageID, DIRTY);
			return FAIL;
		}
	}

	// Move the items back from the new page to the old page until the space is relatively equal
	// If we run across the insert point for our new value while this is occuring, insert it. This will
	// be taken care of later if it is not done during this step.
	bool didInsert = false;
	KeyType curKey;
	RecordID curRid, insertedRid;
	PageID curVal;
	newIndexPage->GetFirst(curRid, curKey, curVal);
	while (fullPage->AvailableSpace() > newIndexPage->AvailableSpace()) {
		// Check if the key we are currently on is bigger than the one we are trying to insert
		// If so, this is our insert oppertunity so insert the new value into the old (first) page
		if(!didInsert && KeyCmp(curKey,key) > 0){
			if (fullPage->Insert(key, pid, insertedRid) != OK){
				UNPIN(newPageID, DIRTY);
				return FAIL;
			}
			didInsert = true;
		}
		else {
			if (fullPage->Insert(curKey,curVal,insertedRid) != OK) {
				UNPIN(newPageID, DIRTY);
				return FAIL;
			}
			if (newIndexPage->DeleteRecord(curRid) != OK){
				UNPIN(newPageID, DIRTY);
				return FAIL;
			}
			newIndexPage->GetFirst(curRid, curKey, curVal);
		}
	}

	// If we were unable to insert our new value in the previous step, do so now. However now it is inserted into the new (second) page.
	if (!didInsert) {
		if (newIndexPage->Insert(key, pid, insertedRid) != OK){
			UNPIN(newPageID, DIRTY);
			return FAIL;
		}
	}

	// Set the output which is the first key of the new (second) page
	newIndexPage->GetFirst(curRid, newPageFirstKey, curVal);

	// Set the left link of the new index node and delete the duplicate key
	newIndexPage->SetLeftLink(curVal);
	newIndexPage->DeleteRecord(curRid);

	UNPIN(newPageID, DIRTY);

	return OK;
}

//-------------------------------------------------------------------
// BTreeFile::Insert
//
// Input   : key - pointer to the value of the key to be inserted.
//           rid - RecordID of the record to be inserted.
// Output  : None
// Return  : OK if successful, FAIL otherwise.
// Purpose : Insert an index entry with this rid and key.  
// Note    : If the root didn't exist, create it.
//-------------------------------------------------------------------
Status BTreeFile::Insert (const char *key, const RecordID rid)
{
	RecordID newRecordID;

	if (header->GetRootPageID() == INVALID_PAGE) {
		// There is no root page, we need to make one
		PageID newPageID;
		Page *newPage;

		NEWPAGE(newPageID, newPage);

		BTLeafPage *newLeafPage = (BTLeafPage *) newPage;
		newLeafPage->Init(newPageID);
		newLeafPage->SetType(LEAF_NODE);

		if (newLeafPage->Insert(key, rid, newRecordID) != OK) {
			FREEPAGE(newPageID);
			return FAIL;
		}

		header->SetRootPageID(newPageID);

		UNPIN(newPageID, DIRTY);
	}
	else {
		// A root page exists
		PageID rootID = header->GetRootPageID();
		SortedPage *rootPage;
		PIN(rootID, rootPage);
		Status s;

		// The rootPage is a leaf node, this is the trivial case
		if (rootPage->GetType() == LEAF_NODE) {
            BTLeafPage* rootleaf = (BTLeafPage*) rootPage;

			// See if there is space in the root leaf to insert the new key
            if (rootPage->AvailableSpace() >= GetKeyDataLength(key,LEAF_NODE)) {
                if(rootleaf->Insert(key, rid, newRecordID) != OK) {
					UNPIN(rootID, CLEAN);
					return FAIL;
				}
                UNPIN(rootID, DIRTY);
            }

			// If not there isn't enough space, we need to split the root leaf and wrap it in an index node.
            else {
				PageID newPageID;
				KeyType newPageFirstKey;

				if (SplitLeafNode(key, rid, rootleaf, newPageID, newPageFirstKey) != OK){
					UNPIN (rootID, CLEAN);
					return FAIL;
				}

				PageID newIndexPageID;
				Page *newIndexPage;
                
				NEWPAGE(newIndexPageID, newIndexPage);
				
				// Create the new root index node and insert our duplicate key value into it
				BTIndexPage *rootindex = (BTIndexPage *) newIndexPage;
				rootindex->Init(newIndexPageID);
				rootindex->SetType(INDEX_NODE);
				rootindex->SetPrevPage(rootID);

				if (rootindex->Insert(newPageFirstKey, newPageID, newRecordID) != OK) {
					UNPIN(newIndexPageID, CLEAN);
					UNPIN(rootID, DIRTY);
					return FAIL;
				}
                header->SetRootPageID(newIndexPageID);
				UNPIN(rootID, DIRTY);
                UNPIN(newIndexPageID, DIRTY);
            }
        }
		else {
			// The root page is an index node so we will have to traverse through the tree to find the leaf page to insert on
            SortedPage* curPage = rootPage;
			PageID curIndexID;
			PageID nextPageID;
			Status s;

			RecordID curRecordID;
			KeyType curKey;
			PageID curPageID;

			// Traverse through the index nodes, pushing their pageIDs onto a stack so we can easily
			// move back up the tree in the event of a node split (and the need to insert a new index key)
			stack<PageID> indexIDStack;
            while (curPage->GetType() == INDEX_NODE) {
				BTIndexPage *curIndexPage = (BTIndexPage *) curPage;
				curIndexID = curIndexPage->PageNo();

				indexIDStack.push(curIndexID);

				
				// While our key to insert is greater than the key of the record we are current on,
				// continue searching the index for the right path to take (get the next record)
				nextPageID = curIndexPage->GetLeftLink();
				s = curIndexPage->GetFirst(curRecordID, curKey, curPageID);
				while(s != DONE && KeyCmp(key, curKey) >= 0) {
					nextPageID = curPageID;
					s = curIndexPage->GetNext(curRecordID, curKey, curPageID);
				}

				UNPIN(curIndexID, CLEAN);

				// Pin the page of the next node we will go to
				PIN(nextPageID, curPage);
            }

			// At this point we have reached the leaf node that we wish to insert on
			BTLeafPage *curLeafPage = (BTLeafPage *) curPage;
			PageID curLeafID = curLeafPage->PageNo();

			// If there is space on the leaf node to insert the record/key do so.
			if (curLeafPage->AvailableSpace() >= GetKeyDataLength(key, LEAF_NODE)) {
				if(curLeafPage->Insert(key, rid, newRecordID) != OK) {
					UNPIN(curLeafID, CLEAN);
					return FAIL;
				}
                UNPIN(curLeafID, DIRTY);
			}
			else {
				// Since there is not enough space to insert in the leaf node. We need to split it and update the index nodes
				PageID newPageID;
                KeyType newPageFirstKey;

				if (SplitLeafNode(key, rid, curLeafPage, newPageID, newPageFirstKey) != OK){
					UNPIN (curLeafID, CLEAN);
					return FAIL;
				}
				UNPIN(curLeafID, DIRTY);

				PageID tmpIndexID;
				BTIndexPage *tmpIndexPage;
                bool continuesplit = true;

                // Iterate through the stack containing the visited index nodes. 
				// Try to insert the key of the leftmost record of the new page created by the split
				// If necessary, split the index node and loop, now attempting to add the duplicate key
				// from the index split into the index node one level above
				char *indexKey = newPageFirstKey;
                while (continuesplit) {
					// Peek at the top of the stack and pin the index page
					tmpIndexID = indexIDStack.top();
					PIN(tmpIndexID, tmpIndexPage);

					// If there is enough space in this node to insert our key, do so and terminate the loop.
					if (tmpIndexPage->AvailableSpace() >= GetKeyDataLength(indexKey, INDEX_NODE)) {
						
						if (tmpIndexPage->Insert(indexKey, newPageID, newRecordID) != OK){
							UNPIN(tmpIndexID, CLEAN);
							return FAIL;
						}
						continuesplit = false;
						UNPIN(tmpIndexID, DIRTY);
					}
					// If there is not enough space, split the index node and loop with the desired insertion key set to
					// the (now deleted) leftmost entry that was returned from splitIndexNode()
					else {
						PageID newPageID2;
						KeyType newPageFirstKey2;
						if (SplitIndexNode(newPageFirstKey, newPageID, tmpIndexPage, newPageID2, newPageFirstKey2) != OK){
							UNPIN(tmpIndexID, CLEAN);
							return FAIL;
						}
						newPageID = newPageID2;
						indexKey = newPageFirstKey2;
						UNPIN(tmpIndexID, DIRTY);

						// Remove the processed indexNodePageID from the stack
						indexIDStack.pop();

						// If the stack is empty, create a new index node to wrap the nodes below and insert the key into
						// the newly created node. Terminate the loop after this since we are now at the root of the tree
						if (indexIDStack.empty()) {
							PageID newIndexPageID;
							Page *newPage;
                
							NEWPAGE(newIndexPageID, newPage);
				
							BTIndexPage *newIndexPage = (BTIndexPage *) newPage;
							newIndexPage->Init(newIndexPageID);
							newIndexPage->SetType(INDEX_NODE);
							newIndexPage->SetPrevPage(tmpIndexID);

							if (newIndexPage->Insert(indexKey, newPageID, newRecordID) != OK) {
								UNPIN(newIndexPageID, CLEAN);
								return FAIL;
							}

							header->SetRootPageID(newIndexPageID);
							UNPIN(newIndexPageID, DIRTY);
							continuesplit = false;
						}						
					}
                }               
			}
		}
	}

	return OK;
}



//-------------------------------------------------------------------
// BTreeFile::Delete
//
// Input   : key - pointer to the value of the key to be deleted.
//           rid - RecordID of the record to be deleted.
// Output  : None
// Return  : OK if successful, FAIL otherwise. 
// Purpose : Delete an entry with this rid and key.  
// Note    : If the root becomes empty, delete it.
//-------------------------------------------------------------------

Status BTreeFile::Delete (const char *key, const RecordID rid)
{
	if (header->GetRootPageID() == INVALID_PAGE) return FAIL;
		
	// A root page exists
	PageID rootID = header->GetRootPageID();
	SortedPage *rootPage;
	PIN(rootID, rootPage);

	// Check if the root is a leaf node (trivial case)
	if (rootPage->GetType() == LEAF_NODE) {
		BTLeafPage *curPage = (BTLeafPage *) rootPage;
		if (curPage->Delete(key,rid) != OK) {
			UNPIN(rootID, CLEAN);
			return FAIL;
		}

		// If the root page is now empty we need to delete it
		if (curPage->IsEmpty()){
			FREEPAGE(rootID);
			header->SetRootPageID(INVALID_PAGE);
		}
		else {
			UNPIN (rootID, DIRTY);
		}
	}
	else{
		// If the root is not a leaf node. We need to traverse the tree to find the leaf node to delete from
        SortedPage* curPage = rootPage;
		PageID curIndexID;
		PageID nextPageID;
		Status s;

		RecordID curRecordID;
		KeyType curKey;
		PageID curPageID;

		// For each visited index node, push it onto the stack (required from redistribution/merge extra credit - Not yet implemented)
		stack<PageID> indexIDStack;
        while (curPage->GetType() == INDEX_NODE) {
			BTIndexPage *curIndexPage = (BTIndexPage *) curPage;
			curIndexID = curIndexPage->PageNo();

			indexIDStack.push(curIndexID);

			nextPageID = curIndexPage->GetLeftLink();
			s = curIndexPage->GetFirst(curRecordID, curKey, curPageID);

			// While our key is greater than the key of the current record, continue searching the index for the right path to take
			// by getting the next record on the page
			while(s != DONE && KeyCmp(key, curKey) >= 0) {
				nextPageID = curPageID;
				s = curIndexPage->GetNext(curRecordID, curKey, curPageID);
			}
			
			UNPIN(curIndexID, CLEAN);

			// Pin the page of the next node to visit
			PIN(nextPageID, curPage);
        }

		BTLeafPage *curLeafPage = (BTLeafPage *) curPage;
		PageID curLeafID = curLeafPage->PageNo();

		// Check if this is the first key on the page. NOT NEEDEED ANYMORE, SEE PIAZZA POST @202
		RecordID curVal;
		if (curLeafPage->GetFirst(curRecordID, curKey, curVal) != OK) {
			std::cout << "GetFirst failed for pageNo" << curLeafPage->PageNo() << std::endl;
			UNPIN(curLeafID, CLEAN);
			return FAIL;
		}

		// THIS HAS BEEN REMOVED DUE TO PIAZZA POST @202
		if (false && KeyCmp(curKey, key) == 0 && curVal == rid) {
			// It is the first one
			// TODO: IMPLEMENT

			if (curLeafPage->Delete(key,rid) != OK) {

				UNPIN(curLeafID, CLEAN);
				return FAIL;
			}

			// If the page is now empty we need to delete it (and its entry) as long as it is not the leftmost link of an index node
			if (curPage->IsEmpty()){
				if (curLeafPage->GetPrevPage() != INVALID_PAGE){
				
				}
				else{
					// This is the leftmost page in the B+ tree, there is nothing to delete from the index
				}
				FREEPAGE(curLeafID);
			}
			else {
				UNPIN (rootID, DIRTY);
			}
		}
		else {
			// Simply delete the entry from the leaf page
			if (curLeafPage->Delete(key,rid) != OK) {
				std::cout << "Delete failed for thing with key= " << key << std::endl;
				UNPIN(curLeafID, CLEAN);
				return FAIL;
			}
			UNPIN (curLeafID, DIRTY);
		}
	}

	return OK;
}


//-------------------------------------------------------------------
// BTreeFile::OpenScan
//
// Input   : lowKey, highKey - pointer to keys, indicate the range
//                             to scan.
// Output  : None
// Return  : A pointer to IndexFileScan class.
// Purpose : Initialize a scan.  
// Note    : Usage of lowKey and highKey :
//
//           lowKey   highKey   range
//			 value	  value	
//           --------------------------------------------------
//           NULL     NULL      whole index
//           NULL     !NULL     minimum to highKey
//           !NULL    NULL      lowKey to maximum
//           !NULL    =lowKey   exact match (may not be unique)
//           !NULL    >lowKey   lowKey to highKey
//-------------------------------------------------------------------

IndexFileScan *BTreeFile::OpenScan (const char *lowKey, const char *highKey)
{
	
	BTreeFileScan *newScan = new BTreeFileScan();

	const char *searchKey;
	if (lowKey == NULL) {
		searchKey = "";
	}
	else {
		searchKey = lowKey;
	}

	PageID leftmostPageID;

	if (Search(searchKey,leftmostPageID) != OK) {
		leftmostPageID = INVALID_PAGE;
	}

	newScan->Init(lowKey, highKey, leftmostPageID);

	return newScan;
}



// Dump Following Statistics:
// 1. Total # of leafnodes, and Indexnodes.
// 2. Total # of dataEntries.
// 3. Total # of index Entries.
// 4. Fill factor of leaf nodes. avg. min. max.
Status BTreeFile::DumpStatistics() {	
	ostream& os = std::cout;
	float avgDataFillFactor, avgIndexFillFactor;

// initialization 
	hight = totalDataPages = totalIndexPages = totalNumIndex = totalNumData = 0;
	maxDataFillFactor = maxIndexFillFactor = 0; minDataFillFactor = minIndexFillFactor =1;
	totalFillData = totalFillIndex = 0;

	if(_DumpStatistics(header->GetRootPageID())== OK)
	{		// output result
		if (totalNumData == 0)
			maxDataFillFactor = minDataFillFactor = avgDataFillFactor = 0;
		else
			avgDataFillFactor = totalFillData/totalDataPages;
		if ( totalNumIndex == 0)
			maxIndexFillFactor = minIndexFillFactor = avgIndexFillFactor = 0;
		else 
			avgIndexFillFactor = totalFillIndex/totalIndexPages;
		os << "\n------------ Now dumping statistics of current B+ Tree!---------------" << endl;
		os << "  Total nodes are        : " << totalDataPages + totalIndexPages << " ( " << totalDataPages << " Data";
		os << "  , " << totalIndexPages <<" indexpages )" << endl;
		os << "  Total data entries are : " << totalNumData << endl;
		os << "  Total index entries are: " << totalNumIndex << endl;
		os << "  Hight of the tree is   : " << hight << endl;
		os << "  Average fill factors for leaf is : " << avgDataFillFactor<< endl;
		os << "  Maximum fill factors for leaf is : " << maxDataFillFactor;
		os << "	  Minumum fill factors for leaf is : " << minDataFillFactor << endl;
		os << "  Average fill factors for index is : " << 	avgIndexFillFactor << endl;
		os << "  Maximum fill factors for index is : " << maxIndexFillFactor;
		os << "	  Minumum fill factors for index is : " << minIndexFillFactor << endl;
		os << "  That's the end of dumping statistics." << endl;

		return OK;
	}
	return FAIL;
}

Status BTreeFile::_DumpStatistics(PageID pageID) { 
	__DumpStatistics(pageID);

	SortedPage *page;
	BTIndexPage *index;

	Status s;
	PageID curPageID;

	RecordID curRid;
	KeyType key;

	PIN (pageID, page);
	NodeType type = page->GetType ();

	switch (type) {
	case INDEX_NODE:
		index = (BTIndexPage *)page;
		curPageID = index->GetLeftLink();
		_DumpStatistics(curPageID);
		s=index->GetFirst(curRid, key, curPageID);
		if ( s == OK) {	
			_DumpStatistics(curPageID);
			s = index->GetNext(curRid, key, curPageID);
			while ( s != DONE) {	
				_DumpStatistics(curPageID);
				s = index->GetNext(curRid, key, curPageID);
			}
		}
		UNPIN(pageID, CLEAN);
		break;

	case LEAF_NODE:
		UNPIN(pageID, CLEAN);
		break;
	default:		
		assert (0);
	}

	return OK;
}

Status BTreeFile::__DumpStatistics (PageID pageID) {
	SortedPage *page;
	BTIndexPage *index;
	BTLeafPage *leaf;
	int i;
	Status s;
	PageID curPageID;
	float	curFillFactor;
	RecordID curRid;

	KeyType  key;
	RecordID dataRid;

	PIN (pageID, page);
	NodeType type = page->GetType ();
	i = 0;
	switch (type) {
	case INDEX_NODE:
		// add totalIndexPages
		totalIndexPages++;
		if ( hight <= 0) // still not reach the bottom
			hight--;
		index = (BTIndexPage *)page;
		curPageID = index->GetLeftLink();
		s=index->GetFirst (curRid , key, curPageID); 
		if ( s == OK) {	
			i++;
			s = index->GetNext(curRid, key, curPageID);
			while (s != DONE) {	
				i++;
				s = index->GetNext(curRid, key, curPageID);
			}
		}
		totalNumIndex  += i;
		curFillFactor = (float)(1.0 - 1.0*(index->AvailableSpace())/MAX_SPACE);
		if ( maxIndexFillFactor < curFillFactor)
			maxIndexFillFactor = curFillFactor;
		if ( minIndexFillFactor > curFillFactor)
			minIndexFillFactor = curFillFactor;
		totalFillIndex += curFillFactor;
		UNPIN(pageID, CLEAN);
		break;

	case LEAF_NODE:
		// when the first time reach a leaf, set hight
		if ( hight < 0)
			hight = -hight;
		totalDataPages++;

		leaf = (BTLeafPage *)page;
		s = leaf->GetFirst (curRid, key, dataRid);
		if (s == OK) {	
			s = leaf->GetNext(curRid, key, dataRid);
			i++;
			while ( s != DONE) {	
				i++;	
				s = leaf->GetNext(curRid, key, dataRid);
			}
		}
		totalNumData += i;
		curFillFactor = (float)(1.0 - 1.0*leaf->AvailableSpace()/MAX_SPACE);
		if ( maxDataFillFactor < curFillFactor)
			maxDataFillFactor = curFillFactor;
		if ( minDataFillFactor > curFillFactor)
			minDataFillFactor = curFillFactor;
		totalFillData += curFillFactor;
		UNPIN(pageID, CLEAN);
		break;
	default:		
		assert (0);
	}

	return OK;	
}

// function  BTreeFile::_SearchIndex
// PURPOSE	: given a IndexNode and key, find the PageID with the key in it
// INPUT	: key, a pointer to key;
//			: currIndexID, 
//			: curIndex, pointer to current BTIndexPage
// OUTPUT	: found PageID
Status BTreeFile::_SearchIndex (const char *key,  PageID currIndexID, BTIndexPage *currIndex, PageID& foundID)
{
	PageID nextPageID;
	
	Status s = currIndex->GetPageID (key, nextPageID);
	if (s != OK)
		return FAIL;
	
	// Now unpin the page, recurse and then pin it again
	
	UNPIN (currIndexID, CLEAN);
	s = _Search (key, nextPageID, foundID);
	if (s != OK)
		return FAIL;
	return OK;
}

// Function: 
// Input	: key, rid
// Output	: found Pid, where Key >= key
// Purpose	: find the leftmost leaf page contain the key, or bigger than the key
Status BTreeFile::_Search( const char *key,  PageID currID, PageID& foundID)
{
	
    SortedPage *page;
	Status s;
	
    PIN (currID, page);
    NodeType type = page->GetType ();
	
    // TWO CASES:
    // - pageType == INDEX:
    //   search the index
    // - pageType == LEAF_NODE:
    //   set the output page ID
	
    switch (type) 
	{
	case INDEX_NODE:
		s =	_SearchIndex(key,  currID, (BTIndexPage*)page, foundID);
		break;
		
	case LEAF_NODE:
		foundID =  page->PageNo();
		UNPIN(currID,CLEAN);
		break;
	default:		
		assert (0);
	}


	return OK;
}

// BTreeeFile:: Search
// PURPOSE	: find the PageNo of a give key
// INPUT	: key, pointer to a key
// OUTPUT	: foundPid

Status BTreeFile:: Search(const char *key,  PageID& foundPid)
{
	if (header->GetRootPageID() == INVALID_PAGE)
	{
		foundPid = INVALID_PAGE;
		return DONE;
		
	}

	Status s;
	
	s = _Search(key,  header->GetRootPageID(), foundPid);
	if (s != OK)
	{
		cerr << "Search FAIL in BTreeFile::Search\n";
		return FAIL;
	}

	return OK;
}

Status BTreeFile::_PrintTree ( PageID pageID)
{
	SortedPage *page;
	BTIndexPage *index;
	BTLeafPage *leaf;
	int i;
	Status s;
	PageID curPageID;

	RecordID curRid;

	KeyType  key;
	RecordID dataRid;

	ostream& os = cout;

    PIN (pageID, page);
    NodeType type = page->GetType ();
	i = 0;
      switch (type) 
	{
	case INDEX_NODE:
			index = (BTIndexPage *)page;
			curPageID = index->GetLeftLink();
			os << "\n---------------- Content of Index_Node-----   " << pageID <<endl;
			os << "\n Left most PageID:  "  << curPageID << endl;
			s=index->GetFirst (curRid , key, curPageID); 
			if ( s == OK)
			{	i++;
				os <<"Key: "<< key<< "	PageID: " 
					<< curPageID  << endl;
				s = index->GetNext(curRid, key, curPageID);
				while ( s != DONE)
				{	
					os <<"Key: "<< key<< "	PageID: " 
						<< curPageID  << endl;
					i++;
					s = index->GetNext(curRid, key, curPageID);

				}
			}
			os << "\n This page contains  " << i <<"  Entries!" << endl;
			UNPIN(pageID, CLEAN);
			break;
		
	case LEAF_NODE:
		leaf = (BTLeafPage *)page;
		s = leaf->GetFirst (curRid, key, dataRid);
			if ( s == OK)
			{	os << "\n Content of Leaf_Node"  << pageID << endl;
				os <<   "Key: "<< key<< "	DataRecordID: " 
					<< dataRid  << endl;
				s = leaf->GetNext(curRid, key, dataRid);
				i++;
				while ( s != DONE)
				{	
					os <<   "Key: "<< key<< "	DataRecordID: " 
						<< dataRid  << endl;
					i++;	
					s = leaf->GetNext(curRid, key, dataRid);
				}
			}
			os << "\n This page contains  " << i <<"  entries!" << endl;
			UNPIN(pageID, CLEAN);
			break;
	default:		
		assert (0);
	}
	//os.close();

	return OK;	
}

Status BTreeFile::PrintTree ( PageID pageID, PrintOption option)
{ 
	_PrintTree(pageID);
	if (option == SINGLE) return OK;

	SortedPage *page;
	BTIndexPage *index;

	Status s;
	PageID curPageID;

	RecordID curRid;
	KeyType  key;

    PIN (pageID, page);
    NodeType type = page->GetType ();
	
	switch (type) {
	case INDEX_NODE:
		index = (BTIndexPage *)page;
		curPageID = index->GetLeftLink();
		PrintTree(curPageID, RECURSIVE);
		s=index->GetFirst (curRid , key, curPageID);
		if ( s == OK)
		{	
			PrintTree(curPageID, RECURSIVE);
			s = index->GetNext(curRid, key, curPageID);
			while ( s != DONE)
			{	
				PrintTree(curPageID, RECURSIVE);
				s = index->GetNext(curRid, key, curPageID);
			}
		}
		UNPIN(pageID, CLEAN);
		break;

	case LEAF_NODE:
		UNPIN(pageID, CLEAN);
		break;
	default:		
		assert (0);
	}

	return OK;
}

Status BTreeFile::PrintWhole() {	
	ostream& os = cout;

	os << "\n\n------------------ Now Begin Printing a new whole B+ Tree -----------"<< endl;

	if(PrintTree(header->GetRootPageID(), RECURSIVE)== OK)
		return OK;
	return FAIL;
}


