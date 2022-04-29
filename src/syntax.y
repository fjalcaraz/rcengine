%{

/**
 * @file syntax.y
 * @author Francisco Alcaraz
 * @brief Syntactic Analyzer of the language
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "engine.h"

#include "codes.h"
#include "classes.hpp"
#include "patterns.hpp"
#include "functions.hpp"
#include "lex.hpp"
#include "rules.hpp"
#include "expr.hpp"
#include "syntax.hpp"

%}


%token PKG_ID RSET_ID
%token _PACKAGE RULESET END RULE
%token CLASS FLOAT_DEF STRING_DEF OBJECT_DEF CHAR_DEF BOOLEAN_DEF INTEGER_DEF
%token FUNCTION PROCEDURE
%token PATT_VAR IDENT INTEGER FLOAT CHAR STRING
%token COUNT_SET SUM_SET PROD_SET MIN_SET MAX_SET CONCAT_SET TIME_FUN
%token IS_A ABSTRACT RESTRICTS TEMPORAL TRIGGER PERMANENT TIMED UNTIMED
%token WINDOW
%token CAT_HIGH CAT_NORMAL CAT_LOW
%token IMPL
%token CREATE MODIFY CHANGE DELETE CALL
%token TRUE_VAL FALSE_VAL
%token NULL_STR
%token DOTDOTDOT
%token ON INSERT RETRACT
%right ','
%right '|'
%right '&'
%left '=' NEQ
%left '<' '>'
%right '!'
%left '+' '-'
%left '*' '/'
%right UMINUS


%start init

%%

init            : PKG_ID rulepkg
                | RSET_ID rule_set_dcls
                ;

rulepkg         : _PACKAGE IDENT         { def_package($2.ident); }
                     declarations
                  END 
                ;

rule_set_dcls   : rule_set_dcl
                | rule_set_dcls rule_set_dcl
                ;

declarations    :
                | declarations declaration
                ;

declaration     : obj_dcl
                | time_dcl
                | func_dcl
                | rule_set_dcl
                ;


obj_dcl         : abstract temp_beh CLASS IDENT         { def_class($4.ident, (int)$1.num, (int)$2.num); }
                  obj_dcl_body
                ;

obj_dcl_body    : normal_obj_body
                | restr_obj_body
                ;

normal_obj_body : superclass_def
                  '{'
                       normal_attr_set
                  '}'                           { end_def_class(); }
                ;
 

restr_obj_body : RESTRICTS IDENT                { restrict_class($2.ident); }
                  '{' 
                       restr_attr_set 
                  '}'                           { end_def_class(); }
                ;

abstract        :                       { $$.num=0; }
                | ABSTRACT              { $$.num=1; }
                ;

temp_beh        :                       { $$.num= IS_NORMAL; }
                | temp_beh TRIGGER      { $$.num= $1.num | IS_TRIGGER; }
                | temp_beh TEMPORAL     { $$.num= $1.num | IS_TEMPORAL; }
                | temp_beh PERMANENT    { $$.num= $1.num | IS_PERMANENT; }
                | temp_beh UNTIMED      { $$.num= $1.num | IS_UNTIMED; }
                | temp_beh TIMED        { $$.num= $1.num | IS_TIMED; }
                ;

superclass_def  :                       { is_a_base_class();}
                | IS_A IDENT            { join_class($2.ident); }
                ;

normal_attr_set :
                | normal_attr_set IDENT ':' type { def_attr($2.ident, (int)$4.num); }
                ;


restr_attr_set  :
                | restr_attr_set restr_attr_def
                ;
                
restr_attr_def  : r_attr_type_def  val_exp_opc
                ;

r_attr_type_def : IDENT ':' type        { def_attr($1.ident, (int)$3.num); }
                | IDENT                 { def_attr($1.ident, ANY_TYPE); }
                ;

type            : INTEGER_DEF   { $$.num = TYPE_NUM; }
                | FLOAT_DEF     { $$.num = TYPE_FLO; }
                | CHAR_DEF      { $$.num = TYPE_CHAR;}
                | STRING_DEF    { $$.num = TYPE_STR; }
                | BOOLEAN_DEF   { $$.num = TYPE_BOOL;}
                | OBJECT_DEF   { $$.num = TYPE_PATTERN;}
                ;

val_exp_opc     :
                | '=' value qualif
                ;


time_dcl        : WINDOW '=' INTEGER    { default_time_window((int)$3.num); }
                ;

func_dcl        : FUNCTION IDENT        { def_func($2.ident); }
                  '(' 
                      arg_set 
                  ')' '=' type          { def_func_type((int)$8.num); }
                | PROCEDURE IDENT       { def_func($2.ident); }
                  '(' 
                      arg_set 
                  ')'                   { def_func_type(TYPE_VOID); }
                ;

arg_set         :
                | DOTDOTDOT                { variable_number_of_args(); }
                | arg_set_not_void
                | arg_set_not_void ',' DOTDOTDOT { variable_number_of_args(); }
                ;

arg_set_not_void: type                  { def_arg((int)$1.num); }
                | arg_set_not_void ',' type { def_arg((int)$3.num); }
                ;

rule_set_dcl    : RULESET IDENT         { def_ruleset($2.ident); }
                    rule_set 
                  END                   { end_ruleset(); }
                ;

rule_set        :
                | rule_set rule
                ;

rule            : RULE IDENT rule_prio timed_rule     { def_rule($2.ident, 
                                                                 (int)$3.num,
                                                                 (int)$4.num);}
                  '{' 
                      pattern_list 
                   IMPL                 { def_production(); }
                      actions
                  '}'                   { end_rule(); }
                ;

rule_prio       : CAT_HIGH              { $$.num = 0; }
                | CAT_NORMAL            { $$.num = 1; }
                | CAT_LOW               { $$.num = 2; }
                ;

timed_rule      :                       { $$.num = NO_TIMED; }
                | TIMED win_time        { $$.num = $2.num;   }
                ;

win_time        :                       { $$.num = DEF_TIME; }
                | INTEGER               { $$.num = $1.num;   }

pattern_list    : patt_decl
                | pattern_list patt_decl
                ;

patt_decl       : temp_beh  { Pattern::initFlags($1.num, TRUE); }
                  patt_var pattern      { if ($3.ident != NULL) def_patt_var($3.ident); end_of_pattern(); }
                  qualif
                ;

pattern         : negated_pattern
                | affirmed_pattern
                ;

negated_pattern : '!' { set_status(ST_NEGATED);} simple_patt
                ;

affirmed_pattern: { set_status(ST_NORMAL); }
                  aff_patt_decl
                | { set_status(ST_OPTIONAL); }
                    '['
                        patt_var aff_patt_decl { if ($3.ident != NULL) def_patt_var($3.ident);} qualif
                    ']'
                ;

aff_patt_decl   : simple_patt
                | '{'                           { begin_set(); }
                      patt_var simple_patt  { if ($3.ident != NULL) def_patt_var($3.ident);}  qualif
                  '}'                           { end_set();   }
                ;

simple_patt     : IDENT                   { match_class($1.ident); } 
                  '(' patt_attr_dcls0 ')'
                ;

patt_var        :                       { $$.ident = NULL;    }
                | PATT_VAR              { $$.ident = $1.ident;}
                ;

patt_attr_dcls0 :
                | patt_attr_decls
                ;

patt_attr_decls : patt_attr_decl
                | patt_attr_decls ',' patt_attr_decls
                ;

patt_attr_decl  : IDENT                { match_attr($1.ident); } 
                  value qualif
                | IDENT                {  match_attr($1.ident); }
                  eq_var_or_ref qualif { equality_ref(); match_type_ref(NO_CAST_TYPE, $3.num); }
                ;

value           : STRING                { match_val(TYPE_STR, $1.str); }
                | FLOAT                 { match_val(TYPE_FLO, $1.flo); }
                | '-' FLOAT %prec UMINUS {match_val(TYPE_FLO, (int)-$2.flo); }
                | NULL_STR              { match_val(TYPE_STR, NULL_STR_VALUE); }
                | other_value
                ; 

cast            : INTEGER_DEF   { $$.num=TYPE_NUM; }
                | OBJECT_DEF    { $$.num=TYPE_PATTERN; }
                | BOOLEAN_DEF   { $$.num=TYPE_BOOL; }
                | CHAR_DEF      { $$.num=TYPE_CHAR; }
                ;


other_value     : INTEGER                 { match_val(TYPE_NUM, (int)$1.num); }
                | '-' INTEGER %prec UMINUS {match_val(TYPE_NUM, (int)-$2.num); }
                | CHAR                    { match_val(TYPE_CHAR, (int)$1.num); }
                | TRUE_VAL                { match_val(TYPE_BOOL, TRUE_VALUE); }
                | FALSE_VAL               { match_val(TYPE_BOOL, FALSE_VALUE); }
                | var_or_ref              { match_type_ref(NO_CAST_TYPE, (int)$1.num); }
                | cast '(' INTEGER ')'    { match_val((int)$1.num, (int)$3.num); }
                | cast '(' CHAR ')'       { match_val((int)$1.num, (int)$3.num); }
                | cast '(' TRUE_VAL ')'   { match_val((int)$1.num, TRUE_VALUE); }
                | cast '(' FALSE_VAL ')'  { match_val((int)$1.num, FALSE_VALUE); }
                | cast '(' var_or_ref ')' { match_type_ref((int)$1.num, (int)$3.num); }
                ;

eq_var_or_ref   : '=' IDENT             { $$.num = (long)ref_of_var($2.ident,TRUE, TRUE); }
                | '=' IDENT '.' IDENT   { $$.num = (long)ref_of_pvar($2.ident, $4.ident, TRUE);   }
                ;



var_or_ref      : IDENT             { $$.num = (long)ref_of_var($1.ident,TRUE);}
                | IDENT '.' IDENT   { $$.num = (long)ref_of_pvar($1.ident, $3.ident);   }
                ;

qualif          :
                | '/' expression        { match_exp($2.exp); }
                ;



expression      : simple_value
                | '-' expression         %prec UMINUS    
                                        { $$.exp = create_rel(MINUS, 
                                                           NULL, $2.exp); }
                        
                | expression '+' expression 
                                        { $$.exp = create_rel(ADD, 
                                                           $1.exp, $3.exp); }
                | expression '-' expression 
                                        { $$.exp = create_rel(SUB,
                                                           $1.exp, $3.exp); }
                | expression '*' expression 
                                        { $$.exp = create_rel(MUL,
                                                           $1.exp, $3.exp); }
                | expression '/' expression 
                                        { $$.exp = create_rel(DIV,
                                                           $1.exp, $3.exp); }
                | expression '<' expression
                                        { $$.exp = create_rel(TLT,
                                                           $1.exp, $3.exp); }
                | expression '>' expression
                                        { $$.exp = create_rel(TGT,
                                                           $1.exp, $3.exp); }
                | expression '<''=' expression
                                        { $$.exp = create_rel(TLE,
                                                           $1.exp, $4.exp); }
                | expression '>''=' expression
                                        { $$.exp = create_rel(TGE,
                                                           $1.exp, $4.exp); }
                | expression '=' expression
                                        { $$.exp = create_rel(TEQ,
                                                           $1.exp, $3.exp); }
                | expression NEQ expression
                                        { $$.exp = create_rel(TNE,
                                                           $1.exp, $3.exp); }
                | expression '&' expression 
                                        { $$.exp = create_rel(TAND, 
                                                           $1.exp, $3.exp); }
                | expression '|' expression
                                        { $$.exp = create_rel(TOR, 
                                                           $1.exp, $3.exp); }
                | '!' expression        {$$.exp = create_rel(TNOT, 
                                                           NULL, $2.exp); }
                      
                | '(' expression ')'      
                                        { $$ = $2; }
                ;

simple_value    : STRING                { $$.exp=create_val(TYPE_STR, $1); }
                | NULL_STR              { $$.exp=create_val(TYPE_STR, $1); }
                | INTEGER               { $$.exp=create_val(TYPE_NUM, $1); }
                | FLOAT                 { $$.exp=create_val(TYPE_FLO, $1); }
                | CHAR                  { $$.exp=create_val(TYPE_CHAR,$1); }
                | TRUE_VAL              { $$.exp=create_val(TYPE_BOOL,$1); }
                | FALSE_VAL             { $$.exp=create_val(TYPE_BOOL,$1); }
                | var_or_ref_expr       { $$.exp=create_val(TYPE_REF, $1); }
                | INTEGER_DEF '(' expression ')'   {$$.exp=create_primitive(CAST_INT, 1, $3.exp);}
                | OBJECT_DEF '(' expression ')'    {$$.exp=create_primitive(CAST_OBJ, 1, $3.exp);}
                | BOOLEAN_DEF '(' expression ')'   {$$.exp=create_primitive(CAST_BOO, 1, $3.exp);}
                | CHAR_DEF '(' expression ')'      {$$.exp=create_primitive(CAST_CHA, 1, $3.exp);}
                | COUNT_SET '(' patt_num ')'       {$$.exp=create_primitive(COUNT,1, create_val(TYPE_NUM, $3));}
                | SUM_SET '(' var_or_ref_expr ')'  {$$.exp=create_primitive(SUMS, 1, create_val(TYPE_NUM, $3));}
                | PROD_SET '(' var_or_ref_expr ')' {$$.exp=create_primitive(PRDS, 1, create_val(TYPE_NUM, $3));}
                | MIN_SET '(' var_or_ref_expr ')'  {$$.exp=create_primitive(MINS, 1, create_val(TYPE_NUM, $3));}
                | MAX_SET '(' var_or_ref_expr ')'  {$$.exp=create_primitive(MAXS, 1, create_val(TYPE_NUM, $3));}
                | CONCAT_SET '(' var_or_ref_expr ',' expression ')'
                                                   {$$.exp=create_primitive(CONCS, 2, create_val(TYPE_NUM, $3), $5.exp);}
                | TIME_FUN '(' patt_num ')'        {$$.exp=create_primitive(PUSHT, 1, create_val(TYPE_NUM, $3));}
                | IDENT '(' exp_arg_decls0 ')'     { $$.exp=create_fun($1.ident, $3.arg); }
                ;


var_or_ref_expr : IDENT             { $$.num = (long)ref_of_var($1.ident,FALSE);}
                | IDENT '.' IDENT   { $$.num = (long)ref_of_pvar($1.ident, $3.ident);   }
                ;

exp_arg_decls0  : { $$.arg = (Argument*)NULL; } 
                | exp_arg_decls
                ;

exp_arg_decls   : expression            { $$.arg = create_arg($1.exp); }
                | expression ',' exp_arg_decls  
                                        { $$.arg = concat_arg(create_arg($1.exp), $3.arg); }
                ;

actions         : action
                | actions action 
                ;
                

modify          : MODIFY         {$$.num=0;}
                | CHANGE         {$$.num=1;}
                ;

action          : CREATE on_tags patt_var IDENT  { notify_for_create($4.ident, $2.num); 
                                                   if ($3.ident != NULL) def_patt_var($3.ident);} 
                  '(' attr_val_list0 ')'         { end_of_RH_order(); }

                | modify on_tags patt_num        { notify_for_modify($1.num, (int)$3.num, $2.num);}
                  '(' attr_val_list ')'          { end_of_RH_order(); }

                | DELETE on_tags patt_num        { notify_for_delete((int)$3.num, $2.num); }

                | patt_var IDENT                 { notify_for_obj_imply($2.ident); 
                                                   if ($1.ident != NULL) def_patt_var($1.ident);} 
                  '(' attr_val_list0 ')'         { end_of_RH_order(); }

                | CALL on_tags IDENT             { init_proc($3.ident, $2.num); }
                  '(' exp_arg_decls0 ')'         { store_proc($3.ident, $6.arg); }
                ;

on_tags         :                       { $$.num = EXEC_INSERT; }
                | ON tags               { $$ = $2; }
                ;

tags            : tag                   { $$ = $1; }
                | tags ',' tag          { $$.num = ($1.num | $3.num); }
                ;

tag             : INSERT                { $$.num = EXEC_INSERT; }
                | MODIFY                { $$.num = EXEC_MODIFY; }
                | RETRACT               { $$.num = EXEC_RETRACT; }
                ;



patt_num        : INTEGER               { $$.num = $1.num - 1; }
                | IDENT                 { $$.num = (long)patt_num_of_var($1.ident);}
                ;

attr_val_list0  :
                | attr_val_list
                ;

attr_val_list   : IDENT                 { match_attr($1.ident);} 
                     expression         { store_exp($3.exp); }
                | attr_val_list ',' attr_val_list
                ;


