/*
 *  tests.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libxml/xmlmemory.h>
#if defined(LIBXML_VERSION) && LIBXML_VERSION >= 20000
#include <libxml/parser.h>
#else
#include <gnome-xml/parser.h>
#endif

#include "odbcbench.h"
#include "tpca_code.h"
#include "thr.h"

void
init_test (test_t * ptest)
{
  ptest->ShowProgress = gui.ShowProgress;
  ptest->SetWorkingItem = gui.SetWorkingItem;
  ptest->SetProgressText = gui.SetProgressText;
  ptest->StopProgress = gui.StopProgress;
  ptest->fCancel = gui.fCancel;
  ptest->tpc._.nMinutes = 5;
  ptest->tpc._.nRuns = 1;
  switch (ptest->TestType)
    {
    case TPC_A:
      {
	tpca_t *a = &ptest->tpc.a;
	a->uwDrvIdx = -1;
	a->fClearHistory = TRUE;
	a->fSQLOption = IDX_PLAINSQL;
	a->udwMaxBranch = 10;
	a->udwMaxTeller = 100;
	a->udwMaxAccount = 1000;

	a->fCreateBranch = TRUE;
	a->fCreateTeller = TRUE;
	a->fCreateAccount = TRUE;
	a->fCreateHistory = TRUE;
	a->fLoadBranch = TRUE;
	a->fLoadTeller = TRUE;
	a->fLoadAccount = TRUE;
	a->fCreateIndex = TRUE;
	a->fCreateProcedure = TRUE;
      }
      break;
    case TPC_C:
      {
	tpcc_t *c = &ptest->tpc.c;
	c->count_ware = 1;
	c->local_w_id = 1;
	c->nRounds = 1;
      }
      break;
    }
}


char *
test_type_name (long fSQLOption, char *def)
{
  switch (fSQLOption)
    {
    case IDX_PLAINSQL:
      return "execute";
    case IDX_PARAMS:
      return "prepare";
    case IDX_SPROCS:
      return "procedures";
    default:
      return def;
    }
}


short
test_type_from_name (char *opt)
{
  if (!strcmp (opt, "execute"))
    return (IDX_PLAINSQL);
  else if (!strcmp (opt, "prepare"))
    return (IDX_PARAMS);
  else if (!strcmp (opt, "procedures"))
    return (IDX_SPROCS);
  else
    return 0;
}

#define _xmlNewProp(ent, nam, val) \
{ \
  prop = xmlNewProp(ent, nam, val); \
  prop->doc = (ent)->doc;\
}

xmlNodePtr
make_test_node (test_t * test, xmlNsPtr ns, xmlNodePtr parent)
{
  xmlNodePtr tst, login, dsn, schema, table, run;
  char szProp[1024];
  xmlAttrPtr prop;

  tst = xmlNewChild (parent, ns, "test", NULL);
  _xmlNewProp (tst, "id", test->szName);
  _xmlNewProp (tst, "type", test->TestType == TPC_A ? "tpc_a" : "tpc_c");

  login = xmlNewChild (tst, ns, "login", NULL);

  if (test->szLoginDSN[0])
    {
      dsn = xmlNewChild (login, ns, "dsn", NULL);
      _xmlNewProp (dsn, "id", test->szLoginDSN);
      _xmlNewProp (dsn, "name", test->szLoginDSN);
      _xmlNewProp (dsn, "uid", test->szLoginUID);
/*      _xmlNewProp (dsn, "pwd", test->szLoginPWD);*/
      _xmlNewProp (dsn, "DBMS", test->szDBMS);
      _xmlNewProp (dsn, "DBMSVer", test->szDBMSVer);
      _xmlNewProp (dsn, "Driver", test->szDriverName);
      _xmlNewProp (dsn, "DriverVer", test->szDriverVer);
    }

  switch (test->TestType)
    {
    case TPC_A:
      schema = xmlNewChild (tst, ns, "aschema", NULL);
      sprintf (szProp, "%d", test->tpc.a.fCreateProcedure);
      _xmlNewProp (schema, "procedures", szProp);
      sprintf (szProp, "%d", test->tpc.a.fCreateIndex);
      _xmlNewProp (schema, "indexes", szProp);
      if (test->tpc.a.uwDrvIdx != -1)
	_xmlNewProp (schema, "type",
	    getDriverDBMSName (test->tpc.a.uwDrvIdx));

      table = xmlNewChild (schema, ns, "table", NULL);
      _xmlNewProp (table, "name", "branch");
      sprintf (szProp, "%d", test->tpc.a.fCreateBranch);
      _xmlNewProp (table, "create", szProp);
      sprintf (szProp, "%d", test->tpc.a.fLoadBranch);
      _xmlNewProp (table, "load", szProp);
      sprintf (szProp, "%ld", test->tpc.a.udwMaxBranch);
      _xmlNewProp (table, "count", szProp);
      if (test->tpc.a.szBranchDSN[0])
	{
	  dsn = xmlNewChild (table, ns, "dsn", NULL);
	  _xmlNewProp (dsn, "id", test->tpc.a.szBranchDSN);
	  _xmlNewProp (dsn, "name", test->tpc.a.szBranchDSN);
	  _xmlNewProp (dsn, "DBMS", test->tpc.a.szBranchDBMS);
	}

      table = xmlNewChild (schema, ns, "table", NULL);
      _xmlNewProp (table, "name", "teller");
      sprintf (szProp, "%d", test->tpc.a.fCreateTeller);
      _xmlNewProp (table, "create", szProp);
      sprintf (szProp, "%d", test->tpc.a.fLoadTeller);
      _xmlNewProp (table, "load", szProp);
      sprintf (szProp, "%ld", test->tpc.a.udwMaxTeller);
      _xmlNewProp (table, "count", szProp);
      if (test->tpc.a.szTellerDSN[0])
	{
	  dsn = xmlNewChild (table, ns, "dsn", NULL);
	  _xmlNewProp (dsn, "id", test->tpc.a.szTellerDSN);
	  _xmlNewProp (dsn, "name", test->tpc.a.szTellerDSN);
	  _xmlNewProp (dsn, "DBMS", test->tpc.a.szTellerDBMS);
	}

      table = xmlNewChild (schema, ns, "table", NULL);
      _xmlNewProp (table, "name", "account");
      sprintf (szProp, "%d", test->tpc.a.fCreateAccount);
      _xmlNewProp (table, "create", szProp);
      sprintf (szProp, "%d", test->tpc.a.fLoadAccount);
      _xmlNewProp (table, "load", szProp);
      sprintf (szProp, "%ld", test->tpc.a.udwMaxAccount);
      _xmlNewProp (table, "count", szProp);
      if (test->tpc.a.szAccountDSN[0])
	{
	  dsn = xmlNewChild (table, ns, "dsn", NULL);
	  _xmlNewProp (dsn, "id", test->tpc.a.szAccountDSN);
	  _xmlNewProp (dsn, "name", test->tpc.a.szAccountDSN);
	  _xmlNewProp (dsn, "DBMS", test->tpc.a.szAccountDBMS);
	}

      table = xmlNewChild (schema, ns, "table", NULL);
      _xmlNewProp (table, "name", "history");
      sprintf (szProp, "%d", test->tpc.a.fCreateHistory);
      _xmlNewProp (table, "create", szProp);
      _xmlNewProp (table, "load", "0");
      _xmlNewProp (table, "count", "0");
      if (test->tpc.a.szHistoryDSN[0])
	{
	  dsn = xmlNewChild (table, ns, "dsn", NULL);
	  _xmlNewProp (dsn, "id", test->tpc.a.szHistoryDSN);
	  _xmlNewProp (dsn, "name", test->tpc.a.szHistoryDSN);
	  _xmlNewProp (dsn, "DBMS", test->tpc.a.szHistoryDBMS);
	}

      run = xmlNewChild (tst, ns, "arun", NULL);
      sprintf (szProp, "%d", test->tpc._.nThreads);
      _xmlNewProp (run, "threads", szProp);
      sprintf (szProp, "%d", test->tpc.a.nArrayParSize);
      _xmlNewProp (run, "arrayparsize", szProp);
      sprintf (szProp, "%d", test->tpc.a.fUseCommit);
      _xmlNewProp (run, "transactions", szProp);
      sprintf (szProp, "%d", test->tpc.a.fDoQuery);
      _xmlNewProp (run, "query", szProp);
      sprintf (szProp, "%d", test->tpc.a.fExecAsync);
      _xmlNewProp (run, "async", szProp);
      _xmlNewProp (run, "isolation",
	  txn_isolation_name (test->tpc.a.txn_isolation, "default"));
      _xmlNewProp (run, "cursor", cursor_type_name (test->tpc.a.nCursorType,
	      "Default"));
      sprintf (szProp, "%d", test->tpc.a.nRowsetSize);
      _xmlNewProp (run, "rowsetsize", szProp);
      sprintf (szProp, "%d", test->tpc.a.nKeysetSize);
      _xmlNewProp (run, "keysetsize", szProp);
      sprintf (szProp, "%d", test->tpc.a.nTraversalCount);
      _xmlNewProp (run, "traversal", szProp);
      _xmlNewProp (run, "type", test_type_name (test->tpc.a.fSQLOption,
	      "All"));
      break;

    case TPC_C:
      {
	char *szNames[] = {
	  "WAREHOUSE",
	  "DISTRICT",
	  "CUSTOMER",
	  "THISTORY",
	  "NEW_ORDER",
	  "ORDERS",
	  "ORDER_LINE",
	  "ITEM",
	  "STOCK"
	};
	int i;

	schema = xmlNewChild (tst, ns, "cschema", NULL);
	sprintf (szProp, "%ld", test->tpc.c.count_ware);
	_xmlNewProp (tst, "warehouses", szProp);
	for (i = 0; i < 9; i++)
	  {
	    table = xmlNewChild (schema, ns, "table", NULL);
	    _xmlNewProp (table, "name", szNames[i]);
	    _xmlNewProp (table, "create", "1");
	    _xmlNewProp (table, "load", "0");
	    _xmlNewProp (table, "count", "0");
	    if (test->tpc.c.tableDSNS[i][0])
	      {
		dsn = xmlNewChild (table, ns, "dsn", NULL);
		_xmlNewProp (dsn, "id", test->tpc.c.tableDSNS[i]);
		_xmlNewProp (dsn, "name", test->tpc.c.tableDSNS[i]);
		_xmlNewProp (dsn, "DBMS", test->tpc.c.tableDBMSes[i]);
	      }
	  }
	run = xmlNewChild (tst, ns, "crun", NULL);
	sprintf (szProp, "%d", test->tpc._.nThreads);
	_xmlNewProp (run, "threads", szProp);
      }
      break;
    }
  return tst;
}


