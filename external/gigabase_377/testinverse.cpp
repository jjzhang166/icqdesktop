#include "gigabase.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

USE_GIGABASE_NAMESPACE

class Node {
  public:
    Node() {}
    Node(const char* n):name(n) {}
        
    const char*                   name;
    dbReference<Node>           parent;
    dbArray<dbReference<Node> > childs;

    TYPE_DESCRIPTOR((FIELD(name),
                     RELATION(parent,childs),
                     RELATION(childs,parent)));
};
REGISTER(Node);

int main(int argc, char* argv[])
{
    dbDatabase db;
    dbCursor<Node> cur;
    
    ::remove("test.dbs");
    db.open("test.dbs");

    Node child1("child1");
    Node child2("child2");
    
    dbReference<Node> ref_child1 = insert(child1);
    dbReference<Node> ref_child2 = insert(child2);
    
    Node parent("parent1");
    
    parent.childs.append(ref_child1);
    parent.childs.append(ref_child2);
    dbReference<Node> ref_parent1 = insert(parent);
    
    assert(cur.at(ref_parent1)->childs.length() == 2);
    
    {
        dbCursor<Node> cur_update(dbCursorForUpdate);
        cur_update.select();
        cur_update.seek(ref_child2);
        cur_update->parent = null;
        cur_update.update();
    }
    assert(cur.at(ref_parent1)->childs.length() == 1);

    parent.childs.resize(0);
    parent.childs.append(ref_child1);
    dbReference<Node> ref_parent2 = insert(parent);

    assert(cur.at(ref_parent2)->childs.length() == 1);
    assert(cur.at(ref_parent1)->childs.length() == 0);
    db.close();
    return 0;
}
