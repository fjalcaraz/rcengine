# RCENGINE a Fast Rule Compiler Engine

RCEngine is a personal development, mainly done between 1995 and 1998, that was an important piece in some advanced projects in the field of AI at that time. Now it has been modernized and taken to the present because I believe it is still something important to be known and I hope you will find it useful and finally decide to use it.

RCEngine is an implementation of the RETE algorithm form Charles Forgy, defined in his famous article *"A Fast Algorithm for the Many Pattern/Many Object Pattern Match Problem"* (In: Artificial Intelligence, vol. 19, pp. 17–37, 1982.), that transforms the rules conditions in a network of nodes that act as filters for the objects. This initial algorithm has being widely extended in RCEngine to cover many other usual needs, as the management of sets, object implications (abstraction), negative and optional patterns, and the use of the time as a quite important variable in the object matching (timed rules).

## What is RCEngine
RCEngine is a production rules compiler (forward chaining rules engine). It is a library able to load a Package of rules, compile them in a suitable way, and accept from that moment a flow of external events (objects created, modified or deleted) that will be correlated according to the rules. As a result of the propagation of each object, some rules may be ready to be triggered. This is what is called the Conflict Set. One rule is then selected and executed what occasionally will generate additional events that will be propagated in the net (chained).

## What are the main advantages of the use of RCEngine
The most important feature is its incredible performance. 

The bottleneck of RETE algorithm is the management of the memories in the "inter" nodes. In these nodes the objects arrive to them by the left or right sides. When a new object arrives, the node conditions are tested between it and each of the objects that arrived in the past to the other side. To this possible the objects are stored in "memories", one for each side, to wait there for the objects to come by the other side. This way of working, as defined by C. Forgy, is affected by an important reduction of performance as the number of objects increase.

To reduce this effect, the memories in RCEngine has been implemented with B-Trees, technology used in databases that minimize the lost of performance. The B-Trees maintain the objects sorted by the attribute values that are used in the node conditions, and they can be multilevel when the conditions are expressed as an AND logical expression (concatenation of conditions).

Also the time and how it is used in the rules and classes is an odd feature that simplify the time based restrictions. Very often the happening of events inside some time-window is an additional condition for the correlation.

## Language Definition
RCEngine expects to receive a **Package** following the next structure:

    PACKAGE --name-of-package--
    --declarations--
    END

There are four kinds of *declarations*
- Classes declarations
- External Functions and Procedures declarations
- Default Time Window declaration
- Rulesets declarations

We will describe the syntax of all these declarations but let us begin by Rulesets. A **Ruleset** is simply a set of related rules. Its structure is as follows:

    RULESET --name-of-ruleset--
        --rule--
        --rule--
        ....
    END

RCEngine is able to load only a Package, and all its RuleSets may be declared within the Package file or they can be in separated files that will have to be loaded individually.

> Every Ruleset and the Package may be also unloaded what will free all the related allocated memory.

> Although in the examples the keywords are written in capitals, the language keywords are case insensitive.

### Classes Declarations
A normal class of objects is declared simply with the keyword **CLASS**, followed by its name and by a set of attributes (its name followed by the type after a colon symbol):

    CLASS --class-name-- {
        --attr-name-- : --type--
        --attr-name-- : --type--
    }

The class name and the attribute names are simple identifiers. The types of the attributes have to be one of:
- INTEGER
- FLOAT
- CHAR
- STRING
- BOOLEAN
- OBJECT

### Class Types
RCEngine supports **inheritance** among classes so:
- A class may be declared as a *Subclass* of another using the keyword **IS_A** after its class name and followed by the superclass name. This means that the first inherits all the attributes of the last.

    ```
    CLASS --subclass-name-- IS_A --superclass-name-- { 
        --additional attributes-- 
    }
    ```

- A class may be declared as an *abstract superclass* preceding the keyword **ABSTRACT** before **CLASS**. This means that won't be any object instances of it. These will be of any of its subclasses defined below.

    ```
    ABSTRACT CLASS --class-name-- { 
        --attributes-- 
    }
    ```