int
do_save_selected (char *filename, OList * tests)
{
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlNodePtr tst, root;
  int ret;
  OList *iter = tests;

  doc = xmlNewDoc ("1.0");
  xmlCreateIntSubset (doc, "tests", NULL, "odbc-bench.dtd");
  ns = NULL;

  root = xmlNewDocNode (doc, ns, "tests", NULL);
  xmlDocSetRootElement (doc, root);

  iter = tests;
  while (iter)
    {
      test_t *test = (test_t *) iter->data;

      tst = make_test_node (test, ns, root);
      iter = o_list_next (iter);
    }

  ret = (xmlSaveFile (filename, doc) != -1);
  xmlFreeDoc (doc);
  if (ret)
    {
      iter = tests;
      while (iter)
	{
	  test_t *test = (test_t *) iter->data;
	  test->is_dirty = FALSE;
	  iter = o_list_next (iter);
	}
    }
  return ret;
}

#define XML_GET_PROP(elem, nam, def) \
if (!(szProp = xmlGetProp (elem, nam))) \
  szProp = def;

static int
tpcc_table_index_from_name (char *szTable)
{
  char *szNames[] = {
    "WAREHOUSE",
    "DISTRICT",
    "CUSTOMER",
    "THISTORY",
    "NEW_ORDER",
    "ORDERS",
    "ORDER_LINE",
    "ITEM",
    "STOCK"
  };
  int i;
  for (i = 0; i < 9; i++)
    if (!strcmp (szNames[i], szTable))
      return i;
  return -1;
}


