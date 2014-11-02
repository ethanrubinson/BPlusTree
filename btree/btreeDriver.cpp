#include <cmath> 
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <ctime>
#include <vector>
#include <algorithm>

using namespace std;

#include "heapfile.h"
#include "bufmgr.h"
#include "db.h"
#include "btfile.h"
#include "btreeDriver.h"


void TestScanCount(int actualCount, int expectedCount) {
	if (actualCount != expectedCount) {
		cerr<<"Error: expected number of scanned items "<<expectedCount;
		exit(1);
	}
}

Status BTreeDriver::runTests() {
	Status status;

	int result;
	const int inTxtLen = 32;
	char *inputTxt = new char[inTxtLen];

	cout << "Input a space separated test sequence (ie. a list of numbers " << endl <<
		" in the range 0-7: 0 3 2 1) or hit ENTER to run all tests: ";

	cin.getline (inputTxt, inTxtLen);
	if (strlen(inputTxt) == 0) {
		inputTxt = "01234567";
	}
	
	minibase_globals = new SystemDefs(status, "BTREEDRIVER", "btlog", 1000, 500, 200, "Clock");
	if (status != OK) {
		cerr << "ERROR: Couldn'initialize the Minibase globals" << std::endl;
		minibase_errors.show_errors();

		cerr << "Hit [enter] to continue..." << endl;
		cin.get();
		exit(1);
	}

	for (int i = 0; i < (int)strlen(inputTxt); i++) {
		minibase_errors.clear_errors();
		switch (inputTxt[i]) {
		case '0' : 
			result = Test0();
			break;
		case '1' :
			result = Test1();
			break;
		case '2' :
			result = Test2();
			break;
		case '3' :
			result = Test3();
			break;	
		case '4':
			result = Test4();
			break;
		case '5':
			result = Test5();
			break;
		case '6':
			result = Test6();
			break;
		case '7':
			result = Test7();
			break;
		}

		if (!result || minibase_errors.error()) {
			status = FAIL;
			if ( minibase_errors.error() )
				cerr << (result ? "*** Unexpected error(s) logged, test failed:\n" : "Errors logged:\n");
			minibase_errors.show_errors(cerr);
		}
	}

	if (status != OK) {
		cout << "Error encountered during BTree Test: " << endl;
		minibase_errors.show_errors();
		return status;
	}

	delete minibase_globals;

	return OK;
}


//-----------------------------------------------------------------------------
// Test Cases
//-----------------------------------------------------------------------------

//	Test empty tree
bool BTreeDriver::Test0() {
	Status status = OK;
	BTreeFile *btf = NULL;
	bool res = true;

	btf = new BTreeFile(status, "TestEmptyTree");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;

	if (res) {
		std::cout << "Test 0 Passed!" << std::endl;
	}
	return res;
}

