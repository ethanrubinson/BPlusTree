#include "btfile.h"
#include "index.h"
#include <set>
using namespace std;

class BTreeTest {
	set<int> old_keys;
	int nEntered; 
public:
	BTreeTest() : nEntered(0) { 
		old_keys = set<int>();
	}
	Status RunTests(istream &in);
	BTreeFile *createIndex(char *name);
	void destroyIndex(BTreeFile *btf, char *name);
	void insertHighLow(BTreeFile *btf, int low, int high);
	void scanHighLow(BTreeFile *btf, int low, int high);
	void deleteHighLow(BTreeFile *btf, int low, int high);
};