test_t *
make_test_from_node (xmlNodePtr cur)
{
  test_t *ret;
  char *szProp;

  if (strcmp (cur->name, "test"))
    {
      pane_log ("document of the wrong type");
      return NULL;
    }

  ret = malloc (sizeof (test_t));
  memset (ret, 0, sizeof (test_t));

  XML_GET_PROP (cur, "type", "tpc_a");
  if (!strcmp (szProp, "tpc_a"))
    ret->TestType = TPC_A;
  else if (!strcmp (szProp, "tpc_c"))
    ret->TestType = TPC_C;
  else
    ret->TestType = TPC_A;
  init_test (ret);

  XML_GET_PROP (cur, "id", "<unknown>");
  strncpy (ret->szName, szProp, sizeof (ret->szName));

  cur = cur->children;

  while (cur)
    {
      if (!strcmp ("login", cur->name))
	{
	  xmlNodePtr dsn = cur->children;
	  while (dsn && xmlIsBlankNode (dsn))
	    {
	      dsn = dsn->next;
	    }
	  if (dsn && !strcmp (dsn->name, "dsn"))
	    {
	      char szDriverName[128], szDriverVer[128], szDBMS[128],
		  szDBMSVer[128];

	      XML_GET_PROP (dsn, "name", "");
	      strcpy (ret->szLoginDSN, szProp);

	      XML_GET_PROP (dsn, "uid", "");
	      strcpy (ret->szLoginUID, szProp);

	      ret->szLoginPWD[0] = 0;
/*	      XML_GET_PROP (dsn, "pwd", "");
	      strcpy (ret->szLoginPWD, szProp);*/

	      XML_GET_PROP (dsn, "DBMS", "");
	      strcpy (szDBMS, szProp);

	      XML_GET_PROP (dsn, "DBMSVer", "");
	      strcpy (szDBMSVer, szProp);

	      XML_GET_PROP (dsn, "Driver", "");
	      strcpy (szDriverName, szProp);

	      XML_GET_PROP (dsn, "DriverVer", "");
	      strcpy (szDriverVer, szProp);
	      if (szDBMS[0])
		{
		  pane_log
		      ("\n\nLast login for %s\nDBMS : %s\nDBMS Ver. : %s\nDriver : %s\nDriver Ver. : %s",
		      ret->szName, szDBMS, szDBMSVer, szDriverName,
		      szDriverVer);
		}
/*	      do_login (ret);
	      if (ret->szSQLError[0])
		{
		  pane_log ("Error logging in : [%s] %s\n", ret->szSQLState, ret->szSQLError);
		}
	      get_dsn_data (ret);
	      if (ret->hdbc)
		{
		  if (strcmp (szDBMS, ret->szDBMS))
		    pane_log ("Stored (%s) and obtained (%s) DBMS names differ\n",  
			szDBMS, ret->szDBMS);

		  if (strcmp (szDBMSVer, ret->szDBMSVer))
		    pane_log ("Stored (%s) and obtained (%s) DBMS versions differ\n",  
			szDBMSVer, ret->szDBMSVer);

		  if (strcmp (szDriverName, ret->szDriverName))
		    pane_log ("Stored (%s) and obtained (%s) Driver names differ\n",  
			szDriverName, ret->szDriverName);

		  if (strcmp (szDriverVer, ret->szDriverVer))
		    pane_log ("Stored (%s) and obtained (%s) Driver versions differ\n",  
			szDriverVer, ret->szDriverVer);
		}
*/
	    }
	}
      else if (!strcmp ("aschema", cur->name) && ret->TestType == TPC_A)
	{
	  xmlNodePtr tables;
	  XML_GET_PROP (cur, "procedures", "");
	  ret->tpc.a.fCreateProcedure = atoi (szProp);
	  if (ret->hdbc && ret->tpc.a.fCreateProcedure
	      && !ret->fProcsSupported)
	    {
	      pane_log
		  ("TPC-A will try to create procedures, but they are not supported by the data source\n");
	      ret->tpc.a.fCreateProcedure = FALSE;
	    }

	  XML_GET_PROP (cur, "indexes", "");
	  ret->tpc.a.fCreateIndex = atoi (szProp);

	  XML_GET_PROP (cur, "type", "");
	  if (strlen (szProp) < 1)
	    ret->tpc.a.uwDrvIdx = -1;
	  else
	    ret->tpc.a.uwDrvIdx = getDriverTypeIndex (szProp);

	  tables = cur->children;
	  while (tables)
	    {
	      if (!strcmp (tables->name, "table"))
		{
		  xmlNodePtr dsn = tables->children;

		  while (dsn && xmlIsBlankNode (dsn))
		    dsn = dsn->next;
		  if (dsn && !strcmp (dsn->name, "dsn"))
		    dsn = NULL;

		  XML_GET_PROP (tables, "name", "");
		  if (!strcmp (szProp, "branch"))
		    {
		      XML_GET_PROP (tables, "create", "");
		      ret->tpc.a.fCreateBranch = atoi (szProp);

		      XML_GET_PROP (tables, "load", "");
		      ret->tpc.a.fLoadBranch = atoi (szProp);

		      XML_GET_PROP (tables, "count", "");
		      ret->tpc.a.udwMaxBranch = atoi (szProp);

		      if (dsn)
			{
			  XML_GET_PROP (dsn, "name", "");
			  strncpy (ret->tpc.a.szBranchDSN, szProp,
			      sizeof (ret->tpc.a.szBranchDSN));
			  XML_GET_PROP (dsn, "DBMS", "");
			  strncpy (ret->tpc.a.szBranchDBMS, szProp,
			      sizeof (ret->tpc.a.szBranchDBMS));
			}
		    }
		  else if (!strcmp (szProp, "teller"))
		    {
		      XML_GET_PROP (tables, "create", "");
		      ret->tpc.a.fCreateTeller = atoi (szProp);

		      XML_GET_PROP (tables, "load", "");
		      ret->tpc.a.fLoadTeller = atoi (szProp);

		      XML_GET_PROP (tables, "count", "");
		      ret->tpc.a.udwMaxTeller = atoi (szProp);
		      if (dsn)
			{
			  XML_GET_PROP (dsn, "name", "");
			  strncpy (ret->tpc.a.szTellerDSN, szProp,
			      sizeof (ret->tpc.a.szTellerDSN));
			  XML_GET_PROP (dsn, "DBMS", "");
			  strncpy (ret->tpc.a.szTellerDBMS, szProp,
			      sizeof (ret->tpc.a.szTellerDBMS));
			}
		    }
		  else if (!strcmp (szProp, "account"))
		    {
		      XML_GET_PROP (tables, "create", "");
		      ret->tpc.a.fCreateAccount = atoi (szProp);

		      XML_GET_PROP (tables, "load", "");
		      ret->tpc.a.fLoadAccount = atoi (szProp);

		      XML_GET_PROP (tables, "count", "");
		      ret->tpc.a.udwMaxAccount = atoi (szProp);
		      if (dsn)
			{
			  XML_GET_PROP (dsn, "name", "");
			  strncpy (ret->tpc.a.szAccountDSN, szProp,
			      sizeof (ret->tpc.a.szAccountDSN));
			  XML_GET_PROP (dsn, "DBMS", "");
			  strncpy (ret->tpc.a.szAccountDBMS, szProp,
			      sizeof (ret->tpc.a.szAccountDBMS));
			}
		    }
		  else if (!strcmp (szProp, "history"))
		    {
		      XML_GET_PROP (tables, "create", "");
		      ret->tpc.a.fCreateHistory = atoi (szProp);
		      if (dsn)
			{
			  XML_GET_PROP (dsn, "name", "");
			  strncpy (ret->tpc.a.szHistoryDSN, szProp,
			      sizeof (ret->tpc.a.szHistoryDSN));
			  XML_GET_PROP (dsn, "DBMS", "");
			  strncpy (ret->tpc.a.szHistoryDBMS, szProp,
			      sizeof (ret->tpc.a.szHistoryDBMS));
			}
		    }
		}
	      tables = tables->next;
	    }
	}
      else if (!strcmp ("arun", cur->name) && ret->TestType == TPC_A)
	{
	  XML_GET_PROP (cur, "threads", "");
	  ret->tpc._.nThreads = atoi (szProp);

	  XML_GET_PROP (cur, "arrayparsize", "");
	  ret->tpc.a.nArrayParSize = atoi (szProp);

	  XML_GET_PROP (cur, "transactions", "");
	  ret->tpc.a.fUseCommit = atoi (szProp);
	  if (ret->hdbc && ret->tpc.a.fUseCommit && !ret->fCommitSupported)
	    {
	      pane_log
		  ("TPC-A will try to use transactions, but they are not supported by the data source\n");
	      ret->tpc.a.fUseCommit = FALSE;
	    }

	  XML_GET_PROP (cur, "query", "");
	  ret->tpc.a.fDoQuery = atoi (szProp);

	  XML_GET_PROP (cur, "async", "");
	  ret->tpc.a.fExecAsync = atoi (szProp);
	  if (ret->hdbc && ret->tpc.a.fExecAsync && !ret->fAsyncSupported)
	    {
	      pane_log
		  ("TPC-A will try to be asynchronous, but this is not supported by the data source\n");
	      ret->tpc.a.fUseCommit = FALSE;
	    }

	  XML_GET_PROP (cur, "isolation", "Default");
	  ret->tpc.a.txn_isolation = txn_isolation_from_name (szProp);
	  if (ret->hdbc && ret->tpc.a.txn_isolation
	      && !(ret->nIsolationsSupported & ret->tpc.a.txn_isolation))
	    {
	      pane_log
		  ("TPC-A will try use %s isolation, but this is not supported by the data source\n",
		  szProp);
	      ret->tpc.a.txn_isolation = 0;
	    }

	  XML_GET_PROP (cur, "cursor", "Default");
	  ret->tpc.a.nCursorType = cursor_type_from_name (szProp);
	  if (ret->hdbc && ret->tpc.a.nCursorType)
	    {
	      long mask = 0;
	      switch (ret->tpc.a.nCursorType)
		{
		case SQL_CURSOR_FORWARD_ONLY:
		  mask = SQL_SO_FORWARD_ONLY;
		  break;
		case SQL_CURSOR_STATIC:
		  mask = SQL_SO_STATIC;
		  break;
		case SQL_CURSOR_MIXED:
		case SQL_CURSOR_KEYSET_DRIVEN:
		  mask = SQL_SO_KEYSET_DRIVEN;
		  break;
		case SQL_CURSOR_DYNAMIC:
		  mask = SQL_SO_DYNAMIC;
		  break;
		}

	      if (!(ret->nCursorsSupported & mask))
		{
		  pane_log
		      ("TPC-A will try use %s cursor, but this is not supported by the data source\n",
		      szProp);
		  ret->tpc.a.txn_isolation = 0;
		}
	    }

	  XML_GET_PROP (cur, "rowsetsize", "");
	  ret->tpc.a.nRowsetSize = atoi (szProp);

	  XML_GET_PROP (cur, "keysetsize", "");
	  ret->tpc.a.nKeysetSize = atoi (szProp);

	  XML_GET_PROP (cur, "traversal", "");
	  ret->tpc.a.nTraversalCount = atoi (szProp);

	  XML_GET_PROP (cur, "type", "");
	  ret->tpc.a.fSQLOption = test_type_from_name (szProp);
	}
      else if (!strcmp ("cschema", cur->name) && ret->TestType == TPC_C)
	{
	  xmlNodePtr table, dsn;
	  int inx;

	  XML_GET_PROP (cur, "warehouses", "1");
	  ret->tpc.c.count_ware = atoi (szProp);

	  table = cur->children;
	  while (table)
	    {
	      XML_GET_PROP (table, "name", "");
	      inx = tpcc_table_index_from_name (szProp);
	      if (inx != -1)
		{
		  dsn = table->children;
		  while (dsn && xmlIsBlankNode (dsn))
		    dsn = dsn->next;
		  if (dsn && !strcmp (dsn->name, "dsn"))
		    {
		      XML_GET_PROP (dsn, "name", "");
		      strncpy (ret->tpc.c.tableDSNS[inx], szProp,
			  sizeof (ret->tpc.c.tableDSNS[inx]));

		      XML_GET_PROP (dsn, "DBMS", "");
		      strncpy (ret->tpc.c.tableDBMSes[inx], szProp,
			  sizeof (ret->tpc.c.tableDBMSes[inx]));
		    }
		}
	      table = table->next;
	    }
	}
      else if (!strcmp ("crun", cur->name) && ret->TestType == TPC_C)
	{
	  XML_GET_PROP (cur, "threads", "");
	  ret->tpc._.nThreads = atoi (szProp);
	}
      cur = cur->next;
    }
  /* do_logout (ret); */
  return ret;
}