- Finally a class can be declared as a *Restriction* of another. A Restriction does not change the class name in the instances but, they are restricted by the values of some attributes (e.g. class PhoneCall and LocalPCalls that restricts the first, due LocalPCalls are those PhoneCalls with no international prefix). A restricted class is defined using the keyword **RESTRICTS**. The value restrictions are expressed in a very similar way as in the rules: the symbol equal (=) followed by a simple value constant or a variable (simple identifiers). If a variable appears more than once it means equality among them. It is possible to set a variable and then after a slash ('/' that must be read as "such that") put an expression that the variable must satisfy (e.g. attr = a / a>0 & a<5). We'll see more deeply the expressions below.

    ```
    CLASS --subclass-name-- RESTRICTS --superclass-name-- { 
        --attr-name-- = --value-or-var-- [ / --expression-- ]
    }
    ```

Any class, independently of its type may be use in the rule patterns. Every abstract class must have at least a non-abstract subclass, to be used.

### Class Timing Behavior
It is possible to define a timing behavior related to two different aspects: the storage in the system and the correlation. 

Related to the storage, a class can be **TRIGGER**, **TEMPORAL** or **PERMANENT**.
- TRIGGER behavior means that the objects of this class won't be stored in the side memories of the inter nodes. They will try matches with the objects of the other side, but cause it won't be stored, it won't make additional matches with objects of the other side to come in the future.
- TEMPORAL means that he objects must be retracted, removed from the memories, when their timestamps are older than current time minus the rule time window. A Class is TEMPORAL when only the instances inside a time window frame are important.
- PERMANENT is the normal behavior, the objects are stored in the memories an they will be in the system until they are retracted, by a rule or externally.

Related to the correlation (matching), the class can be **TIMED** or **UNTIMED**
- TIMED is the usual behavior in timed rules, all the objects that verify the rule patterns at the left-hand-side (LHS) of the rule must be inside a certain time window frame defined in the rule. If the rule is not TIMED the matching does not consider the time, even in the class has been defined as TIMED.
- UNTIMED class are those whose instances are not going to be checked if they are inside or not the time window frame defined in the timed rules, and where the rest of the TIMED objects must be. UNTIMED objects represent timeless states.

PERMANENT and TIMED are usually not declared as they are the default behaviors.

The timing behaviors are put just before the keyword CLASS at the class declaration.

    ABSTRACT TEMPORAL TIMED CLASS Alarm {
        Category: integer
        Text: string
    }

### Default Time Window declaration
The default time window declaration is the default time window that every timed rule will have unless an specific time window will be declared for it. The declaration is quite easy using the keyword WINDOW

    WINDOW = 300

### External Functions and Procedures declarations
A function call always return a value and they can be used in expressions, either at the left or right hand-side of the rules. However, the procedures are external actions and they will be present only at the right hand-side.

RCEngine counts with several built-in functions and procedures and is also able to call to external functions and procedures.

Functions and Procedures are defined in a very similar way: they start with the keyword **FUNCTION** or **PROCEDURE**, followed by the types of the arguments enclosed in round brackets and separated by comma, and in the case of function, followed by a colon ':' and the type of the returned value. Remember the types can be:
- INTEGER
- FLOAT
- CHAR
- STRING
- BOOLEAN
- OBJECT

Variable arguments are also supported with the use of triple dot (...)

    PROCEDURE notify(INTEGER, INTEGER, INTEGER)
    FUNCTION isConnectedTo(INTEGER, INTEGER): BOOLEAN

Functions and Procedures must be already connected using the interface functions (see API) or these declarations will fail

### Rule Definitions
A rule is the declaration of what actions to do when a certain pattern of objects is achieved. Its structure is quite simple: a header, a left hand side (the patterns) and a right hand side (the actions).

In the rule header is possible to declare a rule name, its priority, and its time window if the rule is timed. Following are two rules, the first timed and the second not timed

    RULE rule_name HIGH TIMED 300 {
        --patterns--
        ->
        --actions--
    }

    RULE rule_name NORMAL {
        --patterns--
        ->
        --actions--
    }

The priority can be: HIGH, NORMAL or LOW. This priority helps to decide what rule execute when after the propagation of an event there are several rules ready to be triggered (what is called the conflict set). Based on its priority only a rule is chosen and executed, and the events produced due to that execution are propagated in order. This process is chained until no event propagations are pending and no rules are in the conflict set.

