#include "btleaf.h"


//-------------------------------------------------------------------
// BTLeafPage::InsertRecord
//
// Input   : key  - pointer to the key value to be inserted.
//           dataRid - record id to be associated with key
// Output  : rid - record id of the inserted pair (key, dataRid)
// Purpose : Insert the pair (key, dataRid) into this leaf node.
//-------------------------------------------------------------------

Status BTLeafPage::Insert(const char *key, 
						  RecordID dataRid, RecordID& rid)
{
	KeyDataEntry entry;
	int entryLen;
	
	DataType d;
	
	d.rid = dataRid;
	MakeEntry(&entry, key, (NodeType)type, d, &entryLen);
	//the data is packed into entry so that it can be inserted using SortedPage
	//MakeEntry is defined in key.cpp

	if (SortedPage::InsertRecord ((char *)&entry, entryLen, rid) != OK)
	{
		return FAIL;
	}
	
	return OK;
}


//-------------------------------------------------------------------
// BTLeafPage::GetFirst
//
// Input   : None
// Output  : rid - record id of the first entry
//           key - pointer to the key value
//           dataRid - pointer to the record id
// Purpose : get the first pair (key, dataRid) in the leaf page and 
//           it's rid.
// Return  : OK always.
//-------------------------------------------------------------------


Status BTLeafPage::GetFirst (RecordID& rid, char* key, RecordID & dataRid)
{
	rid.pageNo = pid;
	rid.slotNo = 0;
	
	if (numOfSlots == 0)
	{
		dataRid.pageNo = INVALID_PAGE;
		dataRid.slotNo = INVALID_SLOT;
		return DONE;
	}
	
	GetKeyData(key, 
		(DataType *)&dataRid, 
		(KeyDataEntry *)(data + slots[0].offset),
		slots[0].length,
		(NodeType)type);
	
	return OK;
}


//-------------------------------------------------------------------
// BTLeafPage::GetNext
//
// Input   : rid - record id of the current entry
// Output  : rid - record id of the next entry
//           key - pointer to the key value
//           dataRid - the record id
// Purpose : get the next pair (key, dataRid) in the leaf page and its
//           rid.
// Return  : OK if there is a next record, DONE if no more.
//-------------------------------------------------------------------


Status BTLeafPage::GetNext (RecordID& rid, char* key, RecordID & dataRid)
{
	rid.slotNo ++;
	
	if (rid.slotNo == numOfSlots)
	{
		dataRid.pageNo = INVALID_PAGE;
		dataRid.slotNo = INVALID_SLOT;
		return DONE;
	}
	
	GetKeyData(key, 
		(DataType *)&dataRid, 
		(KeyDataEntry *)(data + slots[rid.slotNo].offset),
		slots[rid.slotNo].length,
		(NodeType)type);
	
	return OK;
}


//-------------------------------------------------------------------
// BTLeafPage::GetCurrent
//
// Input   : rid - record id of the current entry
// Output  : key - pointer to the key value
//           dataRid - the record id
// Purpose : get the current pair (key, dataRid) in the leaf page and its
//           rid.
// Return  : OK always.
//-------------------------------------------------------------------

Status BTLeafPage::GetCurrent (RecordID rid, char* key, RecordID & dataRid)
{
	if (rid.slotNo == numOfSlots)
	{
		dataRid.pageNo = INVALID_PAGE;
		dataRid.slotNo = INVALID_SLOT;
		return DONE;
	}
	
	GetKeyData(key, 
		(DataType *)&dataRid, 
		(KeyDataEntry *)(data + slots[rid.slotNo].offset),
		slots[rid.slotNo].length,
		(NodeType)type);
	
	return OK;
}


//-------------------------------------------------------------------
// BTLeafPage::Delete
//
// Input   : key - pointer to the key
//           dataRid - record id
// Output  : None
// Purpose : Find the pair (key, dataRid) and delete it.
// Return  : OK if successful, FAIL otherwise.
//-------------------------------------------------------------------

Status BTLeafPage::Delete (const char* key, const RecordID& dataRid)
{
	int i;
	
	for (i = numOfSlots - 1; i >= 0; i--)
	{
		KeyType tmpKey;
		RecordID tmpRid;
		GetKeyData(
			tmpKey,
			(DataType *)&tmpRid,
			(KeyDataEntry *)(data + slots[i].offset),
			slots[i].length,
			(NodeType)type);
		if (tmpRid == dataRid && KeyCmp(key, tmpKey) == 0)
		{
			RecordID delRid;
			Status s;
			
			delRid.pageNo = PageNo();
			delRid.slotNo = i;
			s = SortedPage::DeleteRecord(delRid);
			assert(s == OK);
			return OK;
		}
	}
	
	return FAIL;
}