void
do_load_test (char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr cur;

  doc = xmlParseFile (filename);
  if (doc == NULL)
    {
      pane_log ("Invalid XML document %s", filename);
      return;
    }

  cur = xmlDocGetRootElement (doc);
  if (cur == NULL)
    {
      pane_log ("Empty document %s", filename);
      xmlFreeDoc (doc);
      return;
    }
  if (strcmp (cur->name, "tests"))
    {
      pane_log ("document of the wrong type, root node != tests");
      xmlFreeDoc (doc);
      return;
    }

  cur = cur->children;
  while (cur)
    {
      if (!strcmp (cur->name, "test"))
	{
	  test_t *ret = make_test_from_node (cur);
	  if (!(gui.add_test_to_the_pool (ret)))
	    XFREE (ret);
	}
      cur = cur->next;
    }

  xmlFreeDoc (doc);
}


static xmlNodePtr
timer_account_node (timer_account_t * ta, timer_account_t * pack, xmlNsPtr ns,
    xmlNodePtr parent)
{
  char szProp[1024];
  xmlNodePtr timer = xmlNewChild (parent, ns, "timer", NULL);
  xmlAttrPtr prop;
  _xmlNewProp (timer, "name", ta->ta_name);

  if (ta->ta_n_samples && pack->ta_total)
    {
      sprintf (szProp, "%ld", ta->ta_min);
      xmlNewChild (timer, ns, "min", szProp);

      sprintf (szProp, "%ld", ta->ta_total / ta->ta_n_samples);
      xmlNewChild (timer, ns, "avg", szProp);

      sprintf (szProp, "%ld", ta->ta_max);
      xmlNewChild (timer, ns, "max", szProp);

      sprintf (szProp, "%ld", ta->ta_total);
      xmlNewChild (timer, ns, "total", szProp);

      sprintf (szProp, "%ld", ta->ta_n_samples);
      xmlNewChild (timer, ns, "samples", szProp);

      sprintf (szProp, "%ld", (100 * ta->ta_total) / (pack->ta_total));
      xmlNewChild (timer, ns, "pct", szProp);
    }

  return timer;
}