The window time can be not set, and is taken the default in that case.

### Patterns. The LHS of Rule.
The patterns specify the conditions that must verify the objects that match the rule. These conditions can be intra-object (or simply *intra*, those to be satisfied by the object by itself) or inter-objects (or simply *inter*, those to be satisfy by some of the objects among them).

The aspect of a pattern is quite similar to an object: it begins with a class name and between round brackets a sort of attributes and values separated by commas. 

    classA(attr1 3, attr2 4)

But now is expressing the conditions the objects must satisfy, so it is not needed to write all the object attributes but only those whose values must accomplish.

Also, instead a simple value, a variable name can be used. After the object and in a similar way to the restricted classes, using a slash in the middle (as saying "such that"), an expression can be written to express a more complicated condition

    classA(attr1 3, attr2 x / x > 2 & x < 6)

If the same variable appears more than once, this means equality among them

    classA(attr1 3, attr2 x)
    classB(other_attr x)

although it is better to express this equalities in a different way (the previous way generates a warning when compiling)

    classA(attr1 3, attr2 x)
    classB(other_attr = x)


A variable may also be associated to a pattern and may be used to refer the object that made matching, or just an attribute of that object 

    clA: classA(attr1 3)
         classB(other_attr = clA.attr2 )

The three last examples have the same meaning

#### Patterns types
Three types of patterns are supported by RCEngine:
- **Affirmative** patterns (as those we have just seen)
- **Negative** patterns: Whose meaning is that *there is no one object that verify the pattern*. This pattern must have the '!' sign at the beginning

    ```
    classA(attr1 3, attr2 x)
    !classB(other_attr = x)
    ```

- **Optional** patterns: that have the meaning of *may exist or not an object that verify the pattern but, if there is one, we want it*. This pattern is written between square brackets

    ```
    classA(attr1 3, attr2 x)
    [classB(other_attr = x)]
    ```

Also, the patterns may be **Simple** or **Set**. 

A Set pattern is able to join together all the objects that made matching. The Set patterns is written between curly brackets and will be verified with at least one object. If we want to verify the set pattern even with 0 objects in it we can combine Optional and Set characteristics.

    classA(attr1 3, attr2 x)
    [{classB(other_attr = x)}]

### Expressions
As we have already seen, the expressions allow us to manage complex matchings. In a expression we can use simple values, variables, operators and functions (built-in primitives or external)
The operators are:
- Numerical: +, -, *, /, and - (unary minus)
- Logical: |, &, or ! (unary not)
- Comparison: <, <=, >, >=, =, !=
- Parenthesis: (...) to alter the precedence of the operators

### Primitives
There are several primitive functions defined, that can be used in the expressions.
- string functions:
    * **append**(str1, str2): Append str2 to the end of str1.
    * **head**(str, n): Get the first n characters of str
    * **tail**(str, n): Get the last n characters of str
    * **except**(str, n): Get str except the last n characters
    * **substr**(str, from, to): Get the substring of str from the character "from" to "to". Indexes are 1 based, so substr("abcd", 3, 3) => "c"
    * **length**(str): Get the length od the string str
    * **strtonum**(str): Read a number in str
    * **strtofloat**(str): Read a float number in str
- numeric functions
    * **numtostr**(num): Transform a num in a string
    * **floattostr**(float): Transform a float in a string
    * **numtofloat**(num): Transform an integer number in a float
    * **foattonum**(float): Transform a float into an integer number
