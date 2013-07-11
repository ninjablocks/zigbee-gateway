#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "hal_types.h"
#include "SimpleDB.h"

#define TEMP_FILENAME_EXTENTION_LENGTH 4

int sdbErrno;

typedef struct
{
	char name[MAX_SUPPORTED_FILENAME + TEMP_FILENAME_EXTENTION_LENGTH + 1];
	FILE * file;
	long last_accessed_record_start_file_pointer;
	uint32_t last_accessed_record_size;
	get_record_size_f get_record_size;
	check_deleted_f check_deleted;
	check_ignore_f check_ignore;
	mark_deleted_f mark_deleted;
	consolidation_processing_f consolidation_processing;
	uint8_t type;
	uint32_t bin_header_size;
} _db_descriptor;

db_descriptor * sdb_init_db(char * name, get_record_size_f get_record_size, check_deleted_f check_deleted, check_ignore_f check_ignore, mark_deleted_f mark_deleted, consolidation_processing_f consolidation_processing, uint8_t db_type, uint32_t db_bin_header_size)
{
	_db_descriptor * db;
	int abort = FALSE;

	db = malloc(sizeof(_db_descriptor));

	if (db != NULL)
	{
		if (strlen(name) > MAX_SUPPORTED_FILENAME)
		{
			abort = TRUE;
		}
		else
		{
			strcpy(db->name, name);
			
			db->file = fopen(name,(db_type == SDB_TYPE_TEXT) ? "r+t" : "r+b");
			if ((db->file == NULL) && (errno == ENOENT))
			{
				db->file = fopen(name,(db_type == SDB_TYPE_TEXT) ? "w+t" : "w+b");
			}

			if (db->file == NULL)
			{
				abort = TRUE;
			}
			else
			{
				db->last_accessed_record_start_file_pointer = 0;
				db->last_accessed_record_size = 0;
				db->get_record_size = get_record_size;
				db->check_deleted = check_deleted;
				db->check_ignore = check_ignore;
				db->mark_deleted = mark_deleted;
				db->consolidation_processing = consolidation_processing;
				db->type = db_type;
				db->bin_header_size = db_bin_header_size; //used only for binary-type databases.
			}
		}
	}

	if (abort)
	{
		free(db);
		db = NULL;
	}

	return (db_descriptor *)db;
}