static xmlNodePtr
make_result_node (test_t * test, xmlNsPtr ns, xmlNodePtr parent)
{

  char szProp[1024];
  xmlNodePtr result, tst, res, tmr;
  xmlAttrPtr prop;
  result =
      xmlNewChild (parent, ns,
      test->TestType == TPC_C ? "cresult" : "aresult", NULL);
  if (test->szSQLState[0])
    {
      _xmlNewProp (result, "state", test->szSQLState);
      _xmlNewProp (result, "message", test->szSQLError);
    }
  else
    _xmlNewProp (result, "state", "OK");

  tst = make_test_node (test, ns, result);

  if (test->szSQLState[0])
    return result;
  switch (test->TestType)
    {
    case TPC_A:
      sprintf (szProp, "%f", test->tpc.a.dDiffSum);
      xmlNewChild (result, ns, "TransactionTime", szProp);

      sprintf (szProp, "%ld", test->tpc.a.nTrnCnt);
      xmlNewChild (result, ns, "Transactions", szProp);

      sprintf (szProp, "%ld", test->tpc.a.nTrnCnt1Sec);
      xmlNewChild (result, ns, "Transactions1Sec", szProp);

      sprintf (szProp, "%ld", test->tpc.a.nTrnCnt2Sec);
      xmlNewChild (result, ns, "Transactions2Sec", szProp);

      sprintf (szProp, "%f", test->tpc.a.ftps);
      xmlNewChild (result, ns, "TransactionsPerSec", szProp);

      sprintf (szProp, "%f", test->tpc.a.fsub1);
      xmlNewChild (result, ns, "Sub1SecPct", szProp);

      sprintf (szProp, "%f", test->tpc.a.fsub2);
      xmlNewChild (result, ns, "Sub2SecPct", szProp);

      sprintf (szProp, "%f", test->tpc.a.fAvgTPTime);
      xmlNewChild (result, ns, "AvgProcTime", szProp);
      break;

    case TPC_C:
      sprintf (szProp, "%f", test->tpc.c.tpcc_sum / test->tpc.c.nRounds);
      xmlNewChild (result, ns, "tpmC", szProp);

      sprintf (szProp, "%ld",
	  test->tpc.c.nRounds *
	  (test->tpc._.nThreads ? test->tpc._.nThreads : 1));
      xmlNewChild (result, ns, "tenpacks", szProp);

      sprintf (szProp, "%ld", test->tpc.c.local_w_id);
      xmlNewChild (result, ns, "local_w_id", szProp);

      sprintf (szProp, "%f", test->tpc.c.run_time);
      xmlNewChild (result, ns, "run_time", szProp);

      res = xmlNewChild (result, ns, "timers", NULL);
      tmr =
	  timer_account_node (&test->tpc.c.new_order_ta,
	  &test->tpc.c.ten_pack_ta, ns, res);
      tmr =
	  timer_account_node (&test->tpc.c.payment_ta,
	  &test->tpc.c.ten_pack_ta, ns, res);
      tmr =
	  timer_account_node (&test->tpc.c.delivery_ta,
	  &test->tpc.c.ten_pack_ta, ns, res);
      tmr =
	  timer_account_node (&test->tpc.c.slevel_ta,
	  &test->tpc.c.ten_pack_ta, ns, res);
      tmr =
	  timer_account_node (&test->tpc.c.ostat_ta, &test->tpc.c.ten_pack_ta,
	  ns, res);
      break;
    }
  return result;
}