- miscelanea
    * **printf**(format, ...): Print in standard output the result of formatting string to the rest of parameters references in it. Format follows the common printf format available [here](https://en.wikipedia.org/wiki/Printf_format_string) 
    * **sprintf**(str, format, ...) Similar to printf but the result of the formatting is stored in str
    * **fprintf**(num, format, ...) Similar to printf but the result is appended to the file /tmp/eng_--pid--.--num--
- Set patterns related functions (the patterns are passed to the functions using pattern vars, or may be used their patterns number: 1 for th first, 2 for the second, etc).
    * **sum**(set pattern attribute) Sum that attribute values of all the elements in a set
    * **prod**(set pattern attribute) Multiply that attribute values of all the elements in a set
    * **min**(set pattern attribute) Find the lower attribute value of all the elements in a set
    * **max**(set pattern attribute) Find the higher attribute value of all the elements in a set
    * **count**(set pattern) Return the number of elements in a set.
    * **concat**(set pattern attribute, str_separator) Return the concatenation of that attribute values of all the elements in a set using a separator among them.
    * **time**(pattern) Return the timestamp of the object that verify that pattern.

### Expression casting
The expression casting are not functions, they are used to change the type of the result of a expression to another compatible type. This is possible due four types (integer, boolean, char and object) have the same internal representation: an integer number.

The casting is done putting the desired type into round brackets before the variable or value.

- (integer)
- (object)
- (boolean)
- (char)

by example:

    (char)65 is the character 'A'

### Actions, The RHS of the rule
RCEngines compiles all the pattern side (LHS) of the rules generating a nodes net where the events over objects are propagated and tested again the conditions of each node. Only if the object verify that conditions, it is allowed to pass through toward the children nodes. 

We cannot think these events are only insertion of new objects. The events can actually be also a modification or a deletion (retract). These events can be externally created, but it is in the RHS of the rules where they are internally created in what are known as **actions**.

#### CREATE action
This action create a new object. By default these actions are only executed when the rule is triggered in INSERT event tag, but it can be changed to be triggered on MODIFY or RETRACT (or even on some tags). The object created is propagated as a new INSERT event tag.

    RULE trigger_changes HIGH {
        object(type X)    
    ->
        CREATE ON INSERT, MODIFY changed(type X)
    }

This rule creates an object "change" every time an "object" is inserted or modified.

The attributes of the object created must be filled with values or expressions where the variables defined in the LHS patterns can be used.

#### MODIFY action
This action modify the object that made matching against one of the patterns. The patterns is identified by a pattern variable or by its pattern number: 1 for th first, 2 for the second, etc). 

As in the case on CREATE, by default MODIFY actions are only executed when the rule is triggered in INSERT event tag, but it can be changed to be triggered on MODIFY or RETRACT (or even on some of them). The object modified is propagated with the MODIFY event tag.

The order MODIFY and the pattern var or number are followed by an object where are referred only those modified attributes 

    RULE changePriority NORMAL TIMED 300 {
        fa: fireAlarm(building X, priority P / P>1 )
        cs: { calls(building X) } / count(cs) > 10
    ->
        MODIFY ON INSERT, MODIFY fa(priority 1)
    }

> beware that when a new object is added to an existing set the tag become MODIFY 

#### DELETE action
This action delete the object that made matching against one of the patterns. As in the two cases above, these actions are only executed when the rule is triggered in INSERT event tag, but it can be changed to be triggered on MODIFY or RETRACT (or even on some of them). The object deleted is propagated with the RETRACT event tag before be communicated externally and freed.

    RULE ceaseAlarm NORMAL TIMED 300 {
        fireAlarm(building X)
        fireResolved(building X)
    ->
        DELETE 1
    }

#### OBJECT IMPLY action
In RCEngine, beyond these "standard" actions, a new one is now introduced: the **object implication**. 

The object implication can be described as the ability to maintain an object alive (the implied object), meanwhile the objects that triggered the rule are all alive too. More precisely we can say that the rule will create, modify or delete these implied objects according to the event tag it is executed. 

Why are so important these implied objects? Because an implied object condenses a situation expressed in the patterns part of the rule; it is a common mind task: abstraction, an the abstractions make the analysis of the situation easier to be managed.

And object implication is just written in the rule simply as an object at the RHS of the rule. 

    RULE example NORMAL TIMED 300
    {
        conj:{ alarm(type X) } / count(conj) > 3
    ->
        burst(type X, numAlarms: count(conj))
    }

This example maintains the object burst meanwhile we are receiving more than 3 alarms in a window frame of 300 seconds.
Only when receiving a lower rate of alarms the object burst will be automatically deleted.

#### PROCEDURE CALLS
This action means the possibility to call a built-in or to an external procedure. Formally a procedure is quite similar to a function with the only difference that they do not return any value. 

