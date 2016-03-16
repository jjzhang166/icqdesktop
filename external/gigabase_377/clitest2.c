/*-< CLITEST2.C >-----------------------------------------------------*--------*
 * GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
 * (Post Relational Database Management System)                      *   /\|  *
 *                                                                   *  /  \  *
 *                          Created:     18-Jun-2003 K.A. Knizhnik   * / [] \ *
 *                          Last update: 18-Jun-2003 K.A. Knizhnik   * GARRET *
 *-------------------------------------------------------------------*--------*
 * Test for extended FastDB local call level interface 
 *-------------------------------------------------------------------*--------*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"

typedef struct  person { 
    char*       name;
    cli_int8_t  salary;
    char*       address;
    cli_real8_t rating;
    cli_array_t subordinates;
    cli_array_t pets;
} person;

static cli_field_descriptor person_descriptor[] = {
    {cli_asciiz, cli_hashed, _T("name")},
    {cli_int8, cli_indexed, _T("salary")},
    {cli_pasciiz, 0, _T("address")}, 
    {cli_real8, 0, _T("rating")}, 
    {cli_array_of_oid, 0, _T("subordinates"), _T("persons")},
    {cli_array_of_string, 0, _T("pets")},
}; 


int main()
{
    char_t* filePath = _T("clitest2.dbs");
    int statement, statement2;
    int session;
    int table_created = 0;
    int rc;
    size_t i;
    cli_oid_t oid;
    char_t* pets[2];
    person p, p2;

    memset(&p, 0, sizeof(p));   /* correctly initialize array fields */
    memset(&p2, 0, sizeof(p2)); /* correctly initialize array fields */

    session = cli_create(filePath, 0, 0, 0);
    if (session < 0) { 
        fprintf(stderr, "cli_open failed with code %d\n", session);
        return EXIT_FAILURE;
    }

    rc = cli_create_table(session, _T("persons"), sizeof(person_descriptor)/sizeof(cli_field_descriptor), 
			  person_descriptor);
    if (rc == cli_ok) { 
	table_created = 1;
        rc = cli_alter_index(session, _T("persons"), _T("salary"), cli_indexed); 
        if (rc != cli_ok) { 
            fprintf(stderr, "cli_alter_index failed with code %d\n", rc);
            return EXIT_FAILURE;
        }
        rc = cli_alter_index(session, _T("persons"), _T("name"), cli_indexed); 
        if (rc != cli_ok) { 
            fprintf(stderr, "cli_alter_index 2 failed with code %d\n", rc);
            return EXIT_FAILURE;
        }
    } else if (rc != cli_table_already_exists && rc != cli_not_implemented) { 
	fprintf(stderr, "cli_create_table failed with code %d\n", rc);
	return EXIT_FAILURE;
    } 

    p.name = "John Smith";
    p.salary = 75000;
    p.address = "1 Guildhall St., Cambridge CB2 3NH, UK";
    p.rating = 80.3;
    p.subordinates.size = 0;
    p.subordinates.allocated = 0;
    pets[0] = _T("dog");
    pets[1] = _T("cat");
    p.pets.size = 2;
    p.pets.data = pets;
    rc = cli_insert_struct(session, _T("persons"), &p, &oid);
    if (rc != cli_ok) { 
        fprintf(stderr, "cli_insert failed with code %d\n", rc);
        return EXIT_FAILURE;
    }

    p.name = "Joe Cooker";
    p.salary = 100000;
    p.address = "Outlook drive, 15/3";
    p.rating = 60.5;
    p.subordinates.size = 1;
    p.subordinates.data = &oid;
    pets[0] = _T("snake");
    p.pets.size = 1;
    p.pets.data = pets;
    rc = cli_insert_struct(session, _T("persons"), &p, &oid);
    if (rc != cli_ok) { 
        fprintf(stderr, "cli_insert 2 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }

    statement = cli_prepare_query(session, 
                                  _T("select * from persons where length(subordinates) < %i and salary > %li"));
    if (statement < 0) { 
        fprintf(stderr, "cli_statement 2 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    rc = cli_execute_query(statement, cli_cursor_view_only, &p, 2, (cli_int8_t)90000);
    if (rc != 1) { 
        fprintf(stderr, "cli_fetch 1 returns %d instead of 1\n", rc);
        return EXIT_FAILURE;
    }
    rc = cli_execute_query(statement, cli_cursor_for_update, &p, 10, (cli_int8_t)50000);
    if (rc != 2) { 
        fprintf(stderr, "cli_fetch 2 returns %d instead of 2\n", rc);
        return EXIT_FAILURE;
    }
    statement2 = cli_prepare_query(session, _T("select * from persons where current = %p"));
    if (statement2 < 0) { 
        fprintf(stderr, "cli_statement 3 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }
    while ((rc = cli_get_next(statement)) == cli_ok) { 
        printf("%s\t%ld\t%f\t%s\n", p.name, (long)p.salary, p.rating, p.address);
        if (p.pets.size != 0) { 
            printf("Pets:\n");
            for (i = 0; i < p.pets.size; i++) { 
                PRINTF(_T("%s\n"), *((char_t**)p.pets.data + i));
            }
        } 
        if (p.subordinates.size > 0) { 
            printf("Manages:\n");
            for (i = 0; i < p.subordinates.size; i++) { 
                rc = cli_execute_query(statement2, cli_cursor_view_only, &p2, *((cli_oid_t*)p.subordinates.data + i)); 
                if (rc != 1) { 
                    fprintf(stderr, "cli_fetch by oid failed with code %d\n", rc);
                    return EXIT_FAILURE;
                }	
                if ((rc = cli_get_first(statement2)) != cli_ok) { 
                    fprintf(stderr, "cli_get_first failed with code %d\n", rc);
                    return EXIT_FAILURE;
                }
                printf("\t%s\n", p2.name);
            }
        }
        p.salary = p.salary*90/100;
        rc = cli_update(statement);
        if (rc != cli_ok) { 
            fprintf(stderr, "cli_update failed with code %d\n", rc);
            return EXIT_FAILURE;
        }
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
    statement = cli_prepare_query(session, _T("select * from persons order by salary"));
    if (statement < 0) { 
        fprintf(stderr, "cli_statement 4 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }	
    rc = cli_execute_query(statement, cli_cursor_for_update, &p);
    if (rc != 2) { 
        fprintf(stderr, "cli_fetch 4 failed with code %d\n", rc);
        return EXIT_FAILURE;
    }	
    printf("New salaries:\n");
    while ((rc = cli_get_prev(statement)) == cli_ok) { 
        printf("\t%d\n", (int)p.salary);
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
	rc = cli_drop_table(session, _T("persons"));
	if (rc != cli_ok) { 
	    fprintf(stderr, "cli_drop_table failed with code %d\n", rc);
	    return EXIT_FAILURE;
	}
    }    

    if ((rc = cli_close(session)) != cli_ok) { 
        fprintf(stderr, "cli_close failed with code %d\n", rc);
        return EXIT_FAILURE;	
    }
    printf("*** CLI test sucessfully passed!\n");
    return EXIT_SUCCESS;
}
	

