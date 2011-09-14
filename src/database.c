#include <ctype.h>

#ifndef WIN32
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <time.h>
#include <Winsock2.h>
#include <direct.h>
#endif

#include <libmediascan.h>
#include <libavformat/avformat.h>
#include <db.h>

#ifdef WIN32
#include "mediascan_win32.h"
#endif


#include "common.h"
#include "buffer.h"
#include "queue.h"
#include "progress.h"
#include "result.h"
#include "error.h"
#include "mediascan.h"
#include "thread.h"
#include "util.h"

DB_ENV *myEnv;                  /* Env structure handle */

void reset_bdb(MediaScan *s) {
  u_int32_t records;

  s->dbp->truncate(s->dbp, NULL, &records, 0);

  LOG_INFO("Database cleared. %d records deleted\n", records);
}                               /* reset_bdb() */

int init_bdb(MediaScan *s) {
  int ret;
  int tmp_flags;
  char dbpath[MAX_PATH_STR_LEN];

  if (s->dbp)
    return 1;

  // Create an environment object and initialize it for error reporting.
  ret = db_env_create(&myEnv, 0);
  if (ret != 0) {
    LOG_ERROR("Error creating database env handle: %s\n", db_strerror(ret));
    return 0;
  }

  // Open the environment.
  ret = myEnv->open(myEnv,      // DB_ENV ptr
                    s->cachedir ? s->cachedir : ".",  // env home directory
                    DB_CREATE | DB_INIT_MPOOL,  // Open flags
                    0);         // File mode (default)

  if (ret != 0) {
    LOG_ERROR("Environment open failed: %s\n", db_strerror(ret));
    return 0;
  }

  /* Initialize the structure. This
   * database is opened in an environment,
   * so the environment pointer is myEnv. */
  ret = db_create(&s->dbp, myEnv, 0);
  if (ret != 0) {
    ms_errno = MSENO_DBERROR;
    s->dbp = NULL;
    LOG_ERROR("Database creation failed: %s", db_strerror(ret));
    return 0;
  }

  /* open the database */
  sprintf(dbpath, "%s/libmediascan.db", s->cachedir ? s->cachedir : ".");

  if (s->flags & MS_FULL_SCAN) {
    tmp_flags = DB_CREATE | DB_TRUNCATE;
  }
  else {
    tmp_flags = DB_CREATE;
  };

  ret = s->dbp->open(s->dbp,    /* DB structure pointer */
                     NULL,      /* Transaction pointer */
                     dbpath,    /* On-disk file that holds the database. */
                     NULL,      /* Optional logical database name */
                     DB_BTREE,  /* Database access method */
                     tmp_flags, /* Open flags */
                     0);        /* File mode (using defaults) */

  if (ret != 0) {
    ms_errno = MSENO_DBERROR;
    s->dbp->close(s->dbp, 0);
    s->dbp = NULL;
    LOG_ERROR("Database open failed: %s\n", db_strerror(ret));
    return 0;
  }

  return 1;
}                               /* init_bdb() */
