#include "bootpack.h"
#include "sqlite3.h"
sqlite3* db_vfs;
void vfs_init(){
	sqlite3_open(":memory:", &db_vfs); // ":memory:" 表示内存数据库
	sqlite3_exec(db, "CREATE TABLE path2path(rootpath VARCHAR(25000),destpath VARCHAR(25000));", 0, 0, 0);
	sqlite3_exec(db, "CREATE TABLE path2cache(rootpath VARCHAR(25000),cache BIGINT);", 0, 0, 0);
}
void vfs_add_path(char* path,CACHE_TABLE* cache){
	
}