//	Test SinglePage, #1
bool BTreeDriver::Test1() {
	Status status = OK;
	BTreeFile *btf = NULL;
	bool res = true;

	btf = new BTreeFile(status, "TestSinglePage1");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	if (!InsertRange(btf, 1, 50)) {
		std::cerr << "InsertRange(1, 50) failed" << std::endl;
		res = false;
	}

	if (!TestNumLeafPages(btf, 1)) {
		std::cerr << "TestNumLeafPages(1) failed" << std::endl;
		res = false;
	}

	if (!TestNumEntries(btf, 50)) {
		std::cerr << "TestNumEntries(50) failed" << std::endl;
		res = false;
	}
	
	std::vector<int> expectedKeys;
	for (int i = 1; i <= 50; i++) {
		expectedKeys.push_back(i);
	}
	
	if (!TestScanKeys(btf, NULL, NULL, expectedKeys)) {
		std::cerr << "TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// We destroy and rebuild the file before to reduce
	// dependence on working Delete function.
	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;

	if (res) {
		std::cout << "Test 1 Passed!" << std::endl;
	}
	return res;
}

//	Test SinglePage, #2
bool BTreeDriver::Test2() {
	Status status = OK;
	BTreeFile *btf = NULL;
	bool res = true;

	btf = new BTreeFile(status, "TestSinglePage1");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	if (!InsertRange(btf, 1, 50)) {
		std::cerr << "InsertRange(1, 50) failed" << std::endl;
		res = false;
	}

	if (!DeleteStride(btf, 3, 48, 3)) {
		std::cerr << "DeleteStride(3, 48, 3) failed" << std::endl;
		res = false;
	}

	if (!InsertRange(btf, 101, 110)) {
		std::cerr << "InsertRange(101, 125) failed" << std::endl;
		res = false;
	}

	if (!TestNumLeafPages(btf, 1)) {
		std::cerr << "TestNumLeafPages(1) failed" << std::endl;
		res = false;
	}

	if (!TestNumEntries(btf, 44)) {
		std::cerr << "TestNumEntries(44) failed" << std::endl;
		res = false;
	}
	
	std::vector<int> expectedKeys;
	for (int i = 1; i <= 50; i++) 
		if (i % 3 != 0) 
			expectedKeys.push_back(i);
	for (int i = 101; i <= 110; i++)
		expectedKeys.push_back(i);
	
	if (!TestScanKeys(btf, NULL, NULL, expectedKeys)) {
		std::cerr << "TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	//	Try to delete some non-exist keys
	for (int i = 51; i <= 100; i++) {
		if (!DeleteKey(btf, i, 5, true)) 
			std::cerr << "DeleteKey() failed" << std::endl;
	}

	// We destroy and rebuild the file before to reduce
	// dependence on working Delete function.
	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;

	if (res) {
		std::cout << "Test 2 Passed!" << std::endl;
	}
	return res;
}

//	Test Insert with Leaf Split
bool BTreeDriver::Test3() {
	Status status = OK;
	BTreeFile *btf = NULL;
	bool res = true;

	btf = new BTreeFile(status, "TestInsertsWithLeafSplits1");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	//	Insert 31 long keys.
	if (!InsertRange(btf, 1, 31, 1, 20)) {
		std::cerr << "InsertRange(1, 31) failed" << std::endl;
		res = false;
	}

	if (!TestNumLeafPages(btf, 2)) {
		std::cerr << "TestNumLeafPages(btf, 2) failed" << std::endl;
		res = false;
	} 

	std::vector<int> expectedKeys;
	for (int i = 1; i <= 31; i++) {
		expectedKeys.push_back(i);
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 20)) {
		std::cerr << "TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// We destroy and rebuild the file before to reduce
	// dependence on working Delete function.
	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;
	if (res) {
		std::cout << "Test 3 Passed!" << std::endl;
	}
	return res;
}

//	Test Insert with Leaf Split and Delete with Merge
bool BTreeDriver::Test4() {
	Status status = OK;
	BTreeFile *btf = NULL;
	bool res = true;

	btf = new BTreeFile(status, "TestInsertsWithLeafSplits1");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	//	Similary, insert keys in [1, 31], but this time insert in a different ordering.
	if (!InsertRange(btf, 17, 31, 1, 20)) {
		std::cerr << "InsertRange(17, 31) failed" << std::endl;
		res = false;
	}
	if (!InsertRange(btf, 1, 15, 1, 20)) {
		std::cerr << "InsertRange(1, 15) failed" << std::endl;
		res = false;
	}
	if (!InsertRange(btf, 16, 16, 1, 20)) {
		std::cerr << "InsertRange(1, 15) failed" << std::endl;
		res = false;
	}

	if (!TestNumLeafPages(btf, 2)) {
		std::cerr << "TestNumLeafPages(btf, 2) failed" << std::endl;
		res = false;
	}

	//	Delete a few keys, so the leaves will be underflow.
	if (!DeleteStride(btf, 5, 30, 5, 20)) {
		std::cerr << "DeleteStride(5, 30, 5) failed" << std::endl;
	}
	//	After delete these keys, a merge should happen
	if (!TestNumLeafPages(btf, 1)) {
		std::cerr << "TestNumLeafPages(btf, 1) failed" << std::endl;
		res = false;
	}

	std::vector<int> expectedKeys;
	for (int i = 1; i <= 31; i++) {
		if (i % 5 != 0) 
			expectedKeys.push_back(i);
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 20)) {
		std::cerr << "TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// We destroy and rebuild the file before to reduce
	// dependence on working Delete function.
	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;
	if (res) {
		std::cout << "Test 4 Passed!" << std::endl;
	}
	return res;
}

//	Test insert with large workloads
bool BTreeDriver::Test5() {
	Status status;
	BTreeFile *btf;
	bool res = true;

	btf = new BTreeFile(status, "TestLargeWorkload");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	std::vector<int> expectedKeys;

	// Round 1
	if (!InsertRange(btf, 1001, 2000, 1, 5, true)) {
		std::cerr << "Round 1: InsertRange(1001, 2000) failed" << std::endl;
		res = false;
	}

	for (int i = 1001; i <= 2000; i++) {
		expectedKeys.push_back(i);
	}

	if (!TestNumEntries(btf, 1000)) {
		std::cerr << "Round 1: TestNumEntries(1000) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 1: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// Round 2
	if (!InsertRange(btf, 1, 1000, 1, 5, true)) {
		std::cerr << "Round 2: InsertRange(1, 1000) failed" << std::endl;
		res = false;
	}

	expectedKeys.clear();
	for (int i = 1; i <= 2000; i++) {
		expectedKeys.push_back(i);
	}

	if (!TestNumEntries(btf, 2000)) {
		std::cerr << "Round 2: TestNumEntries(2000) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 2: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// Round 3
	if (!InsertRange(btf, 5001, 6000, 1, 5, false)) {
		std::cerr << "Round 3: InsertRange(5001, 6000) failed" << std::endl;
		res = false;
	}

	for (int i = 5001; i <= 6000; i++)
		expectedKeys.push_back(i);

	if (!TestNumEntries(btf, 3000)) {
		std::cerr << "Round 3: TestNumEntries(3000) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 3: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// Round 4
	if (!InsertRange(btf, 2001, 4000, 1, 5, false)) {
		std::cerr << "Round 4: InsertRange(2001, 4000) failed" << std::endl;
		res = false;
	}

	for (int i = 2001; i <= 4000; i++) {
		expectedKeys.push_back(i);
	}
	sort(expectedKeys.begin(), expectedKeys.end());

	if (!TestNumEntries(btf, 5000)) {
		std::cerr << "Round 4: TestNumEntries(5000) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 4: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;

	if (res) {
		std::cout << "Test 5 Passed!" << std::endl;
	}
	return res;
}

//	Test insert/delete with large workloads
bool BTreeDriver::Test6() {
	Status status;
	BTreeFile *btf;
	bool res = true;

	btf = new BTreeFile(status, "TestLargeWorkload");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	std::vector<int> expectedKeys;

	// Round 1
	if (!InsertRange(btf, 1001, 2000, 1, 5, true)) {
		std::cerr << "Round 1: InsertRange(1001, 2000) failed" << std::endl;
		res = false;
	}

	for (int i = 1001; i <= 2000; i++) {
		expectedKeys.push_back(i);
	}

	if (!TestNumEntries(btf, 1000)) {
		std::cerr << "Round 1: TestNumEntries(1000) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 1: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	//	Round 2, Delete something.
	if (!DeleteStride(btf, 1002, 2000, 3, 5)) {
		std::cerr << "Round 2: DeleteStride(1002, 2000, 3) failed" << std:: endl;
	}

	expectedKeys.clear();
	for (int i = 1001; i <= 2000; i++) {
		if (i % 3 != 0)
			expectedKeys.push_back(i);
	}

	if (!TestNumEntries(btf, 667)) {
		std::cerr << "Round 2: TestNumEntries(667) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 2: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// Round 3
	if (!InsertRange(btf, 1, 1000, 1, 5, true)) {
		std::cerr << "Round 3: InsertRange(1, 1000) failed" << std::endl;
		res = false;
	}

	expectedKeys.clear();
	for (int i = 1; i <= 2000; i++) {
		if (i % 3 != 0 || i <= 1000)
			expectedKeys.push_back(i);
	}

	if (!TestNumEntries(btf, 1667)) {
		std::cerr << "Round 3: TestNumEntries(2000) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 3: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	// Round 4
	if (!InsertRange(btf, 5001, 6000, 1, 5, false)) {
		std::cerr << "Round 4: InsertRange(5001, 6000) failed" << std::endl;
		res = false;
	}

	for (int i = 5001; i <= 6000; i++)
		expectedKeys.push_back(i);

	if (!TestNumEntries(btf, 2667)) {
		std::cerr << "Round 4: TestNumEntries(2667) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 4: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	//	Round 5
	if (!DeleteStride(btf, 5010, 6000, 10, 5)) {
		std::cerr << "Round 5: DeleteStride(5010, 6000, 10) failed" << std::endl;
		res = false;
	}
	expectedKeys.clear();

	for (int i = 1; i <= 2000; i++) {
		if (i % 3 != 0 || i <= 1000)
			expectedKeys.push_back(i);
	}
	for (int i = 5001; i <= 6000; i++) {
		if (i % 10 != 0) 
			expectedKeys.push_back(i);
	}

	if (!TestNumEntries(btf, 2567)) {
		std::cerr << "Round 4: TestNumEntries(2567) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "Round 4: TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;

	if (res) {
		std::cout << "Test 6 Passed!" << std::endl;
	}
	return res;
}

//	Test scan
bool BTreeDriver::Test7() {
	Status status;
	BTreeFile *btf;
	bool res = true;

	btf = new BTreeFile(status, "TestLargeWorkload");

	if (status != OK) {
		std::cerr << "ERROR: Couldn't create a BTreeFile" << std::endl;
		minibase_errors.show_errors();

		std::cerr << "Hit [enter] to continue..." << std::endl;
		std::cin.get();
		exit(1);
	}

	std::vector<int> expectedKeys;

	if (!InsertRange(btf, 1001, 2000, 1, 5, true)) {
		std::cerr << "Round 1: InsertRange(1001, 2000) failed" << std::endl;
		res = false;
	}

	if (!InsertRange(btf, 1, 1000, 1, 5, true)) {
		std::cerr << "Round 2: InsertRange(1, 1000) failed" << std::endl;
		res = false;
	}

	if (!InsertRange(btf, 5001, 6000, 1, 5, false)) {
		std::cerr << "Round 3: InsertRange(5001, 6000) failed" << std::endl;
		res = false;
	}

	for (int i = 1; i <= 2000; i++) {
		expectedKeys.push_back(i);
	}
	for (int i = 5001; i <= 6000; i++)
		expectedKeys.push_back(i);

	if (!TestNumEntries(btf, 3000)) {
		std::cerr << "TestNumEntries(3000) failed" << std::endl;
		res = false;
	}

	if (!TestScanKeys(btf, NULL, NULL, expectedKeys, 5)) {
		std::cerr << "TestScanKeys(NULL, NULL) failed" << std::endl;
		res = false;
	}

	//	Fix the RNG generator seed
	srand(1234567);
	for (int i = 0; i < 100; i++) {
		if (!TestScanKeysRandomSubrange(btf, expectedKeys, 5)) {
			std::cerr << "Round " << i << ": TestScanKeysRandomSubrange() failed" << std::endl;
			res = false;
			break;
		}
	}

	if (btf->DestroyFile() != OK) {
		std::cerr << "Error destroying BTreeFile" << std::endl;
		res = false;
	}

	delete btf;

	if (res) {
		std::cout << "Test 7 Passed!" << std::endl;
	}
	return res;
}

//	Test Helper functions

//-------------------------------------------------------------------
// BTreeDriver::InsertRange
//
// Input   : btf,  The BTree to insert into.
//           low,  The beginning of the range (inclusive).
//           high, The end of the range (inclusive).
//           ridOffset, The offset used to compute the rid.
//           pad,  The amount of padding to use for the keys.
//           reverse, Whether to insert in reverse order.
// Output  : None
// Return  : True if this operation completed succesfully.
// Purpose : Inserts a range of keys into the B-Tree.
//-------------------------------------------------------------------
bool BTreeDriver::InsertRange(BTreeFile *btf, int low, int high,
							  int ridOffset, int pad, bool reverse) {
	int numKeys = high - low + 1;
	int keyNum;

	char skey[MAX_KEY_SIZE];

	int i;
	if (reverse) {
		i = numKeys - 1;
	} else {
		i = 0;
	}

	while (reverse && i >= 0 || !reverse && i < numKeys) {
		RecordID rid;
		keyNum = i+low;
		rid.pageNo=keyNum + ridOffset;
		rid.slotNo=keyNum + ridOffset + 1;
		BTreeDriver::toString(keyNum, skey, pad);

		if (btf->Insert(skey, rid) != OK) {
			std::cerr << "Insertion of range failed at key=" << skey
					  << " rid=" << rid << std::endl;
			minibase_errors.show_errors();
			return false;
		}

		if (reverse) {
			i--;
		} else {
			i++;
		}
	}

	return true;
}

//	Delete from BTree with a certain stride
bool BTreeDriver::DeleteStride(BTreeFile *btf, int low, int high,
							  int stride, int pad) {
	char lowKey[MAX_KEY_SIZE];
	char highKey[MAX_KEY_SIZE];
    BTreeDriver::toString(low, lowKey, pad);
    BTreeDriver::toString(high, highKey, pad);

	IndexFileScan* scan = btf->OpenScan(lowKey, highKey);
	RecordID rid;
	char curKey[MAX_KEY_SIZE];
	int count = 0;

	std::vector<std::pair<char*, RecordID>*> deleteVec;

	while(scan->GetNext(rid, curKey) != DONE) {
		char* tempKey = new char[MAX_KEY_SIZE];
		memcpy(tempKey, curKey, strlen(curKey) + 1);

		if(count == 0) {
			deleteVec.push_back(new std::pair<char*, RecordID>(tempKey, rid));
		}
		count = (count + 1)	% stride;
	}

	delete scan;

	int numToDelete = deleteVec.size();
	int numDeleted = 0;

	std::vector<std::pair<char*, RecordID>*>::iterator it;
	for(it = deleteVec.begin(); it != deleteVec.end(); it++) {
		std::pair<char*, RecordID>* pair = *it;
		if(btf->Delete(pair->first, pair->second) == OK) {
			numDeleted++;
		}
	}

	for(it = deleteVec.begin(); it != deleteVec.end(); it++) {
		delete [] (*it)->first;
		delete *it;
	}

	if(numDeleted != numToDelete) {
		std::cerr << "Error: Attempted to delete " << numToDelete
         	      << "pairs. Only able to delete " << numDeleted << std::endl;
		return false;
	}

	return true;

}

//-------------------------------------------------------------------
// BTreeDriver::TestNumLeafPages
//
// Input   : btf,  The BTree to test
//           expected,  The number of leaf pages you expect.
// Output  : None
// Return  : True if the number of leaf pages == expected.
// Purpose : Tests the number of leaf pages in the tree.
//-------------------------------------------------------------------
bool BTreeDriver::TestNumLeafPages(BTreeFile *btf, int expected)
{
	PageID pid = GetLeftmostLeaf(btf);

	if (pid == INVALID_PAGE) {
		std::cerr << "Unable to access left leaf" << std::endl;
		return false;
	}

	int numPages = 0;

	while (pid != INVALID_PAGE) {
		numPages++;

		SortedPage *page;
		if (MINIBASE_BM->PinPage(pid, (Page *&)page) == FAIL) {
			std::cerr << "Unable to pin page" << std::endl;
			return false;
		}

		pid = page->GetNextPage();

		if (MINIBASE_BM->UnpinPage(page->PageNo(), CLEAN) == FAIL) {
			std::cerr << "Unable to unpin page" << std::endl;
			return false;
		}
	}

	if (numPages != expected) {
		std::cerr << "Unexpected number of leaf pages. Expected " << expected
				  << ", but got " << numPages << std::endl;
		return false;
	}

	return true;
}

//	Get the leftmost leaf page in this index.
PageID BTreeDriver::GetLeftmostLeaf(BTreeFile *btf) {
	PageID curPid = btf->header->GetRootPageID();

	while (curPid != INVALID_PAGE) {
		SortedPage *curPage;

		if (MINIBASE_BM->PinPage(curPid, (Page *&)curPage) == FAIL) {
			std::cerr << "Unable to pin page" << std::endl;
			return INVALID_PAGE;
		}
	
		NodeType nodeType = curPage->GetType();

		if (nodeType == LEAF_NODE) {
			//	If we have reached a leaf page, then we are done.
			break;
		} else {
			//	Traverse down to the leftmost branch
			PageID tempPid = curPage->GetPrevPage();

			if (MINIBASE_BM->UnpinPage(curPid, CLEAN) == FAIL) {
				std::cerr << "Unable to unpin page" << std::endl;
				return INVALID_PAGE;
			}

			curPid = tempPid;
		}
	}

	if (curPid != INVALID_PAGE) {
		if (MINIBASE_BM->UnpinPage(curPid, CLEAN) == FAIL) {
			std::cerr << "Unable to unpin page" << std::endl;
			return INVALID_PAGE;
		}
	}
	return curPid;
}

//-------------------------------------------------------------------
// BTreeDriver::TestNumEntries
//
// Input   : btf,  The B-Tree to test.
//           expected,  The expected number of elements in the tree.
// Output  : None
// Return  : True if the number of elements == expected.
// Purpose : Tests the number of elements in a tree.
//-------------------------------------------------------------------
bool BTreeDriver::TestNumEntries(BTreeFile *btf, int expected)
{
	IndexFileScan *scan = btf->OpenScan(NULL, NULL);
	bool test = TestScanCount(scan, expected);
	delete scan;
	return test;
}

bool BTreeDriver::TestScanCount(IndexFileScan* scan, int expected) {
	RecordID rid;
	char prevKey[MAX_KEY_SIZE] = "";
	char curKey[MAX_KEY_SIZE];
	int numEntries = 0;

	while (scan->GetNext(rid, curKey) != DONE) {
		if (strcmp(prevKey, curKey) > 0) {
			std::cerr << "Error: Keys are not sorted. Saw"
					  << prevKey << " before " << curKey << std::endl;
			return false;
		}

		strcpy(prevKey, curKey);
		numEntries++;
	}

	if (numEntries != expected) {
		std::cerr << "Unexpected number of entries in scan. Expected " << expected
				  << " Got " << numEntries << std::endl;
		return false;
	}

	return true;
}

bool BTreeDriver::TestScanKeys(BTreeFile *btf, const char *lowKey, const char *highKey, const std::vector<int> &keys, int pad)
{
	IndexFileScan *scan = btf->OpenScan(lowKey, highKey);

	if (scan == NULL) {
		std::cerr << "Error opening scan." << std::endl;
		return false;
	}
	
	char expectedKey[MAX_KEY_SIZE];
	bool ret = false;

	char curKey[MAX_KEY_SIZE];
	RecordID curRid;
	unsigned int index = 0;

	std::vector<int> expectedKeyVec;
	std::vector<int>::const_iterator iter;
	for (iter = keys.begin(); iter != keys.end(); iter++) {
		BTreeDriver::toString(*iter, expectedKey, pad);

		if (lowKey == NULL || strcmp(lowKey, expectedKey) <= 0) {
			if (highKey == NULL || strcmp(expectedKey, highKey) <= 0) {
				expectedKeyVec.push_back(*iter);
			}
		}
	}

	while (scan->GetNext(curRid, curKey) != DONE) {
		if (index >= expectedKeyVec.size()) {
			std::cerr << "Bailing with index = " << index << ", expectedKeyVec.size() = " << expectedKeyVec.size() << std::endl;
			std::cerr << "lowKey = \"" << (lowKey == NULL ? "NULL" : lowKey) << "\"" << std::endl;
			std::cerr << "highKey = \"" << (highKey == NULL ? "NULL" : highKey) << "\"" << std::endl;
			std::cerr << "curKey = " << curKey << std::endl;
			delete scan;
			return false;
		}
		
		BTreeDriver::toString(expectedKeyVec[index], expectedKey, pad);
		if (strcmp(curKey, expectedKey) != 0) {
			delete scan;
			return false;
		}

		index++;
	}

	delete scan;

	if (index != expectedKeyVec.size()) {
		return false;
	}

	return true;
}

bool BTreeDriver::TestScanKeysRandomSubrange(BTreeFile *btf,
						const std::vector<int> &keys,
						int pad) {
	char lowScanKeyBuffer[MAX_KEY_SIZE];
	char highScanKeyBuffer[MAX_KEY_SIZE];

	char *lowScanKey = NULL;
	char *highScanKey = NULL;

	int lowScanKeyIndex = 0, highScanKeyIndex = -1;

	if (keys.size() > 0) {
		if (rand() % 5 != 0) {
			lowScanKeyIndex = rand() % keys.size();
			BTreeDriver::toString(keys[lowScanKeyIndex], lowScanKeyBuffer, pad);
			lowScanKey = lowScanKeyBuffer;
		}

		if (rand() % 5 != 0) {
			highScanKeyIndex = rand() % (keys.size() - lowScanKeyIndex);
			highScanKeyIndex += lowScanKeyIndex;
			BTreeDriver::toString(keys[highScanKeyIndex], highScanKeyBuffer, pad);
			highScanKey = highScanKeyBuffer;
		}
	}

	return TestScanKeys(btf, lowScanKey, highScanKey, keys, pad);
}

//	Helper function to insert a key
bool BTreeDriver::InsertKey(BTreeFile *btf, int key, int pad)
{
	char skey[MAX_KEY_SIZE];
	BTreeDriver::toString(key, skey, pad);
	RecordID rid;
	rid.pageNo=key;
	rid.slotNo=key + 1;

	if (btf->Insert(skey, rid) != OK) {
		std::cerr << "Inserting key " << skey << " failed"  << std::endl;
		return false;
	}

	return true;
}

//	Helper function to delete a key
bool BTreeDriver::DeleteKey(BTreeFile *btf, int key, int pad, bool expectedFail)
{
	char skey[MAX_KEY_SIZE];
	BTreeDriver::toString(key, skey, pad);
	RecordID rid;
	rid.pageNo=key;
	rid.slotNo=key + 1;

	if (!expectedFail) {
		if (btf->Delete(skey, rid) != OK) {
			std::cerr << "Deleting key " << skey << " failed" << std::endl;
			return false;
		}
	} else {
		if (btf->Delete(skey, rid) == OK) {
			std::cerr << "Deleting non-exist key " << skey << " successfully" << std::endl;
			return false;
		}
	}

	return true;
}

//-------------------------------------------------------------------
// BTreeDriver::toString
//
// Input   : n,   The number to convert
//           pad, The number of digits to use. Should be larger than the
//                number of digits in n. Leading digits will be 0.
// Output  : str, The converted number. This function assumes that str is
//                a character array of size MAX_KEY_LENGTH.
// Return  : None
// Purpose : Converts a number of a strings so that it can be used as a key.
//-------------------------------------------------------------------
void BTreeDriver::toString(const int n, char *str, int pad)
{
	char format[200];
	sprintf_s(format, 200, "%%0%dd", pad);
	sprintf_s(str, MAX_KEY_SIZE, format, n);
}