void
do_save_run_results (char *filename, OList * selected, int nMinutes)
{
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlNodePtr root, result;
  char szProp[1024];
  time_t tim;
  struct tm *_tm;
  OList *iter = selected;
  xmlAttrPtr prop;

  time (&tim);
  _tm = gmtime (&tim);
  doc = xmlNewDoc ("1.0");
  xmlCreateIntSubset (doc, "run", NULL, "odbc-bench.dtd");
  ns = NULL;

  root = xmlNewDocNode (doc, ns, "run", NULL);
  sprintf (szProp, "%d", nMinutes);
  _xmlNewProp (root, "duration", szProp);
  sprintf (szProp, "%04d-%02d-%02dT%02d:%02d:%02d.%03ld",
      _tm->tm_year + 1900, _tm->tm_mon + 1, _tm->tm_mday, _tm->tm_hour,
      _tm->tm_min, _tm->tm_sec, 0L);
  _xmlNewProp (root, "end", szProp);

  xmlDocSetRootElement (doc, root);

  while (iter)
    {
      test_t *test = (test_t *) iter->data;
      result = make_result_node (test, ns, root);
      iter = o_list_next (iter);
    }
  xmlSaveFile (filename, doc);
  xmlFreeDoc (doc);
}

#if defined (PTHREADS) || defined(WIN32)
#define CONTINUE_IF_NOT_SUPPORTED(cond) \
iter = tests; \
supported = TRUE; \
while (iter) \
{ \
  test_t *test = (test_t *)iter->data; \
  if (cond) {\
    supported = FALSE; \
    break; \
  } \
  iter = o_list_next (iter); \
} \
if (!supported) \
  continue

#define FOR_ALL_TESTS(action) \
iter = tests; \
while (iter) \
{ \
  test_t *test = (test_t *)iter->data; \
  action; \
  iter = o_list_next (iter); \
}

