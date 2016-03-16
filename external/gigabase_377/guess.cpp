//-< GUESS.CPP >-----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     10-Dec-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 19-Dec-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Sample of database application: game "Guess an animal"
//-------------------------------------------------------------------*--------*

#include "gigabase.h"

#ifdef SUPPORT_DATA_ENCRYPTION
#include "crypt/crypt_file.h"
#endif

USE_GIGABASE_NAMESPACE

const int maxStrLen = 256;

class Guess {
  public:
    dbReference<Guess> yes;
    dbReference<Guess> no;
    char const* question;

    TYPE_DESCRIPTOR((FIELD(yes), FIELD(question), FIELD(no)));
};

REGISTER(Guess);

void input(char const* prompt, char* buf, size_t buf_size)
{
    char* p;
    do {
        printf(prompt);
        *buf = '\0';
        fgets(buf, (int)buf_size, stdin);
        p = buf + strlen(buf);
    } while (p <= buf+1);

    if (*(p-1) == '\n') {
        *--p = '\0';
    }
}

bool askQuestion(char const* question) {
    char answer[maxStrLen];
    input(question, answer, sizeof answer);
    return *answer == 'y' || *answer == 'Y';
}


dbReference<Guess> whoIsIt(dbReference<Guess> const& parent) {
    char animal[maxStrLen];
    char difference[maxStrLen];
    input("What is it ? ", animal, sizeof animal);
    input("What is a difference from other ? ", difference, sizeof difference);
    Guess node;
    node.question = animal;
    dbReference<Guess> child = insert(node);
    node.question = difference;
    node.yes = child;
    node.no = parent;
    return insert(node);
}


dbReference<Guess> dialog(dbCursor<Guess>& cur) {
    char question[maxStrLen+16];
    dbCursor<Guess> c(dbCursorForUpdate);
    sprintf(question, "May be %s (y/n) ? ", cur->question);
    if (askQuestion(question)) {
        if (cur->yes == null) {
            printf("It was very simple question for me...\n");
        } else {
            c.at(cur->yes);
            dbReference<Guess> clarify = dialog(c);
            if (clarify != null) {
                cur->yes = clarify;
                cur.update();
            }
        }
    } else {
        if (cur->no == null) {
            if (cur->yes == null) {
                return whoIsIt(cur.currentId());
            } else {
                cur->no = whoIsIt(null);
                cur.update();
            }
        } else {
            c.at(cur->no);
            dbReference<Guess> clarify = dialog(c);
            if (clarify != null) {
                cur->no = clarify;
                cur.update();
            }
        }
    }
    return null;
}


int __cdecl main(int argc, char* argv[])
{
    dbDatabase db(dbDatabase::dbMulticlientReadWrite);
#ifdef SUPPORT_DATA_ENCRYPTION
    dbCryptFile cf("KEY");
    dbCryptFile bck("KEY");
    int rc = cf.open(_T("guess.dbs"), 0);
    if (rc != dbFile::ok) { 
        char_t buf[256];
        cf.errorText(rc, buf, itemsof(buf));
        FPRINTF(stderr, _T("Failed to open encrupted file: %s\n"), buf);
        return EXIT_FAILURE;
    }
    if (db.open(&cf)) {
#else
    if (db.open(STRLITERAL("guess.dbs"))) {
#endif
        dbCursor<Guess> cur(dbCursorForUpdate);
#ifdef TEST_XML_IMPORT
        { 
            FILE* f = fopen("guess.xml", "r");
            if (f != NULL) { 
                db.importDatabaseFromXml(f);
                fclose(f);
            }
        }
#endif
        //db.scheduleBackup(STRLITERAL("guess.bck?"), 10);
        //db.scheduleBackup(STRLITERAL("guess.bc"), 10);
        while (askQuestion("Think of an animal. Ready (y/n) ? ")) {
            if (cur.select() != 0) {
                cur.next(); // first question is in record number 2
                dialog(cur);
            } else {
                whoIsIt(null);
            }
            db.commit();
        }
#ifdef SUPPORT_DATA_ENCRYPTION 
        rc = bck.open(_T("guess.bck"), 0);
        if (rc != dbFile::ok) { 
            char_t buf[256];
            bck.errorText(rc, buf, itemsof(buf));
            FPRINTF(stderr, _T("Failed to open backup file: %s\n"), buf);
            return EXIT_FAILURE;
        }
        db.backup(&bck, false);
#endif
#ifdef TEST_XML_IMPORT
        { 
            FILE* f = fopen("guess.xml", "w");
            if (f != NULL) { 
                db.exportDatabaseToXml(f, NULL, 0, dbDatabase::sel_all);
                fclose(f);
            }
        }
#endif
        db.close();
        printf("End of the game\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Failed to open database\n");
        return EXIT_FAILURE;
    }
}

