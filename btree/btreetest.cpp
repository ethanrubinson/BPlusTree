#include <cmath> 
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cassert>
using namespace std;

#include "bufmgr.h"
#include "db.h"
#include "btfile.h"

#include "btreetest.h"

#define MAX_COMMAND_SIZE 1000

#define MAX_INT_LENGTH 5

void toString(const int n, char* str) {
	sprintf(str, "%04d", n);
}

Status BTreeTest::RunTests(istream &in) {

	char *dbname="btdb";
	char *logname="btlog";
	char *btfname="BTreeIndex";

	remove(dbname);
	remove(logname);

	Status status;
	minibase_globals = new SystemDefs(status, dbname, logname, 1000,500,200);
	if (status != OK) {
		minibase_errors.show_errors();
		exit(1);
	}
	
	BTreeFile* btf=createIndex(btfname);
	if (btf==NULL) {
		cout << "Error: Unable to create index file"<< endl;
	}

	char command[MAX_COMMAND_SIZE];
	in >> command;
	while(in) {
		if(!strcmp(command, "insert")) {
			int high, low;
			in >> low >> high ;
			insertHighLow(btf,low,high);
		}
		else if(!strcmp(command, "scan")) {
			int high, low;
			in >> low >> high;
			scanHighLow(btf,low,high);
		}
		else if(!strcmp(command, "delete")) {
			int high, low;
			in >> low >> high;
			deleteHighLow(btf,low,high);
		}
		else if(!strcmp(command, "print")) {
			btf->PrintWhole();
		}
		else if(!strcmp(command, "stats")) {
			btf->DumpStatistics();
		}
		else if(!strcmp(command, "quit")) {
			break;
		}
		else {
			cout << "Error: Unrecognized command: "<<command<<endl;
		}
		in >> command;
	}

	destroyIndex(btf, btfname);

	delete minibase_globals;
	remove(dbname);
	remove(logname);

	return OK;
}


BTreeFile *BTreeTest::createIndex(char *name) {
    cout << "Create B+tree." << endl;
    cout << "  Page size="<<MINIBASE_PAGESIZE<< " Max space="<<MAX_SPACE<<endl;
	
    Status status;
    BTreeFile *btf = new BTreeFile(status, name);
    if (status != OK) {
        minibase_errors.show_errors();
        cout << "  Error: can not open index file."<<endl;
		if (btf!=NULL)
			delete btf;
		return NULL;
    }
    cout<<"  Success."<<endl;
	return btf;
}


void BTreeTest::destroyIndex(BTreeFile *btf, char *name) {
	cout << "Destroy B+tree."<<endl;
	Status status = btf->DestroyFile();
	if (status != OK)
		minibase_errors.show_errors();
    delete btf;
}

void BTreeTest::insertHighLow(BTreeFile *btf, int low, int high) {
	
	int numkey=high-low+1, key;
	cout << "Inserting: ("<<low<<" to "<<high<<")"<<endl;

	char skey[MAX_INT_LENGTH];
    for (int i=0; i<numkey; i++) {
		RecordID rid;
        rid.pageNo=i; rid.slotNo=i+1;
		
		key = i+low;
		
		if(old_keys.find(key) != old_keys.end()) {
			// This is a duplicate report error
			cout << "Duplicate Insert Request Denied \n\t" <<  __FILE__ 
				<< ":"<<  __LINE__ << endl;
		} else {
			cout << "  Insert: "<<key<<" @[pg,slot]=["<<rid.pageNo<<","
				<<rid.slotNo<<"]"<<endl;

			toString(key, skey);
			if (btf->Insert(skey, rid) != OK) {
				cout << "  Insertion failed."<< endl;
				minibase_errors.show_errors();
				return;
			}
		}
		old_keys.insert(key);
    }
	
	cout << "  Success."<<endl;
}


void BTreeTest::scanHighLow(BTreeFile *btf, int low, int high) {
	
	cout << "Scanning ("<<low<<" to "<<high<<"):"<<endl;

	char strLow[MAX_INT_LENGTH], strHigh[MAX_INT_LENGTH];
	toString(low, strLow);
	toString(high, strHigh);
	IndexFileScan *scan = btf->OpenScan(strLow, strHigh);

	if(scan == NULL) {
		cout << "  Error: cannot open a scan." << endl;
		minibase_errors.show_errors();
		return;
	}
	
	RecordID rid;
	int count=0;
	char skey[MAX_INT_LENGTH];
	Status status = scan->GetNext(rid, skey);
	while (status == OK) {
		count++;
		cout<<"  Scanned @[pg,slot]=["<<rid.pageNo<<","<<rid.slotNo<<"]";
		cout<<" key="<<skey<<endl;
		status = scan->GetNext(rid, skey);
	}
	delete scan;
	cout << "  "<< count << " records found."<<endl;

	if (status!=DONE) {
		minibase_errors.show_errors();
		return;
	}
	cout << "  Success."<<endl;
}


void BTreeTest::deleteHighLow(BTreeFile *btf, int low, int high) {
	cout << "Deleting ("<<low<<"-"<<high<<"):"<<endl;

	char strLow[MAX_INT_LENGTH], strHigh[MAX_INT_LENGTH];
	toString(low, strLow);
	toString(high, strHigh);
	IndexFileScan *scan = btf->OpenScan(strLow, strHigh);

	if(scan == NULL) {
		cout << "Error: cannot open a scan." << endl;
		minibase_errors.show_errors();
		return;
	}
	RecordID rid;
	int count=0;
	char skey[MAX_INT_LENGTH];
	Status status = scan->GetNext(rid, skey);
    while (status == OK) {
		cout << "  Delete [pg,slot]=["<<rid.pageNo<<","<<rid.slotNo<<"]";
		cout << " key="<<skey<<endl;
		delete scan;
		scan=NULL;
		if ((status = btf->Delete(skey,rid)) != OK) {
			cout << "  Failure to delete record...\n";
			minibase_errors.show_errors();
			break;
		}
		count++;

		int ikey = atoi(skey);
		old_keys.erase(ikey);

		//scan = btf->OpenScan(plow, phigh);
		scan = btf->OpenScan(strLow, strHigh);
		if(scan == NULL) {
			cout << "Error: cannot open a scan." << endl;
			minibase_errors.show_errors();
			break;
		}
		status = scan->GetNext(rid, skey);
    }
	delete scan;
	cout << "  " << count << " records deleted."<<endl;

    if (status != DONE) {
		cout << "  Error: During delete";
		minibase_errors.show_errors();
		return;
    }

	cout << "  Success."<<endl;
}