static char *iso_names[] = {
  "/Deflt",
  "/Uncommitted",
  "/Committed",
  "/Repeatable",
  "/Serializable"
};

static char *crsr_names[] = {
  "/Fwd",
  "/Static",
  "/Keyset",
  "/Dynamic",
  "/Mixed"
};

static short grgiOption[] = {
  IDX_PLAINSQL,
  IDX_PARAMS,
  IDX_SPROCS
};
static long isolations[] = {
  SQL_TXN_DRIVER_DEFAULT,
  SQL_TXN_READ_UNCOMMITTED,
  SQL_TXN_READ_COMMITTED,
  SQL_TXN_REPEATABLE_READ,
  SQL_TXN_SERIALIZABLE
};
static long cursors[] = {
  SQL_CURSOR_FORWARD_ONLY,
  SQL_CURSOR_STATIC,
  SQL_CURSOR_KEYSET_DRIVEN,
  SQL_CURSOR_DYNAMIC,
  SQL_CURSOR_MIXED
};
static long cursor_masks[] = {
  SQL_SO_FORWARD_ONLY,
  SQL_SO_STATIC,
  SQL_SO_KEYSET_DRIVEN,
  SQL_SO_DYNAMIC,
  SQL_SO_KEYSET_DRIVEN
};

static char *grgiOptionNames[] = {
  "SQL Text",
  "Prepare & Execute using bound parameters",
  "Stored procedures"
};


int
do_threads_run_all (int nTests, OList * tests_orig, int nMinutes,
    char *filename)
{
  int fAsync;			/* Asynchronous execution */
  int fQuery;			/* Execute query option */
  int fTrans;			/* Execute query option */
  int nOption;			/* Index for sql options */
  int nIsolation;		/* Index for sql options */
  int nCursor;			/* Index for sql options */
  OList *iter = tests_orig, *tests = NULL;
  int supported;
  xmlAttrPtr prop;

  char szTemp[128];
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlNodePtr root;
  char szProp[1024];
  time_t tim;
  struct tm *_tm;

  time (&tim);
  _tm = gmtime (&tim);
  doc = xmlNewDoc ("1.0");
  xmlCreateIntSubset (doc, "run", NULL, "odbc-bench.dtd");
  ns = NULL;

  root = xmlNewDocNode (doc, ns, "run", NULL);
  sprintf (szProp, "%d", nMinutes);
  _xmlNewProp (root, "duration", szProp);
  sprintf (szProp, "%04d-%02d-%02dT%02d:%02d:%02d.%03ld",
      _tm->tm_year + 1900, _tm->tm_mon + 1, _tm->tm_mday, _tm->tm_hour,
      _tm->tm_min, _tm->tm_sec, 0L);
  _xmlNewProp (root, "end", szProp);

  xmlDocSetRootElement (doc, root);

  while (iter)
    {
      test_t *tst = malloc (sizeof (test_t));
      memcpy (tst, iter->data, sizeof (test_t));
      tests = o_list_append (tests, tst);
      iter = o_list_next (iter);
    }

  for (fAsync = FALSE; fAsync <= TRUE; fAsync++)
    {
      /* If async is not supported, then skip this try */
      CONTINUE_IF_NOT_SUPPORTED ((!test->fAsyncSupported && fAsync));
      FOR_ALL_TESTS (test->tpc.a.fExecAsync = fAsync);

      for (fTrans = FALSE; fTrans <= TRUE; fTrans++)
	{
	  /* If transactions are not supported, then skip them */
	  CONTINUE_IF_NOT_SUPPORTED ((!test->fCommitSupported && fTrans));
	  FOR_ALL_TESTS (test->tpc.a.fUseCommit = fTrans);

	  /* Vary the use of the 100 row query */
	  for (fQuery = FALSE; fQuery <= TRUE; fQuery++)
	    {
	      FOR_ALL_TESTS (test->tpc.a.fDoQuery = fQuery);

	      for (nIsolation = 0; nIsolation < 5; nIsolation++)
		{
		  if (!fQuery && nIsolation > 0)
		    continue;
		  if (fQuery)
		    {
		      CONTINUE_IF_NOT_SUPPORTED (!(test->nIsolationsSupported
			      & isolations[nIsolation]));
		      FOR_ALL_TESTS (test->tpc.a.txn_isolation =
			  isolations[nIsolation];
			  test->tpc.a.nRowsetSize = 30;
			  test->tpc.a.nKeysetSize = 30;
			  test->tpc.a.nTraversalCount = 3);
		    }

		  for (nCursor = 0; nCursor < 5; nCursor++)
		    {
		      if (!fQuery && nCursor > 0)
			continue;
		      if (fQuery)
			{
			  CONTINUE_IF_NOT_SUPPORTED (!(test->nCursorsSupported
				  & cursor_masks[nCursor]));
			  FOR_ALL_TESTS (test->tpc.a.nCursorType =
			      cursors[nCursor]);
			  if (nCursor == 4)
			    {
			      FOR_ALL_TESTS (test->tpc.a.nKeysetSize = 60);
			    }
			}

		      /* Loop around the SQL options */
		      for (nOption = 0; nOption < NUMITEMS (grgiOption);
			  nOption++)
			{
			  CONTINUE_IF_NOT_SUPPORTED ((test->tpc.a.fSQLOption ==
				  IDX_SPROCS && !test->fProcsSupported));
			  /* If sprocs are not supported, then we have to skip */
			  FOR_ALL_TESTS (test->tpc.a.fSQLOption =
			      grgiOption[nOption]);

			  /* All options are set, so do the run */
			      sprintf (szTemp, "%s%s%s%s%s%s for",
			      grgiOptionNames[nOption],
			      (fAsync ? "/Async" : ""),
			      (fQuery ? "/Query" : ""),
			      (fTrans ? "/Trans" : ""),
			      (fQuery ? iso_names[nIsolation] : ""),
			      (fQuery ? crsr_names[nCursor] : ""));
			  if (!do_threads_run (nTests, tests, nMinutes,
				  szTemp))
			    {
			      FOR_ALL_TESTS (if (test->szSQLError[0])
				  make_result_node (test, ns, root));
			      goto end;
			    }
			  else
			    {
			      FOR_ALL_TESTS (make_result_node (test, ns,
				      root));
			    }
			}
		    }
		}		/* SQL options */
	    }			/* Execution query  */
	}			/* Transactions */
    }				/* Async */
end:
  FOR_ALL_TESTS (XFREE (test));
  o_list_free (tests);
  xmlSaveFile (filename, doc);
  xmlFreeDoc (doc);
  return TRUE;
}