A Procedure call is written with the keyword CALL followed by the name of the procedure and its arguments between round brackets and separated y commas. To fill its arguments simply constant values or variables may be used.  

In the same way it has been introduced with CREATE, MODIFY or DELETE the call will be done only on INSERT tag (When the patterns become achieved) but it can be changed to be called on MODIFY or RETRACT (or even on some of them)

    PROCEDURE notify(STRING, STRING)

    RULE notify NORMAL {
        Alarm(user u, text t)
    ->
        CALL ON INSERT notify(u, t);
    }

##### Built-in procedures
**empty_set**(set pattern attribute) generate a retraction for all the objects in the set

## API

Extern code must include *engine.h*

### Functions to manage Packages and Rulesets

#### *int load_pkg(char \*path)*
Load a package file given its filename and compile it. Return 1 if everything was right, and 0 otherwise.

#### *int load_pkg_str(char \*text)*
Load a package from a string and compile it. Return 1 if everything was right, and 0 otherwise.

#### *int load_rset(char \*path)*
Load a RuleSet file given its filename and compile it. Return 1 if everything was right, and 0 otherwise.

#### *int load_rset_str(char \*text)*
Load a RuleSet from a string and compile it. Return 1 if everything was right, and 0 otherwise.

#### *void reset_pkg()*
Reset all the node memories and set the package as if is was just loaded 

#### *void reset_rset(char \*name)*
Reset all 

#### *void free_pkg()*
Free a Package freeing all the allocated memory 

#### *void free_rset(char \*name)*
Free all the rules and memory allocated by a rule set 

### Management of objects (creation and propagation)
To manage objects RCEngine use of the following types:

    typedef struct obj_data
    {
        long time;
        void *user_data;
        Value attr[1];
    } ObjectType;

Define an object that will propagated in the nodes net. Only is allocated the first attribute that will be the class name.

Value is an union of the different base types: flo (float), num (integer), str(string)

    #define PROTECTED 2
    #define DYNAMIC 1
    ......
    
    typedef union
    {
        struct
        {
            char *str_p;
            int dynamic_flags;
        } str;
        long num;
        float flo;
    } Value;

The dynamic flags in the string allow to control if a string can be freed or not. It can be an OR of these flags:
- DYNAMIC means that a string has allocated in the heap by malloc().
- PROTECTED is used to protect while are in the stack the strings that are attributes of objects. If is needed to free strings in the stack must be verified that the dynamic_flags are equal (==) to DYNAMIC to exclude those with the PROTECTED flag. 

These are the related functions:

#### *ObjectType \*new_object(int n_attrs, long time)*
Create a object with n_attr attributes (more than the unique attribute defined in the struct). 

#### *void \*get_class(char \*name, int \*n_attr)*
Given a name, it returns the object class information in a hidden data and the number of attributes of that class (to be used in new_object)

#### *int attr_index(void \*objclass, char \*name)*
Given the object class and an attribute name it return the attribute index in ObjectType struct

#### *int attr_type(void \*objclass, int n_attr)*
Given the object class and the attribute number, it returns the attribute type. The different types are defined in *engine.h*:
- TYPE_STR
- TYPE_NUM
- TYPE_FLO
- TYPE_CHAR
- TYPE_BOOL
- TYPE_PATTERN (Object)

#### *char \*attr_name(void \*objclass, int n_attr)*
Given the object class and the attribute number, it returns the attribute name

#### *int class_is_subclass_of(char \*name1, char \*name2)*
Given two class names, returns if class name1 is a subclass of class name 2

To propagate objects. Must be used these functions:
#### *void engine_modify(ObjectType \*obj)*
In case of an external modification of an object, this function must be called before being modified

#### *void engine_loop(int tag, ObjectType \*obj)*
Propagate an Object with a certain tag. The tag must be one of *INSERT_TAG*, *MODIFY_TAG* or *DELETE_TAG*. These constants are defined in *engine.h*:

    #define INSERT_TAG 0x1
    #define RETRACT_TAG 0x2
    #define MODIFY_TAG 0x3

