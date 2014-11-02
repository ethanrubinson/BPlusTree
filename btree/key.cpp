/*
 * key.C - implementation of <key,data> abstraction for BT*Page and 
 *         BTreeFile code.
 *
 * Gideon Glass & Johannes Gehrke  951016  CS564  UW-Madison
 * Cleaned up by Wei Tsang Ooi 970301  CS432  Cornell 
*/

#include <string.h>
#include <assert.h>

#include "bt.h"



//-------------------------------------------------------------------
// KeyCmp
//
// Input   : key1, key2 - pointer to two key to compare.
//           t - type of attribute key1 and key2
// Output  : None
// Purpose : Compare the value of two keys
// Return  : 
//   - key1  < key2 : negative
//   - key1 == key2 : 0
//   - key1  > key2 : positive
//-------------------------------------------------------------------


int KeyCmp(const char *key1, const char *key2)
{
	return strncmp(key1, key2, MAX_KEY_SIZE);
}

//-------------------------------------------------------------------
// GetKeyLength
//
// Input   : key - key we are interested in.
// Output  : None
// Purpose : Return the size of key
// Return  : The size of the key.
//-------------------------------------------------------------------

int GetKeyLength(const char *key)
{
	int len;
	
	len = strlen((char *) key); 
	return len+1;
}


//-------------------------------------------------------------------
// GetKeyDataLength
//
// Input   : key - key we are interested in.
//           nodeType - the type of the node (INDEX or LEAF)
// Output  : None
// Purpose : Return the size of key and data.
// Return  : The size of the key and data.
//-------------------------------------------------------------------

int GetKeyDataLength(const char *key, const NodeType nodeType)
{
	int keylen = GetKeyLength (key);

	switch(nodeType) 
	{
	
	case INDEX_NODE:
		return keylen + sizeof(PageID);
	
	case LEAF_NODE:
		return keylen + sizeof(RecordID);
	
	default:  // sanity check
		assert(0);
	}
	
	assert(0);
	return 0;
}


//-------------------------------------------------------------------
// FillEntryKey
//
// Write the key part of a (key, data) pair.  Set keyLen to the length
// of the key.
//
// Assume for key of type string, there is enough space.
//-------------------------------------------------------------------

static void FillEntryKey(KeyType *target, const char *key, 
                         int *keyLen)
{
	char *p = (char *) target;
	int len = strlen((char *)key)+1;
	if (len >= MAX_KEY_SIZE) {
		cerr<<"error: string key length exceeds maximum"<<endl;
		exit(1);
	}
	strcpy(p, (char *) key);
	*keyLen = len;
	return;
}


//-------------------------------------------------------------------
// FillEntryData
//
// Write the data part of a (key, data) pair.  Set dataLen to the 
// length of the key.
//
// Note that these do memcpy's instead of direct assignments because 
// target' may not be properly aligned.
//-------------------------------------------------------------------

static void FillEntryData(char *target, DataType source, NodeType nodeType,
                          int *dataLen)
{
	switch(nodeType) {

	case INDEX_NODE:
		{
			DataType src = source;
			memcpy(target, &src, sizeof(PageID));
			*dataLen = sizeof(PageID);
			return;
		}
	case LEAF_NODE:
		{
			DataType src = source;
			memcpy((char*)target, (char*)(&source), sizeof(RecordID));      
			*dataLen = sizeof(RecordID);
			return;
		}
	default:
		assert(0); // internal error
	}
}

//-------------------------------------------------------------------
// MakeEntry
//
// Input   : target   - pointer to a location in mem where entry is to 
//                      be created.
//           key      - pointer to the key.
//           nodeType - type of the B+-tree node where the entry is to
//                      be created.
//           data     - data to be inserted into the entry.
// Output  : len      - length of the entry created.
// Purpose : Create an entry (key, data) in location target.
// Precond : target is big enough to hold the created entry.
//-------------------------------------------------------------------


void MakeEntry (KeyDataEntry *target,
                const char *key,
                NodeType nodeType, DataType data,
                int *len)
{
	int keyLen, dataLen;
	char *c;
	
	FillEntryKey (&target->key, key, &keyLen);
	
	// below we can't say "&target->data" because <data> field may actually
	// start before that location (recall that KeyDataEntry is simply 
	// a chunk of memory big enough to hold any legal <key,data> pair).
	c = (char *)target+keyLen;
	FillEntryData ((char *) (((char *)target) + keyLen),
		data, nodeType, &dataLen);         
	*len = keyLen + dataLen;
}


//-------------------------------------------------------------------
// GetKeyData
//
// Input   : key      - pointer to mem area where the key is to be
//                      copied. (NULL if we are not interested in
//                      key)
//           data     - pointer to mem area where data is to be copied
//                      (NULL if we are not interested in data)
//           pair     - pointer to a (key, data) pair.
//           len      - length (num of bytes) of the (key, data) pair.
//           nodeType - type of the B+-tree node where the entry is in.
// Output  : None
// Purpose : Extract the key and data from an (key, data) pair.
//-------------------------------------------------------------------

void GetKeyData (char *key, DataType *data, KeyDataEntry *pair, int len, NodeType nodeType)
{
	int dataLen;
	int keyLen;
	
	switch(nodeType) 
	{
	
	case INDEX_NODE:
		dataLen = sizeof(PageID);
		break;
	
	case LEAF_NODE:
		dataLen = sizeof(RecordID);
		break;
	
	default:
		assert(0);
	}
	
	keyLen = len - dataLen;

	if (key)
		memcpy(key, pair, keyLen);

	if (data)
		memcpy(data, ((char*)pair) + keyLen, dataLen);
}