void
DoRunAll (test_t * test_orig, char *filename)
{
  int fAsync;			/* Asynchronous execution */
  int fQuery;			/* Execute query option */
  int fTrans;			/* Execute query option */
  int nOption;			/* Index for sql options */
  int nIsolation;		/* Index for sql options */
  int nCursor;			/* Index for sql options */
  char szTemp[128];
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlNodePtr root;
  char szProp[1024];
  time_t tim;
  struct tm *_tm;
  test_t *test;
  xmlAttrPtr prop;

  test = malloc (sizeof (test_t));
  memcpy (test, test_orig, sizeof (test_t));

  time (&tim);
  _tm = gmtime (&tim);
  doc = xmlNewDoc ("1.0");
  xmlCreateIntSubset (doc, "run", NULL, "odbc-bench.dtd");
  ns = NULL;

  root = xmlNewDocNode (doc, ns, "run", NULL);
  sprintf (szProp, "%d", test->tpc._.nMinutes);
  _xmlNewProp (root, "duration", szProp);
  sprintf (szProp, "%04d-%02d-%02dT%02d:%02d:%02d.%03ld",
      _tm->tm_year + 1900, _tm->tm_mon + 1, _tm->tm_mday, _tm->tm_hour,
      _tm->tm_min, _tm->tm_sec, 0L);
  _xmlNewProp (root, "end", szProp);

  xmlDocSetRootElement (doc, root);

  for (fAsync = FALSE; fAsync <= TRUE; fAsync++)
    {
      /* If async is not supported, then skip this try */
      if ((!test->fAsyncSupported && fAsync))
	continue;
      test->tpc.a.fExecAsync = fAsync;

      for (fTrans = FALSE; fTrans <= TRUE; fTrans++)
	{
	  /* If transactions are not supported, then skip them */
	  if ((!test->fCommitSupported && fTrans))
	    continue;
	  test->tpc.a.fUseCommit = fTrans;

	  /* Vary the use of the 100 row query */
	  for (fQuery = FALSE; fQuery <= TRUE; fQuery++)
	    {
	      test->tpc.a.fDoQuery = fQuery;

	      for (nIsolation = 0; nIsolation < 5; nIsolation++)
		{
		  if (!fQuery && nIsolation > 0)
		    continue;
		  if (fQuery)
		    {
		      if (!(test->nIsolationsSupported &
			      isolations[nIsolation]))
			continue;
		    }
		  test->tpc.a.txn_isolation = isolations[nIsolation];

		  for (nCursor = 0; nCursor < 5; nCursor++)
		    {
		      if (!fQuery && nCursor > 0)
			continue;
		      if (fQuery)
			{
			  if (!(test->nCursorsSupported &
				  cursor_masks[nCursor]))
			    continue;
			}
		      test->tpc.a.nCursorType = cursors[nCursor];

		      /* Loop around the SQL options */
		      for (nOption = 0; nOption < NUMITEMS (grgiOption);
			  nOption++)
			{
			  /* If sprocs are not supported, then we have to skip */
			  test->tpc.a.fSQLOption = grgiOption[nOption];
			  if ((test->tpc.a.fSQLOption == IDX_SPROCS
				  && !test->fProcsSupported))
			    continue;

			  /* All options are set, so do the run */
			      sprintf (szTemp, "%s%s%s%s%s%s for",
			      grgiOptionNames[nOption],
			      (fAsync ? "/Async" : ""),
			      (fQuery ? "/Query" : ""),
			      (fTrans ? "/Trans" : ""),
			      (fQuery ? iso_names[nIsolation] : ""),
			      (fQuery ? crsr_names[nCursor] : ""));
			  if (!DoRun (test, szTemp))
			    {
			          if (test->szSQLError[0]) {
				make_result_node (test, ns, root);
				    test->szSQLError[0] = '\0';
			          } if (gui.isCancelled())
			      goto end;
			    }
			  else
			    {
			      make_result_node (test, ns, root);
			    }
			}
		    }
		}		/* SQL options */
	    }			/* Execution query  */
	}			/* Transactions */
    }				/* Async */
end:
  XFREE (test);
  xmlSaveFile (filename, doc);
  xmlFreeDoc (doc);
}
#endif