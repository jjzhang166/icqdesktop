/*-< CLITEST.C >-----------------------------------------------------*--------*
 * GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
 * (Post Relational Database Management System)                      *   /\|  *
 *                                                                   *  /  \  *
 *                          Created:     13-Jan-2000 K.A. Knizhnik   * / [] \ *
 *                          Last update: 13-Jan-2000 K.A. Knizhnik   * GARRET *
 *-------------------------------------------------------------------*--------*
 * Test for GigaBASE call level interface
 * Spawn "subsql  clitest.sql" to start CLI server.
 *-------------------------------------------------------------------*--------*/

#include "cli.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PETS 8

typedef struct  person {
    char_t      name[64];
    cli_int8_t  salary;
    char_t*     address;
    cli_real8_t rating;
    cli_int4_t  n_subordinates;
    cli_oid_t*  subordinates;
    int         n_pets;
    char_t*     pets[MAX_PETS];
} person;

void* set_subordinates(int var_type, void* var_ptr, int len)
{
    person* p = (person*)var_ptr;
    if (p->subordinates != NULL) {
	free(p->subordinates);
    }
    p->n_subordinates = len;
    p->subordinates = (cli_oid_t*)malloc(len*sizeof(cli_oid_t));
    return p->subordinates;
}

void* get_subordinates(int var_type, void* var_ptr, int* len)
{
    person* p = (person*)var_ptr;
    *len = p->n_subordinates;
    return p->subordinates;
}

static cli_field_descriptor person_descriptor[] = {
    {cli_asciiz, cli_hashed|cli_unique, _T("name")},
    {cli_int4, cli_indexed, _T("salary")},
    {cli_pasciiz, 0, _T("address")}, 
    {cli_real8, 0, _T("rating")}, 
    {cli_array_of_string, 0, _T("pets")},
    {cli_array_of_oid, 0, _T("subordinates"), _T("person")}
}; 


