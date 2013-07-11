
#ifndef SIMPLE_DB_H
#define SIMPLE_DB_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
#define FALSE 0
#define TRUE (!FALSE)
typedef int bool;
typedef unsigned __int32 uint32_t;
typedef unsigned __int8 uint8_t;
*/

#define MAX_SUPPORTED_RECORD_SIZE 500
#define MAX_SUPPORTED_FILENAME 100


#define SDB_CHECK_KEY_EQUAL 0
#define SDB_CHECK_KEY_BIGGER 1
#define SDB_CHECK_KEY_SMALLER (-1)
#define SDB_CHECK_KEY_NOT_EQUAL 2
#define SDB_CHECK_KEY_ERROR 3

enum
{
	SDB_TYPE_TEXT,
	SDB_TYPE_BINARY
};

typedef void db_descriptor;

typedef int(* check_key_f)(void * record, void * key);
typedef uint32(* get_record_size_f)(void * record);
typedef bool(* check_deleted_f)(void * record);
typedef bool(* check_ignore_f)(void * record);
typedef void(* mark_deleted_f)(void * record);
typedef bool(* consolidation_processing_f)(db_descriptor * db, void * record);

typedef struct
{
	char * errorLocation;
	int code;
	uint16_t field;
} parsingResult_t;

db_descriptor * sdb_init_db(char * name, get_record_size_f get_record_size, check_deleted_f check_deleted, check_ignore_f check_ignore, mark_deleted_f mark_deleted, consolidation_processing_f consolidation_processing, uint8_t db_type, uint32_t db_bin_header_size);
bool sdb_add_record(db_descriptor * db, void * rec);
void * sdb_delete_record(db_descriptor * db, void * key, check_key_f check_key);
bool sdb_consolidate_db(db_descriptor ** db);
void * sdb_get_record(db_descriptor * db, void * key, check_key_f check_key, uint32_t * context);
bool sdb_release_record(void ** record);
bool sdb_release_db(db_descriptor ** db);
void sdb_flush_db(db_descriptor * db);
bool sdb_modify_last_accessed_record(db_descriptor * _db, void * record);
#define SDB_GET_FIRST_RECORD(_db, _context) ((*(_context) = 0), sdb_get_record((_db), NULL, NULL, _context))
#define SDB_GET_NEXT_RECORD(_db, _context) (sdb_get_record((_db), NULL, NULL, _context))
#define SDB_GET_UNIQUE_RECORD(_db, _key, _check_key_func) (sdb_get_record((_db), (_key), (_check_key_func), NULL))

extern int sdbErrno;
extern const char * parsingErrorStrings[];

/* Macros for parsing records of a TEXT based database */
/* The following variables are expected to be defined (and initialized as specified):   */
/* bool end_of_record = FALSE; */
/* char * pBuf = record; */

#define SDB_TXT_PARSER_RESULT_OK 0
#define SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD 1
#define SDB_TXT_PARSER_RESULT_UNEXPECTED_CHARACTER_OR_TOO_LONG 2
#define SDB_TXT_PARSER_RESULT_FIELD_MISSING 3
#define SDB_TXT_PARSER_RESULT_HEX_UNEXPECTED_CHARACTER_OR_TOO_SHORT 4
#define SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE 5
#define SDB_TXT_PARSER_RESULT_MISSING_STARTING_QUOTE 6
#define SDB_TXT_PARSER_RESULT_MISSING_ENDING_QUOTE 7
#define SDB_TXT_PARSER_RESULT_STRING_TOO_LONG 8
#define SDB_TXT_PARSER_RESULT_MAX 8

void sdb_txt_parser_get_hex_field(char ** pBuf, uint8_t * field, uint32_t len, parsingResult_t * reault);
void sdb_txt_parser_get_numeric_field(char ** pBuf, uint8_t * field, uint32_t len, bool isSigned, parsingResult_t * reault); //, int * prev_result);
void sdb_txt_parser_get_quoted_string(char ** pBuf, char * field, size_t size, parsingResult_t * reault);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_DB_H */

