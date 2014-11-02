#include <stdlib.h>
#include <iostream>
using namespace std;


#include "btreetest.h"
#include "btreeDriver.h"

int MINIBASE_RESTART_FLAG = 0;

void printHelp() {
	cout << "Commands should be of the form:"<<endl;
	cout << "insert <low> <high>"<<endl;
	cout << "scan <low> <high>"<<endl;
	cout << "delete <low> <high>"<<endl;
	cout << "print"<<endl;
	cout << "stats"<<endl;
	cout << "quit (not required)"<<endl;
	cout << "Note that (<low>==-1)=>min and (<high>==-1)=>max"<<endl;

}

int btreeTestManual() {
	cout << "btree tester with manual input" <<endl;
	printHelp();

	BTreeTest btt;
	Status dbstatus = btt.RunTests(cin);

	if (dbstatus != OK) {       
		cout << "Error encountered during btree tests: " << endl;
		minibase_errors.show_errors();      
		return 1;
	}
	return 0;
}

int btreeTestAutomatic() {
	cout << "btree tester with automatic test cases" <<endl;

	BTreeDriver btt;

	Status dbstatus = btt.runTests();

	if (dbstatus != OK) {       
		cout << "Error encountered during btree tests: " << endl;
		minibase_errors.show_errors();      
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	std::cout << "Please choose the test mode: " << std::endl;
	std::cout << "Hit [Enter] for automatic test" << std::endl;
	std::cout << "Type 'man' for manual test" << std::endl;

	char testMode[20];
	std::cin.getline(testMode, 20);
	int ret = -1;
	if (strcmp(testMode, "") == 0) {
		//	Automatic test
		ret = btreeTestAutomatic();
	} else if (strcmp(testMode, "MAN") == 0 || strcmp(testMode, "man") == 0) {
		//	Manual test
		ret = btreeTestManual();
	} else {
		std::cout << "Unrecognized test mode: " << testMode << std::endl;
		std::cout << "Please type [Enter] or 'man'" << std::endl;
	
	}
	
	std::cout << "Hit [enter] to continue..." << endl;
	std::cin.get();

	return ret;
}