### Configuration
#### *void def_function(const char \*name, ExternFunction f)*
With this function extern functions and procedures are related with their implementations. This must be done before loading the Package where they are defined. An ExternFunction is defined as:

    typedef void (*ExternFunction)(Value *stack, int tag);

The first value is the base of the stack, so the different arguments will at the stack at index [0], [1], [2], ... The second argument is the propagation tag (as defined above), if it would be needed.

Function must free all the strings passed as parameters with dynamic_flags equal (==) to DYNAMIC and set the resulting value at stack[0] setting also, in case of strings, the dynamic_flags accordingly.  

#### *void add_callback_func(int when_flags, CallBackFunc f)*
#### *void del_callback_func(int when_flags, CallBackFunc f)*
With these functions it is possible to receive all the information about objects created, modified or deleted, due it will be very often that the external code will have to do something with them (free?, communicate/notify to other systems?).

When_flags is a logical OR of the bits of the following constants:

    #define WHEN_INSERTED 0x1
    #define WHEN_MODIFIED 0x2
    #define WHEN_RETRACTED 0x4
    #define WHEN_NOT_USED 0x8

    #define WHEN_EVENTS (WHEN_INSERTED | WHEN_MODIFIED | WHEN_RETRACTED)
    #define WHEN_ALL (WHEN_INSERTED | WHEN_MODIFIED | WHEN_RETRACTED | WHEN_NOT_USED)

Notice that there is also a notification when an object is no more used in the system, no references to it have been done internally.

A callback function will receive the when_flag, the object, the context and the number of objects in the context. 

    typedef void (*CallBackFunc)(int when_flag, ObjectType *obj, ObjectType **ctx, int n_objs);

The context is an array with all the objects that made matching in the left hand side of the rule when the event was generated. The context array (not the objects) must be freed. It will be null in case of WHEN_NOT_USED.

### Advanced Configuration

**MATCHING**

#### *void allow_same_obj()*
To allow the matching of the same object on different patterns of the same rule. By default it is forbidden

**ERRORS AND WARNINGS**

#### *void set_comp_warnings(int status)*
To show (true) or to hide(false) compiler warnings. By default *true*

#### *void set_errors_file(FILE \*file)*
To set a file where the errors are written to. By default *stderr*

**TRACE**

#### *void set_trace_level(int level)*
To set the trace level: 0: no traces, 1: basic information, 2: advanced. Advance information (level 2) is also set when the environment variable TRACE_ENG is set. By default is 0.

#### *void set_trace_file(FILE \*file)*
Where to write the traces. By default *stdout*

#### *void set_trace_file_name(const char \*name)*
File name where to write the traces. The extension is always set to ".dat"

#### *void set_trace_file_size(unsigned long size)*
Maximum trace file size. When reached the file ".dat" is renamed to ".old" and an empty ".dat" is started. By default 5MB

void set_lex_debug(int debug);
Special way to enable trace in the lexical analyzer. Allow to see the tokens read and it is only useful to resolve syntax problems.

**SETS**

The objects in a set are stored in B-Trees. To manage the sets in external functions (with pattern vars pointing to set patterns) is possible to include btree.hpp and manage the class BTree due the data stored in the stack is just a BTState that will ease to manage the tree. Anyway, the following functions are defined:

#### *ObjectType \*get_obj_of_set(void \*tree_state)*
This function will allow us to walk the set's tree. It will return the next object at each call.

#### *int n_obj_of_set(void \*tree_state)*
Return the number of objects in the set's tree

#### *void end_obj_of_set(void \*tree_state)*
This function ends the walk over the set's tree and free the BTState.

**TIME REFRESH**

#### *void engine_refresh(long real_time)*
Do the temporal refresh of the engine. It may produce the RETRACTION of objects that are older than the window from real time in temporal rules.

**UTILITIES**

#### *const char \*clave(const void \*vector)*
Debug utility to generate a small identifier, different for each pointer passed.

#### *void print_obj(FILE \*file, const ObjectType \*obj)*
Debug utility to print Objects to a FILE

#### *int get_inf_cnt()*
Allow to access to the counter of inferences done from the starting of the process

#### *void reset_inf_cnt()*
Reset the inference counter


        