bool sdb_release_db(db_descriptor ** _db)
{
	_db_descriptor * db = *_db;
	
	if (db != NULL)
	{
		fclose(db->file);
		free(db);
		*_db = NULL;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void sdb_flush_db(db_descriptor * db)
{
	// do nothing.
	// In the current implementation,  flushing is done automatically after every change
}

bool sdb_release_record(void ** record)
{
	// In the current implementation, the retrieved record is statically allocated, so no need to release it.
	*record = NULL;
	return TRUE;
}

bool sdb_add_record(db_descriptor * _db, void * rec)
{
	_db_descriptor * db = _db;

	if (fseek(db->file, 0, SEEK_END) != 0)
	{
		return FALSE;
	}
	
	db->last_accessed_record_start_file_pointer = ftell(db->file);
	db->last_accessed_record_size = db->get_record_size(rec);
	
	return ((fwrite(rec, db->get_record_size(rec), 1, db->file) == 1) &&
	   (fflush(db->file) == 0));
}

bool sdb_modify_last_accessed_record(db_descriptor * _db, void * record)
{
	_db_descriptor * db = _db;

	if ((db->get_record_size(record) != db->last_accessed_record_size)||
	   ((fseek(db->file, db->last_accessed_record_start_file_pointer, SEEK_SET) != 0) ||
	   (fwrite(record, db->last_accessed_record_size, 1, db->file) != 1) ||
	   (fflush(db->file) != 0)))
	{
		return FALSE;
	}

	return TRUE;	
}


void * sdb_delete_record(db_descriptor * _db, void * key, check_key_f check_key)
{
	_db_descriptor * db = _db;
	void * rec;
	uint32_t size;

	sdbErrno = 0;
	
	rec = sdb_get_record(db, key, check_key, NULL);

	if (rec != NULL)
	{

		size = db->get_record_size(rec);
		db->mark_deleted(rec);

		if (!sdb_modify_last_accessed_record(db, rec))
		{
			sdbErrno = 1;
			rec = NULL;
		}
	}
	
	return rec;
}

bool sdb_rename_db(db_descriptor * _db, char * newName)
{
	_db_descriptor * db = _db;
	int rc;
	
	fclose(db->file);

	rc = rename(db->name, newName);
	if (rc != 0)
	{
		return FALSE;
	}

	strcpy(db->name, newName);
	
	db->file = fopen(db->name,(db->type == SDB_TYPE_TEXT) ? "r+t" : "r+b");	

	if (db->file == NULL)
	{
		return FALSE;
	}
	
	return TRUE;
}

bool sdb_consolidate_db(db_descriptor ** _db)
{
	_db_descriptor * db = *_db;
	_db_descriptor * tempDb;
	void * rec;
	uint32_t context;
	int rc;

	char tempfilename[MAX_SUPPORTED_FILENAME + TEMP_FILENAME_EXTENTION_LENGTH + 1];


	strcpy(tempfilename, db->name);
	strcat(tempfilename, ".tmp");

	rc = remove(tempfilename);
	if ((rc != 0) && (errno != ENOENT))
	{
		return FALSE;
	}

	tempDb = sdb_init_db(tempfilename, db->get_record_size, db->check_deleted, db->check_ignore, db->mark_deleted, db->consolidation_processing, db->type, db->bin_header_size);

	if (tempDb == NULL)
	{
		return FALSE;
	}

	db->check_ignore = NULL; //only deleted lines should be removed. Ignored lines should stay.
	
	rec = SDB_GET_FIRST_RECORD(db, &context);
	rc = TRUE;
	while ((rec != NULL) && (rc == TRUE))
	{

		if (db->consolidation_processing != NULL)
		{
			rc = db->consolidation_processing(tempDb, rec);
//			devListErrorComment(tempDb, rec);
		}
		else
		{
			rc = sdb_add_record(tempDb, rec);
		}
		
		rec = SDB_GET_NEXT_RECORD(db, &context);
	}

	strcpy(tempfilename, db->name);

	sdb_release_db(_db);
	(*_db) = tempDb;

	rc = remove(tempfilename);

	if (rc != 0)
	{
		return FALSE;
	}
	
	rc = sdb_rename_db(*_db, tempfilename);
	
	return rc;
}

void * sdb_get_record(db_descriptor * _db, void * key, check_key_f check_key, uint32_t * context)
{
	_db_descriptor * db = _db;
	static char rec[MAX_SUPPORTED_RECORD_SIZE];
	bool found = FALSE;
	uint32_t _context;


	if (context != NULL)
	{
		_context = *context;
	}
	else
	{
		_context = 0;
	}

	if (ftell(db->file) != _context)
	{
		fseek(db->file, _context, SEEK_SET);
	}

	if (db->type == SDB_TYPE_TEXT)
	{
		while ((! found) && (fgets(rec, sizeof(rec), db->file) != NULL)) //order matters!!!
		{
			db->last_accessed_record_start_file_pointer = _context;
			db->last_accessed_record_size = db->get_record_size(rec);
			_context = ftell(db->file);
			
			if (rec[strlen(rec) - 1] != '\n')
			{
				//todo: set errno: record too long
				return NULL;
			}

			if ((!(db->check_deleted(rec))) && ((db->check_ignore == NULL) || (!(db->check_ignore(rec)))) && ((check_key == NULL) || (check_key(rec, key) == SDB_CHECK_KEY_EQUAL)))
			{
				found = TRUE;
			}
		}
	}
	else //db->type == SDB_TYPE_BINARY
	{
		//todo
	}

	if (!found)
	{
		return NULL;
	}

	if (context != NULL)
	{
		*context = _context;	
	}
	return rec;
}


/***** USAGE EXAMPLE ***************************************************************

uint32_t text_db_get_record_size(void * record)
{
	return strlen(record);
}

uint32_t text_db_check_deleted(void * record)
{
	return (((char *)record)[0] == ';');
}

void text_db_mark_deleted(void * record)
{
	((char *)record)[0] = ';';
}

bool text_db_check_key_test(void * record, void * key)
{
	return memcmp(((char *)record) + 2, key, 4);
}

void main(void)
{
	char * rec;
	uint32_t context = 0;
	
	_db_descriptor * db;
	db = sdb_init_db("c:\\temp\\testdb.txt", text_db_get_record_size, text_db_check_deleted, text_db_mark_deleted, SDB_TYPE_TEXT, 0);
	sdb_add_record(db, "Hello world!\n");
	sdb_add_record(db, "Hello world2!\n");
	sdb_add_record(db, "This is line #2...\n");
	sdb_add_record(db, "Hello world3!\n");
	sdb_add_record(db, "And this is the third one.\n");
	sdb_add_record(db, "Hello world4!\n");
	sdb_add_record(db, "Hello world5!\n");
	sdb_delete_record(db,"llo ", text_db_check_key_test);

	while ((rec = sdb_get_record(db,"llo ", text_db_check_key_test, &context)) != NULL)
	{
		printf("-->%s", rec);
	}

	rec = SDB_GET_FIRST_RECORD(db, &context);
	while (rec != NULL)
	{
		printf("=->%s", rec);
		rec = SDB_GET_NEXT_RECORD(db, &context);
	}

	sdb_consolidate_db(&db);

	rec = SDB_GET_FIRST_RECORD(db, &context);
	while (rec != NULL)
	{
		printf("=->%s", rec);
		rec = SDB_GET_NEXT_RECORD(db, &context);
	}

	sdb_release_db(&db);
}
*/


const char * parsingErrorStrings[] = 
{
	"SDB_TXT_PARSER_RESULT_OK",
	"SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD",
	"SDB_TXT_PARSER_RESULT_UNEXPECTED_CHARACTER_OR_TOO_LONG",
	"SDB_TXT_PARSER_RESULT_FIELD_MISSING",
	"SDB_TXT_PARSER_RESULT_HEX_UNEXPECTED_CHARACTER_OR_TOO_SHORT",
	"SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE",
	"SDB_TXT_PARSER_RESULT_MISSING_STARTING_QUOTE",
	"SDB_TXT_PARSER_RESULT_MISSING_ENDING_QUOTE",
	"SDB_TXT_PARSER_RESULT_STRING_TOO_LONG",
};

void sdb_txt_parser_move_to_next_field(char ** pBuf, parsingResult_t * result)
{
	while ((result->code == SDB_TXT_PARSER_RESULT_OK) && (**pBuf != ',') && (**pBuf != '\0'))
	{
		if ((**pBuf != ' ') && (**pBuf != '\t') && (**pBuf != '\n')  && (**pBuf != '\r'))
		{
			result->code = SDB_TXT_PARSER_RESULT_UNEXPECTED_CHARACTER_OR_TOO_LONG;
//printf("char is %d\n", **pBuf);
			result->errorLocation = *pBuf;
		}
		else
		{
			(*pBuf)++;
		}
	}

	if (result->code == SDB_TXT_PARSER_RESULT_OK)
	{
		if (**pBuf == '\0')
		{
			result->code = SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD;
			result->errorLocation = *pBuf;
		}
		else
		{
			(*pBuf)++;
		}
	}

}

void sdb_txt_parser_get_hex_field(char ** pBuf, uint8_t * field, uint32_t len, parsingResult_t * result)
{
	int i;
	unsigned long tempNum;

	if (result->code == SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD)
	{
		result->code = SDB_TXT_PARSER_RESULT_FIELD_MISSING;
		result->errorLocation = *pBuf;
	}
	else
	{
		for (i = 0; (result->code == SDB_TXT_PARSER_RESULT_OK) && (i < len); i++)
		{
			tempNum = strtoul(*pBuf, pBuf, 16);
			if ((errno == ERANGE) | (tempNum > 0xFF))
			{
				result->code = SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE;
				result->errorLocation = (*pBuf) - 1;
			}
			else
			{
				field[(len - 1) - i] = (uint8)tempNum;

				if (i < (len - 1))
				{
					if (**pBuf != ':')
					{
						result->code = SDB_TXT_PARSER_RESULT_HEX_UNEXPECTED_CHARACTER_OR_TOO_SHORT;
						result->errorLocation = *pBuf;
	//					printf("pBuf=%p, *pBuf=%p, **pBuf=%d\n", pBuf, *pBuf, **pBuf);
	//					printf(":result->errorLocation=%p\n", result->errorLocation);
					}
					else
					{
						(*pBuf)++;
					}
				}
			}
		}

		if (result->code == SDB_TXT_PARSER_RESULT_OK)
		{
			result->field++;
			sdb_txt_parser_move_to_next_field(pBuf, result);
		}
	}
}

void sdb_txt_parser_get_numeric_field(char ** pBuf, uint8_t * field, uint32_t len, bool isSigned, parsingResult_t * result)
{
	union
	{
		signed long sNum;
		unsigned long uNum;
	} temp;
	int i;
	
	if (result->code == SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD)
	{
		result->code = SDB_TXT_PARSER_RESULT_FIELD_MISSING;
		result->errorLocation = *pBuf;
	}
	else if (result->code == SDB_TXT_PARSER_RESULT_OK)
	{
		if (isSigned)
		{
			temp.sNum = strtol(*pBuf, pBuf, 0);
			
			if ((errno == ERANGE) || (temp.sNum > (((signed long)0x7F) << (8*(len - 1)))) || (temp.sNum < (((signed long)0x80) << (8*(len - 1)))))
			{
				result->code = SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE;
				result->errorLocation = (*pBuf) - 1;
			}
		}
		else
		{
			temp.uNum = strtoul(*pBuf, pBuf, 0);
//printf("%u >? %u",temp.uNum, (((unsigned long)0xFF) << (8*(len-1))));
			if ((errno == ERANGE) || (temp.uNum > (((unsigned long)0xFF) << (8*(len-1)))))
			{
				result->code = SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE;
				result->errorLocation = (*pBuf) - 1;
			}
		}

		// From this point, only temp.uNum is used. It actually access temp.sNum, since temp is a union

		if (result->code == SDB_TXT_PARSER_RESULT_OK)
		{
			for (i = 0; i < len; i++)
			{
				*field++ = temp.uNum & 0xFF; //note: assuming little endian machine
				temp.uNum >>= 8;
			}

			if (temp.uNum != 0)
			{
				result->code = SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE;
				result->errorLocation = (*pBuf) - 1;
			}
			else
			{
				result->field++;
				sdb_txt_parser_move_to_next_field(pBuf, result);
			}
		}
	}
}


void sdb_txt_parser_get_quoted_string(char ** pBuf, char * field, size_t size, parsingResult_t * result)
{
	char * tmpPtr;
	size_t stringLen;
	
	if (result->code == SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD)
	{
		result->code = SDB_TXT_PARSER_RESULT_FIELD_MISSING;
		result->errorLocation = *pBuf;
	}
	else
	{
		while ((result->code == SDB_TXT_PARSER_RESULT_OK) && (**pBuf != '\"') && (**pBuf != '\0'))
		{
			if ((**pBuf != ' ') && (**pBuf != '\t'))
			{
				result->code = SDB_TXT_PARSER_RESULT_MISSING_STARTING_QUOTE;
				result->errorLocation = *pBuf;
			}
			else
			{
				(*pBuf)++;
			}
		}

		if (result->code == SDB_TXT_PARSER_RESULT_OK)
		{
			(*pBuf)++;
			tmpPtr = *pBuf;
			while ((**pBuf != '\"') && (**pBuf != '\0'))
			{
				(*pBuf)++;
			}
			
			if (**pBuf != '\"')
			{
				result->code = SDB_TXT_PARSER_RESULT_MISSING_ENDING_QUOTE;
				result->errorLocation = *pBuf;
			}
			else
			{
				stringLen = *pBuf - tmpPtr;
				if (stringLen > size)
				{
					result->code = SDB_TXT_PARSER_RESULT_STRING_TOO_LONG;
					result->errorLocation = tmpPtr + size;
				}
			}

			if (result->code == SDB_TXT_PARSER_RESULT_OK)
			{
				(*pBuf)++;
				memcpy(field, tmpPtr, stringLen);
				field[stringLen] = '\0';

				result->field++;
				sdb_txt_parser_move_to_next_field(pBuf, result);
			}
		}
	}
}

