
lib_LTLIBRARIES = librcengine.la
librcengine_la_SOURCES = \
    syntax.y       \
    actions.cpp    \
    callbacks.cpp  \
    classes.cpp    \
    compound.cpp   \
    confset.cpp    \
    dasm_rete.cpp  \
    eng.cpp        \
    error.cpp      \
    expr.cpp       \
    functions.cpp  \
    lex.cpp        \
    load.cpp       \
    metaobj.cpp    \
    nodes.cpp      \
    nodes_exec.cpp \
    patterns.cpp   \
    primit.cpp     \
    rules.cpp      \
    set.cpp        \
    single.cpp     \
    strlow.cpp     \
    btree.cpp      \
    keys.cpp       \
    vars.cpp

HEADERS_DIR=./hdrs
librcengine_la_CFLAGS = -I$(HEADERS_DIR)
librcengine_la_CXXFLAGS = -I$(HEADERS_DIR)
librcengine_la_YFLAGS = -d -p eng_
librcengine_la_LDFLAGS = -version-info 1:0:0

include_HEADERS = $(HEADERS_DIR)/engine.h $(HEADERS_DIR)/btree.hpp
