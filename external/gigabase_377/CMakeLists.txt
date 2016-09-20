cmake_minimum_required(VERSION 2.4)

if( COMMAND cmake_policy )
cmake_policy(SET CMP0003 NEW)
endif (COMMAND cmake_policy)

PROJECT(GIGABASE)

add_definitions(-DUSE_NAMESPACES)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

SET(GB_SRCS
blob.cpp
btree.cpp
class.cpp
compiler.cpp
container.cpp
cursor.cpp
database.cpp
exception.cpp
file.cpp
forcerecovery.cpp
hashtab.cpp
localcli.cpp
pagepool.cpp
query.cpp
replicator.cpp
rtree.cpp
server.cpp
session.cpp
symtab.cpp
sync.cpp
unisock.cpp
wwwapi.cpp
w32sock.cpp 
)

file(GLOB_RECURSE GB_HDRS *.h)

ADD_LIBRARY( gigabase STATIC ${GB_SRCS} ${GB_HDRS} )

target_compile_features(gigabase PRIVATE cxx_range_for)
