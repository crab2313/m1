#ifndef __M1_AST_H__
#define __M1_AST_H__

#include "symtab.h"
#include "decl.h"
#include "instr.h"
#include "compiler.h"

#include "ann.h"


typedef struct m1_block {
    struct m1_expression *stats;
    struct m1_symboltable locals;
    
} m1_block;

/* structure to represent a function. */
typedef struct m1_chunk {
    char                 *rettype;      /* return type of chunk */
    char                 *name;         /* name of this chunk */    
    struct m1_chunk      *next;         /* chunks are stored in a list. */
    struct m1_block      *block;        /* list of statements. */

    struct m1_var        *parameters;   /* list of parameters */
    unsigned              num_params;   /* parameter count. */
    
    unsigned              line;         /* line of function declaration. */    
    struct m1_symboltable constants;    /* constants used in this chunk */
        
} m1_chunk;

/* Struct to represent each struct field. */
typedef struct m1_structfield {
    char        *name;              /* name of struct member. */
    char        *type;              /* type of struct member. */
    unsigned     offset;            /* memory offset of this member in the struct */
    
    struct m1_structfield *next;    /* fields are stored as a list. */
    
} m1_structfield;

/* structure that holds an M1 struct definition */
typedef struct m1_struct {
    char    *name;              /* name of this struct. */
    unsigned size;              /* total size of this struct; can calculate from fields but 
                                   better keep a "cached" value */
    
    struct m1_structfield *fields; /* list of fields in this struct. */ 
      
} m1_struct;

typedef struct m1_pmc {
    char    *name;
    unsigned size;

    struct m1_chunk       *methods;    
    struct m1_structfield *fields;
    
} m1_pmc;

/* To represent "lhs = rhs" statements. */
typedef struct m1_assignment {
    struct m1_object     *lhs;
    struct m1_expression *rhs;
    
} m1_assignment;

/* To represent function call statements. */
typedef struct m1_funcall {
    char                 *name;
    struct m1_expression *arguments;
    struct m1_decl       *typedecl;
    
} m1_funcall;

/* To represent new expressions (new Object(x, y, z) ). */
typedef struct m1_newexpr {
	char                 *type;            /* name of new type to instantiate. */
	struct m1_decl       *typedecl;        /* pointer to declaration of type. */
	struct m1_expression *args;            /* arguments passed on to type's constructor. */    
	
} m1_newexpr;


typedef enum m1_expr_type {
    EXPR_ADDRESS,   /* &x */
    EXPR_ASSIGN,
    EXPR_BINARY,
    EXPR_BLOCK,
    EXPR_BREAK,
    EXPR_CONTINUE,
    EXPR_CAST,
    EXPR_CHAR,    
    EXPR_CONSTDECL,
    EXPR_DEREF,     /* *x */
    EXPR_DOWHILE,
    EXPR_FALSE,
    EXPR_FOR,
    EXPR_FUNCALL,
    EXPR_IF,
    EXPR_INT,
    EXPR_M0BLOCK,
    EXPR_NEW,
    EXPR_NULL,
    EXPR_NUMBER,
    EXPR_OBJECT,
    EXPR_PRINT,   /* temporary? */    
    EXPR_RETURN,
    EXPR_STRING,
    EXPR_SWITCH,
    EXPR_TRUE,
    EXPR_UNARY,
    EXPR_VARDECL,
    EXPR_WHILE
    

} m1_expr_type;


typedef enum m1_binop {
	OP_ASSIGN,
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_XOR,
    OP_GT,
    OP_GE,
    OP_LT,
    OP_LE,
    OP_EQ,
    OP_NE,
    OP_AND,
    OP_OR,
    OP_BAND,
    OP_BOR,
    OP_RSH,
    OP_LSH
} m1_binop;

/* To represent binary expressions, like a + b. */
typedef struct m1_binexpr {
    struct m1_expression *left;
    struct m1_expression *right;
    m1_binop op;
        
} m1_binexpr;


typedef enum m1_unop {
    UNOP_POSTINC,  /* a++ */
    UNOP_POSTDEC,  /* a-- */
    UNOP_PREINC,   /* ++a */
    UNOP_PREDEC,   /* --a */
    UNOP_NOT       /* !a  */
    /* There is no UNOP_NEG: unary minus is handled by multiplying by -1 */
} m1_unop;

/* for unary expressions, like -x, and !y. */
typedef struct m1_unexpr {
    struct m1_expression *expr;     
    m1_unop op;                     
    
} m1_unexpr;