int __cdecl main()
{
    char* serverURL = "localhost:6100";
    char_t* databaseName = _T("clitest.dbs");
    int session, statement, statement2, rc, len;
    int table_created = 0;
    int i, n, salary;
    char_t name[256];
    char_t address[256];
    cli_oid_t oid;
    person p;

    session = cli_open(serverURL, 10, 1, _T("guest"), _T(""), 1);
    if (session == cli_bad_address) { 
	session = cli_create(databaseName, 0, 0, 0);
    }
    if (session < 0) {
	fprintf(stderr, "cli_open failed with code %d\n", session);
	return EXIT_FAILURE;
    }

    rc = cli_create_table(session, _T("person"), sizeof(person_descriptor)/sizeof(cli_field_descriptor), 
			  person_descriptor);
    if (rc == cli_ok) { 
	table_created = 1;
        rc = cli_alter_index(session, _T("person"), _T("salary"), cli_indexed); 
        if (rc != cli_ok) { 
            fprintf(stderr, "cli_alter_index failed with code %d\n", rc);
            return EXIT_FAILURE;
        }
        rc = cli_alter_index(session, _T("person"), _T("name"), cli_indexed); 
        if (rc != cli_ok) { 
            fprintf(stderr, "cli_alter_index 2 failed with code %d\n", rc);
            return EXIT_FAILURE;
        }
     } else if (rc != cli_table_already_exists && rc != cli_not_implemented) { 
	fprintf(stderr, "cli_create_table failed with code %d\n", rc);
	return EXIT_FAILURE;
    } 
	
    statement = cli_statement(session, _T("insert into person"));
    if (statement < 0) {
	fprintf(stderr, "cli_statement failed with code %d\n", statement);
	return EXIT_FAILURE;
    }

    if ((rc=cli_column(statement, _T("name"), cli_asciiz, NULL, p.name)) != cli_ok
	|| (rc=cli_column(statement, _T("salary"), cli_int8, NULL, &p.salary)) != cli_ok
	|| (rc=cli_column(statement, _T("address"), cli_pasciiz, &len, &p.address)) != cli_ok
	|| (rc=cli_column(statement, _T("rating"), cli_real8, NULL, &p.rating)) != cli_ok
	|| (rc=cli_column(statement, _T("pets"), cli_array_of_string, &p.n_pets, p.pets)) != cli_ok
	|| (rc=cli_array_column(statement, _T("subordinates"), cli_array_of_oid, &p,
				set_subordinates, get_subordinates)) != cli_ok)
    {
	fprintf(stderr, "cli_column 1 failed with code %d\n", rc);
	return EXIT_FAILURE;
    }
    STRCPY(p.name, _T("John Smith"));
    p.salary = 75000;
    p.address = _T("1 Guildhall St., Cambridge CB2 3NH, UK");
    p.rating = 80.3;
    p.n_subordinates = 0;
    p.subordinates = NULL;
    p.pets[0] = _T("dog");
    p.pets[1] = _T("cat");
    p.n_pets = 2;
    rc = cli_insert(statement, &oid);
    if (rc != cli_ok) {
	fprintf(stderr, "cli_insert failed with code %d\n", rc);
	return EXIT_FAILURE;
    }

    STRCPY(p.name, _T("Joe Cooker"));
    p.salary = 100000;
    p.address = _T("Outlook drive, 15/3");
    p.rating = 80.3;
    p.n_subordinates = 1;
    p.subordinates = &oid;
    p.pets[0] = _T("snake");
    p.n_pets = 1;
    rc = cli_insert(statement, NULL);
    if (rc != cli_ok) {
        fprintf(stderr, "cli_insert 2 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }

    rc = cli_free(statement);
    if (rc != cli_ok) {
        fprintf(stderr, "cli_free failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    p.subordinates = NULL;
    statement = cli_statement(session,
                              _T("select * from person where ")
                              _T("length(subordinates) < %subordinates and salary > %salary"));
    if (statement < 0) {
        fprintf(stderr, "cli_statement 2 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    p.address = address;
    len = sizeof(address);
    if ((rc=cli_column(statement, _T("name"), cli_asciiz, NULL, p.name)) != cli_ok
        || (rc=cli_column(statement, _T("salary"), cli_int8, NULL, &p.salary)) != cli_ok
        || (rc=cli_column(statement, _T("address"), cli_pasciiz, &len, &p.address)) != cli_ok
        || (rc=cli_column(statement, _T("rating"), cli_real8, NULL, &p.rating)) != cli_ok
        || (rc=cli_column(statement, _T("pets"), cli_array_of_string, &p.n_pets, p.pets)) != cli_ok
        || (rc=cli_array_column(statement, _T("subordinates"), cli_array_of_oid, &p,
                                set_subordinates, get_subordinates)) != cli_ok)
    {
        fprintf(stderr, "cli_column 2 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if ((rc = cli_parameter(statement, _T("%subordinates"), cli_int4, &n)) != cli_ok
        || (rc = cli_parameter(statement, _T("%salary"), cli_int4, &salary)) != cli_ok)
    {
        fprintf(stderr, "cli_parameter failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    n = 2;
    salary = 90000;
    rc = cli_fetch(statement, cli_cursor_view_only);
    if (rc != 1) {
        fprintf(stderr, "cli_fetch 1 returns %d instead of 1\n", rc);
        cli_commit(session);
        return EXIT_FAILURE;
    }
    n = 10;
    salary = 50000;
    rc = cli_fetch(statement, cli_cursor_for_update);
    if (rc != 2) {
        fprintf(stderr, "cli_fetch 2 returns %d instead of 2\n", rc);
        return EXIT_FAILURE;
    }
    statement2 = cli_statement(session, _T("select * from person where current = %oid"));
    if (statement2 < 0) {
        fprintf(stderr, "cli_statement 3 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }

    if ((rc=cli_column(statement2, _T("name"), cli_asciiz, NULL, name)) != cli_ok) {
        fprintf(stderr, "cli_column 3 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if ((rc = cli_parameter(statement2, _T("%oid"), cli_oid, &oid)) != cli_ok) {
        fprintf(stderr, "cli_parameter 3 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }

    p.n_pets = MAX_PETS;
    while ((rc = cli_get_next(statement)) == cli_ok) {
        PRINTF(_T("%s\t%ld\t%f\t%s\n"), p.name, (long)p.salary, p.rating, p.address);
        if (p.n_pets != 0) { 
            printf("Pets:\n");
            for (i = 0; i < p.n_pets; i++) { 
                PRINTF(_T("%s\n"), p.pets[i]);
            }
        } 
        if (p.n_subordinates > 0) {
            printf("Manages:\n");
            for (i = 0; i < p.n_subordinates; i++) {
                oid = p.subordinates[i];
                rc = cli_fetch(statement2, cli_cursor_view_only);
                if (rc != 1) {
                    fprintf(stderr, "cli_fetch by oid failed with code %d\n", rc);
                    return EXIT_FAILURE;
                }
                if ((rc = cli_get_first(statement2)) != cli_ok) {
                    fprintf(stderr, "cli_get_first failed with code %d\n", rc);
                    return EXIT_FAILURE;
                }
                PRINTF(_T("\t%s\n"), name);
            }
        }
        len = sizeof(address);
        p.salary = p.salary*90/100;
        rc = cli_update(statement);
        if (rc != cli_ok) {
            fprintf(stderr, "cli_update failed with code %d\n", rc);
            return EXIT_FAILURE;
        }
        p.n_pets = MAX_PETS;
    }
    if (rc != cli_not_found) {
        fprintf(stderr, "cli_get_next failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if ((rc = cli_free(statement)) != cli_ok ||
        (rc = cli_free(statement2)) != cli_ok)
    {
        fprintf(stderr, "cli_free 2 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if ((rc = cli_commit(session)) != cli_ok) {
        fprintf(stderr, "cli_commit failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    statement = cli_statement(session, _T("select * from person order by salary"));
    if (statement < 0) {
        fprintf(stderr, "cli_statement 4 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if ((rc=cli_column(statement, _T("salary"), cli_int4, NULL, &salary)) != cli_ok) {
        fprintf(stderr, "cli_column 4 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    rc = cli_fetch(statement, cli_cursor_for_update);
    if (rc != 2) {
        fprintf(stderr, "cli_fetch 4 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    printf("New salaries:\n");
    while ((rc = cli_get_prev(statement)) == cli_ok) {
        printf("\t%d\n", salary);
    }
    if (rc != cli_not_found) {
        fprintf(stderr, "cli_get_prev failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if ((rc = cli_remove(statement)) != cli_ok) {
        fprintf(stderr, "cli_remove failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if ((rc = cli_free(statement)) != cli_ok) {
        fprintf(stderr, "cli_free 3 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    if (table_created) { 
        /*
        rc = cli_drop_table(session, _T("person"));
        if (rc != cli_ok) { 
            fprintf(stderr, "cli_drop_table failed with code %d\n", rc);
            return EXIT_FAILURE;
        }
        */
    }    
    if ((rc = cli_close(session)) != cli_ok) {
        fprintf(stderr, "cli_close failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    printf("*** CLI test sucessfully passed!\n");
    return EXIT_SUCCESS;
}


