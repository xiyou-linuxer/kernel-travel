#ifndef _FS_FILE_TIME_H
#define _FS_FILE_TIME_H
typedef struct file_time {
	long st_atime_sec;
	long st_atime_nsec;
	long st_mtime_sec;
	long st_mtime_nsec;
	long st_ctime_sec;
	long st_ctime_nsec;
} file_time_t;
#endif