typedef struct m1_castexpr {
    struct m1_expression *expr;
    char                 *type;    
    m1_valuetype          targettype;
} m1_castexpr;

/* object types. */
typedef enum m1_object_type {
    OBJECT_LINK,  /* node linking a and b in a.b */
    OBJECT_MAIN,  /* a in a.b  */
    OBJECT_FIELD, /* b in a.b  */
    OBJECT_INDEX, /* b in a[b] */
    OBJECT_DEREF, /* b in a->b */
    OBJECT_SCOPE, /* b in a::b */
    OBJECT_SELF,  /* "self"    */
    OBJECT_SUPER  /* "super"   */
    
} m1_object_type;

/* struct to represent an element or link between two elements
   in aggregates. In a.b.c, each element (a, b, c) is represented
   by one m1_object node. Links, between a and b, and b and c are ALSO
   represented by a m1_object node. Yes, that's a lot of nodes for 
   an expression like "a.b.c" (5 in total).
   
 */
typedef struct m1_object {
    
    union {
        char                 *name;  /* for name, field or deref access, in a.b.c for instance. */
        struct m1_expression *index; /* for array index (a[42]) */        
        struct m1_object     *field; /* if this is a linking node (OBJECT_LINK) representing "a.b" as a whole. */
    } obj;
    
    enum m1_object_type type;       /* selector for union */
    struct m1_symbol   *sym;        /* pointer to this object's declaration. */ 
    
    struct m1_object   *parent;     /* pointer to its parent (in a.b.c, a is b's parent) */
      
} m1_object;



/* for while and do-while statements */
typedef struct m1_whileexpr {
    struct m1_expression *cond;    
    struct m1_expression *block;
} m1_whileexpr;

/* for if statements */
typedef struct m1_ifexpr {
    struct m1_expression *cond;
    struct m1_expression *ifblock;
    struct m1_expression *elseblock;
} m1_ifexpr;

/* for for-statements */
typedef struct m1_forexpr {
    struct m1_expression *init;
    struct m1_expression *cond;
    struct m1_expression *step;
    struct m1_expression *block;    
} m1_forexpr;

/* const declarations */
typedef struct m1_const {
    char                 *type;
    char                 *name;
    struct m1_expression *value;
} m1_const;


/* variable declarations */
typedef struct m1_var {
    char                 *name;
    char                 *type;      /* store type name; type may not have been parsed yet; check in type checker. */
    struct m1_expression *init;      /* to handle: int x = 42; */
    unsigned              num_elems; /* 1 for non-arrays, larger for arrays */
    struct m1_symbol     *sym;       /* pointer to symbol in symboltable */
    
    struct m1_var        *next;      /* var nodes are stored as a list. */
} m1_var;


/* structure to represent a single case of a switch statement. */
typedef struct m1_case {
	int                   selector;
	struct m1_expression *block;	
	struct m1_case       *next;
	
} m1_case;


/* structure to represent a switch statement. */
typedef struct m1_switch {
	struct m1_expression *selector;
	struct m1_case       *cases;
	struct m1_expression *defaultstat;
	
} m1_switch;

/* for representing literal constants, i.e., int, float and strings */
typedef struct m1_literal {
    union m1_value     value; /* the value */
    enum m1_valuetype  type; /* selector for the union value */
    struct m1_symbol  *sym; /* pointer to a symboltable entry. */
    
} m1_literal;

typedef struct m0_block {
    struct m0_instr       *instr;
       
} m0_block;

typedef struct m1_enumconst {
    char  *name;                /* name of this constant */
    int    value;               /* value of this constant */
    struct m1_enumconst *next;
} m1_enumconst;

typedef struct m1_enum {
    char         *enumname;
    m1_enumconst *enums;
    
} m1_enum;

/* to represent statements */
typedef struct m1_expression {
    union {
        struct m1_unexpr     *u;
        struct m1_binexpr    *b;
        struct m1_funcall    *f;  
        struct m1_assignment *a; 
        struct m1_whileexpr  *w;  
        struct m1_forexpr    *o;
        struct m1_ifexpr     *i;
        struct m1_expression *e; 
        struct m1_object     *t;
        struct m1_const      *c;
        struct m1_var        *v;
        struct m0_block      *m0;
        struct m1_switch     *s;
        struct m1_newexpr    *n;
        struct m1_literal    *l;
        struct m1_castexpr   *cast;
        struct m1_block      *blck;
    } expr;
    
    m1_expr_type  type; /* selector for union */
    unsigned      line; /* line number */
    
    struct m1_expression *next;
    
} m1_expression;


