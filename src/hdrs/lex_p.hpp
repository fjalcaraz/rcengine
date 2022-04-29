/**
 * @file lex_p.hpp
 * @author Francisco Alcaraz
 * @brief Private Functions defined at the lex module
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef ERROR
#define ERROR   -1
#endif


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "engine.h"

#include "codes.h"
#include "syntax.hpp"
#include "error.hpp"
#include "config.hpp"
#include "rules.hpp"
#include "lex.hpp"
#include "librcengine_la-syntax.h"

#ifndef ANALEX_P_H_INCLUDED
#define ANALEX_P_H_INCLUDED


#define SINTAX_ERROR 1000


typedef struct {
  int token;
  const char *txt;
} TokenText_Type;
 

static char *input;
static int line_num = 1;
 
static TokenText_Type TokenText[] = {
        { _PACKAGE,             "package"       },
        { RULESET,              "ruleset"       },
        { END,                  "end"           },
        { RULE,                 "rule"          },
        { CLASS,                "class"         },
        { TRIGGER,              "trigger"       },
        { TEMPORAL,             "temporal"      },
        { PERMANENT,            "permanent"     },
        { TIMED,                "timed"         },
        { UNTIMED,              "untimed"       },
        { FUNCTION,             "function"      },
        { WINDOW,               "window"        },
        { PROCEDURE,            "procedure"     },
        { IS_A,                 "is_a"          },
        { RESTRICTS,            "restricts"     },
        { ABSTRACT,             "abstract"      },
        { INTEGER_DEF,          "integer"       },
        { FLOAT_DEF,            "float"         },
        { CHAR_DEF,             "char"          },
        { STRING_DEF,           "string"        },
        { BOOLEAN_DEF,          "boolean"       },
        { OBJECT_DEF,           "object"        },
        { CAT_HIGH,             "high"          },
        { CAT_NORMAL,           "normal"        },
        { CAT_LOW,              "low"           },
        { COUNT_SET,            "count"         },
        { PROD_SET,             "prod"          },
        { SUM_SET,              "sum"           },
        { MIN_SET,              "min"           },
        { MAX_SET,              "max"           },
        { CONCAT_SET,           "concat"        },
        { TIME_FUN,             "time"          },
        { CREATE,               "create"        },
        { MODIFY,               "modify"        },
        { CHANGE,               "change"        },
        { DELETE,               "delete"        },
        { CALL,                 "call"          },
	{ TRUE_VAL,             "true"          },
	{ FALSE_VAL,            "false"         },
	{ NULL_STR,             "null"          },
        { ON,                   "on"            },
        { INSERT,               "insert"        },
        { RETRACT,              "retract"       },
        { 0,                    NULL            }
};



PRIVATE void first_printable(char &c);
PRIVATE int read_ident(char &c);
PRIVATE int read_number(char &c);
PRIVATE int read_char(char &c);
PRIVATE int read_str(char &c);
PRIVATE int read_special(char &c);


#endif
