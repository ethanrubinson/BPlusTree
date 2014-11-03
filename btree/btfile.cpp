#include "minirel.h"
#include "bufmgr.h"
#include "db.h"
#include "new_error.h"
#include "btfile.h"
#include "btfilescan.h"

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
		//Allocate a new header page.
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
	//TODO: add your code here
	return FAIL;
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
	//TODO: add your code here
	return FAIL;
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
	//TODO: add your code here
	return NULL;
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


