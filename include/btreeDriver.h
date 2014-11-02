#ifndef _B_TREE_DRIVER_H_
#define _B_TREE_DRIVER_H_

#include "btfile.h"
#include "index.h"
#include <vector>

const int BTREE_DEFAULT_PAD = 4;
const int BTREE_DEFAULT_RID_OFFSET = 1;


class BTreeDriver {

public:

	Status runTests();

private:
	static void toString(const int n, char *str, int pad = BTREE_DEFAULT_PAD);
	static bool InsertRange(BTreeFile *btf, int low, int high,
							int ridOffset = BTREE_DEFAULT_RID_OFFSET,
							int pad = BTREE_DEFAULT_PAD,
							bool reverse = false);
	static bool DeleteStride(BTreeFile *btf, int low, int high, int stride,
 							 int pad = BTREE_DEFAULT_PAD);
	static bool InsertKey(BTreeFile *btf, int key, int pad);
	static bool DeleteKey(BTreeFile *btf, int key, int pad, bool expectedFail);

	static bool TestNumLeafPages(BTreeFile *btf, int expected);
	static bool TestNumEntries(BTreeFile *btf, int expected);
	static bool TestScanKeys(BTreeFile *btf,
						     const char *lowKey, const char *highKey,
							 const std::vector<int> &keys,
							 int pad = BTREE_DEFAULT_PAD);
	static bool TestScanKeysRandomSubrange(BTreeFile *btf,
										   const std::vector<int> &keys,
										   int pad);

	static PageID GetLeftmostLeaf(BTreeFile *btf);

	static bool TestScanCount(IndexFileScan* scan, int expected);



	bool Test0();
	bool Test1();
	bool Test2();
	bool Test3();
	bool Test4();
	bool Test5();
	bool Test6();
	bool Test7();
};


#endif
