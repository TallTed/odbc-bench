/*
 *  odbcbench.h
 * 
 *  $Id$
 *
 *  odbc-bench - a TPCA and TPCC benchmark program for databases 
 *  Copyright (C) 2000-2002 OpenLink Software <odbc-bench@openlinksw.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct test_s	test_t;

#include "odbcinc.h"
#include "olist.h"

#define NUMITEMS(p1) (sizeof(p1)/sizeof(p1[0]))
#define IDX_PLAINSQL 1011
#define	IDX_PARAMS   1012
#define	IDX_SPROCS   1013

#define XFREE(X)   if (X) free(X);

/*
 * Reasonable defaults
 */
#ifndef HAVE_CONFIG_H
#define PACKAGE			"odbc-bench"
#define PACKAGE_BUGREPORT	"odbc-bench@openlinksw.com"
#define PACKAGE_NAME 		"OpenLink ODBC Benchmark Utility"
#define PACKAGE_STRING 		"OpenLink ODBC Benchmark Utility 0.99.3"
#define PACKAGE_TARNAME 	"odbc-bench"
#define PACKAGE_VERSION 	"0.99.3"
#define VERSION			"0.99.3"
#endif

extern void (*pane_log) (const char *format, ...);

#ifdef _DEBUG
#define assert(exp)	((exp) ? (void)0 : g_error("'%s', File %s, line %d", #exp, __FILE__, __LINE__))
#else
#define assert(exp)
#endif

#if 0
void pipe_trough_isql (char *szFileName);
#else
void pipe_trough_isql (HSTMT stmt, char *szFileName, int print_commands);
#endif

typedef struct GUI_s
{
  void (*main_quit)(void);

  void (*err_message)(const char *, ...);
  void (*warn_message)(const char *, ...);
  void (*message)(const char *, ...);

  int  (*add_test_to_the_pool) (test_t * ptest);
  void (*for_all_in_pool) (void);
  void (*do_MarkFinished) (int nConn, int thread_no);

  BOOL (*isCancelled)(void);

  void (*ShowProgress) (void * parent_win, char * title, BOOL bForceSingle, float nMax);
  void (*SetWorkingItem) (char *pszWorking);
  void (*SetProgressText) (char *pszProgress, int n_conn, int thread_no,
      float nValue, int nTrnPerCall);
  void (*StopProgress) (void);
  int (*fCancel) (void);
  void (*vBusy) (void);  /* Change to Hourglass/Normal Cursor */
  
} GUI_t;

extern HENV henv;
extern GUI_t gui;

/* odbc callbacks - odbcs.c */
typedef enum
{ TPC_A, TPC_C }
testtype;

typedef struct test_common_s
{
  int nThreads;
  int nThreadNo;
  int nConn;

  int nMinutes;			/* Minutes to execute the test */
  int nRuns;			/* How many runs to do */
}
test_common_t;

#include "timeacct.h"
#include "tpca.h"
#include "tpcc.h"

struct test_s
{
  char szName[128];
  testtype TestType;
  int is_dirty;

  void *hwndOut;		/* Output window for logging info */

  /* ODBC handles */
  HDBC hdbc;			/* Connection handle */
  HSTMT hstmt;			/* Statement handle */

  /* login data */
  char szLoginDSN[50];
  char szLoginUID[50];
  char szLoginPWD[50];

  char szSQLState[10];
  char szSQLError[256];

  /* Other info */
  char szDSN[SQL_MAX_DSN_LENGTH];
  char szDBMS[128];
  char szDBMSVer[128];
  char szDriverName[128];
  char szDriverVer[128];
  char *pszDateTimeSQLFunc;	/* ODBC scalar function for current date/time */

  int fAsyncSupported;		/* TRUE if async mode is supported */
  int fCommitSupported;		/* TRUE if commitment control is supported */
  int fProcsSupported;		/* TRUE if sprocs are supported */
  int fBatchSupported;
  int fSQLBatchSupported;
  unsigned long nCursorsSupported;	/* the bitmask of supported cursor types */
  unsigned long nIsolationsSupported;	/* the bitmask of supported isolation levels */
  long default_txn_isolation;	/* txn_isolation_mode */

  /* log functions */
  void (*ShowProgress) (void * parent_win, char * title, BOOL bForceSingle, float nMax);
  void (*SetWorkingItem) (char *pszWorking);
  void (*SetProgressText) (char *pszProgress, int n_conn, int thread_no,
      float nValue, int nTrnPerCall);
  void (*StopProgress) (void);
  int (*fCancel) (void);

  int fHaveResults;		/* TRUE if sprocs are supported */
  union
  {
    struct tpca_s a;
    struct tpcc_s c;
    struct test_common_s _;
  }
  tpc;

  char szTemp[512];
  BOOL bTablePropsShown, bRunPropsShown;
};

#define IS_A(test) ((test).TestType == TPC_A)
#define IS_C(test) ((test).TestType == TPC_C)

void get_dsn_data (test_t * ptest);
/* from prefs.c */
typedef enum
{
  A_REFRESH_RATE,
  C_REFRESH_RATE,
  LOCK_TIMEOUT,
  DISPLAY_REFRESH_RATE
}
OdbcBenchPref;

long bench_get_long_pref (OdbcBenchPref pref);
int bench_set_long_pref (OdbcBenchPref pref, long value);
char *bench_get_string_pref (OdbcBenchPref pref);

int _strnicmp (const char *s1, const char *s2, size_t n);
char *_stristr (const char *str, const char *find);

BOOL fSQLExecDirect (HSTMT hstmt, char *pszSqlStr, test_t * lpBench);
BOOL fSQLGetData (HSTMT hstmt, UWORD icol, SWORD fCType, PTR rgbValue,
    SDWORD cbValueMax, SDWORD FAR * pcbValue);
BOOL fSQLBindCol (HSTMT hstmt, UWORD icol, SWORD fCType, PTR rgbValue,
    SDWORD cbValueMax, SDWORD FAR * pcbValue);
BOOL fSQLParamOptions (HSTMT hstmt, UDWORD crow, UDWORD *pirow);

#if defined(WIN32)
#define sleep_msecs(x) Sleep(x)
#else
#include <sys/poll.h>
#define sleep_msecs(x) poll (NULL, 0, x)
#endif

int do_save_selected (char *szFileName, OList * tests);
void do_save_run_results (char *filename, OList * selected, int nMinutes);
void init_test (test_t * test);


/*********************
#ifndef	FALSE
#define	FALSE	(0)
#endif

#ifndef	TRUE
#define	TRUE	(!FALSE)
#endif

**********************/