extern int yyget_lineno(yyscan_t yyscanner);

//extern m1_chunk *chunk(M1_compiler *comp, char *rettype, char *name);
extern m1_chunk *chunk(ARGIN_NOTNULL(M1_compiler * const comp), ARGIN(char *rettype), ARGIN_NOTNULL(char *name));

//extern m1_expression *block(M1_compiler *comp);
extern m1_block *block(ARGIN_NOTNULL(M1_compiler *comp));

extern m1_expression *expression(M1_compiler *comp, m1_expr_type type);       
extern m1_expression *funcall(M1_compiler *comp, m1_object *fun, m1_expression *args);
            
extern m1_object *object(M1_compiler *comp, m1_object_type type);            
extern void obj_set_ident(m1_object *node, char *ident);

extern m1_structfield *structfield(M1_compiler *comp, char *name, char *type);

extern m1_struct *newstruct(M1_compiler *comp, char *name, m1_structfield *fields);

extern m1_pmc *newpmc(M1_compiler *comp, char *name, m1_structfield *fields, m1_chunk *methods);

extern m1_expression *ifexpr(M1_compiler *comp, m1_expression *cond, m1_expression *ifblock, m1_expression *elseblock);
extern m1_expression *whileexpr(M1_compiler *comp, m1_expression *cond, m1_expression *block);
extern m1_expression *dowhileexpr(M1_compiler *comp, m1_expression *cond, m1_expression *block);
extern m1_expression *forexpr(M1_compiler *comp, m1_expression *init, m1_expression *cond, m1_expression *step, m1_expression *stat);

extern m1_expression *inc_or_dec(M1_compiler *comp, m1_expression *obj, m1_unop optype);
extern m1_expression *returnexpr(M1_compiler *comp, m1_expression *retexp);
extern m1_expression *assignexpr(M1_compiler *comp, m1_expression *lhs, int assignop, m1_expression *rhs);

extern m1_expression *objectexpr(M1_compiler *comp, m1_object *obj, m1_expr_type type);

extern m1_expression *binexpr(M1_compiler *comp, m1_expression *e1, int op, m1_expression *e2);
extern m1_expression *number(M1_compiler *comp, double value);
extern m1_expression *integer(M1_compiler *comp, int value);
extern m1_expression *character(M1_compiler *comp, char ch);

extern m1_expression *string(M1_compiler *comp, char *str);
extern m1_expression *unaryexpr(M1_compiler *comp, m1_unop op, m1_expression *e);
extern m1_object *arrayindex(M1_compiler *comp, m1_expression *index);
extern m1_object *objectfield(M1_compiler *comp, char *field);
extern m1_object *objectderef(M1_compiler *comp, char *field);
extern m1_expression *printexpr(M1_compiler *comp, m1_expression *e);
extern m1_expression *constdecl(M1_compiler *comp, char *type, char *name, m1_expression *expr);
extern m1_expression *vardecl(M1_compiler *comp, char *type, m1_var *v);

extern m1_var *var(M1_compiler *comp, char *name, m1_expression *init);
extern m1_var *array(M1_compiler *comp, char *name, unsigned size, m1_expression *init);

extern unsigned field_size(struct m1_structfield *field);

extern m1_expression *switchexpr(M1_compiler *comp, m1_expression *expr, m1_case *cases, m1_expression *defaultstat);
extern m1_case *switchcase(M1_compiler *comp, int selector, m1_expression *block);

extern m1_expression *newexpr(M1_compiler *copm, char *type, m1_expression *args);

extern m1_object *lhsobj(M1_compiler *comp, m1_object *parent, m1_object *field);
extern m1_expression *castexpr(M1_compiler *comp, char *type, m1_expression *castedexpr);

extern m1_structfield *struct_find_field(M1_compiler *comp, m1_struct *structdef, char *fieldname);

extern m1_enumconst *enumconst(M1_compiler *comp, char *enumitem, int enumvalue);
extern m1_enum *newenum(M1_compiler *comp, char *name, m1_enumconst *enumconstants);

extern m1_var *parameter(M1_compiler *comp, char *type, char *name);


extern void block_set_stat(m1_block *block, m1_expression *stat);

extern struct m1_block *open_scope(M1_compiler *comp);
extern void close_scope(M1_compiler *comp);

extern void enter_param(M1_compiler *comp, m1_var *parameter);

#endif

