/*

AST node constructors

*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ast.h"
#include "symtab.h"
#include "compiler.h"

#include "ann.h"

extern int yyget_lineno(yyscan_t yyscanner);

static void expr_set_unexpr(M1_compiler *comp, m1_expression *node, m1_expression *exp, m1_unop op);             
static void expr_set_expr(m1_expression *node, m1_expression *expr);
static void expr_set_obj(m1_expression *node, m1_object *obj);
static void expr_set_assign(M1_compiler *comp, m1_expression *node, m1_expression *lhs, int assignop, m1_expression *rhs);

static void obj_set_index(m1_object *node, m1_expression *index);

static void *
m1_malloc(size_t size) {
    void *mem = calloc(1, size);
    if (mem == NULL) {
        fprintf(stderr, "Failed to allocate mem!\n");
        exit(EXIT_FAILURE);
    }
    return mem;   
}


/*@modiefies nothing @*/ 
m1_chunk *
chunk(M1_compiler *comp, int rettype, NOTNULL(char *name), m1_expression *block) {
    m1_chunk *c = (m1_chunk *)m1_malloc(sizeof(m1_chunk));
    c->rettype  = rettype;
    c->name     = name;
    c->block    = block;
    c->next     = NULL;
    
    return c;   
}

m1_expression *
expression(M1_compiler *comp, m1_expr_type type) {
    m1_expression *e;
    
    assert(comp != NULL);
    assert(comp->yyscanner != NULL);
    
    e        = (m1_expression *)m1_malloc(sizeof(m1_expression));
    e->type  = type;
    e->line  = yyget_lineno(comp->yyscanner);
    return e;   
}


static m1_literal *
new_literal(m1_valuetype type) {
    m1_literal *l = (m1_literal *)m1_malloc(sizeof(m1_literal));
    l->type = type;
    return l;    
}

static void 
expr_set_num(M1_compiler *comp, m1_expression *e, double v) {
    assert(e->type == EXPR_NUMBER); 

    e->expr.l = new_literal(VAL_FLOAT);
    e->expr.l->value.fval = v;
    e->expr.l->sym = sym_enter_num(&comp->currentchunk->constants, v);   
}

static void 
expr_set_int(M1_compiler *comp, m1_expression *e, int v) {
    assert(e->type == EXPR_INT);

    e->expr.l = new_literal(VAL_INT);
    e->expr.l->value.ival = v;
    e->expr.l->sym = sym_enter_int(&comp->currentchunk->constants, v);

}



m1_expression *
number(M1_compiler *comp, double value) {
	m1_expression *expr = expression(comp, EXPR_NUMBER);
	expr_set_num(comp, expr, value);
	return expr;	
}

m1_expression *
integer(M1_compiler *comp, int value) {
	m1_expression *expr = expression(comp, EXPR_INT);
	expr_set_int(comp, expr, value);
	return expr;	
}
m1_expression *
string(M1_compiler *comp, char *str) {
	m1_expression *expr = expression(comp, EXPR_STRING);
	assert(str != NULL);

    expr->expr.l = new_literal(VAL_STRING);
    expr->expr.l->value.sval = str;
    
    assert(comp != NULL);
    assert(comp->currentchunk != NULL);
    assert(&comp->currentchunk->constants != NULL);
    
    
    expr->expr.l->sym = sym_enter_str(&comp->currentchunk->constants, str, 0);

	return expr;	
}

m1_object *
arrayindex(M1_compiler *comp, m1_expression *index) {
	m1_object *obj = object(comp, OBJECT_INDEX);
	obj_set_index(obj, index);
	return obj;	
}

m1_object *
objectfield(M1_compiler *comp, char *field) {
	m1_object *obj = object(comp, OBJECT_FIELD);
	obj_set_ident(obj, field);
	return obj;	
}

m1_object *
objectderef(M1_compiler *comp, char *field) {
	m1_object *obj = object(comp, OBJECT_DEREF);
	obj_set_ident(obj, field);
	return obj;	
}
               
m1_expression *
unaryexpr(M1_compiler *comp, m1_unop op, m1_expression *e) {
	m1_expression *expr = expression(comp, EXPR_UNARY);
	expr_set_unexpr(comp, expr, e, op);
	return expr;
}	
                
m1_expression *
binexpr(M1_compiler *comp, m1_expression *e1, m1_binop op, m1_expression *e2) {
	m1_expression *expr = expression(comp, EXPR_BINARY);
	expr->expr.b = (m1_binexpr *)m1_malloc(sizeof(m1_binexpr));
    expr->expr.b->op    = op;
    expr->expr.b->left  = e1;
    expr->expr.b->right = e2;       
    return expr;    
}

m1_expression *
printexpr(M1_compiler *comp, m1_expression *e) {
	m1_expression *expr = expression(comp, EXPR_PRINT);
	expr_set_expr(expr, e);
	return expr;	
}

static m1_unexpr *
unexpr(M1_compiler *comp, m1_expression *node, m1_unop op) {
    m1_unexpr *e = (m1_unexpr *)m1_malloc(sizeof(m1_unexpr));
    e->expr      = node;
    e->op        = op;    
    return e;   
}

static void
expr_set_unexpr(M1_compiler *comp, m1_expression *node, m1_expression *exp, m1_unop op) {
    assert(node->type == EXPR_UNARY);   
    node->expr.u = unexpr(comp, exp, op);
}

static void 
expr_set_funcall(m1_expression *node, m1_funcall *f) {
    assert(node->type == EXPR_FUNCALL);   
    node->expr.f = f;
}

m1_expression *
funcall(M1_compiler *comp, char *name) {
	m1_expression *expr = expression(comp, EXPR_FUNCALL);
	expr->expr.f       = (m1_funcall *)m1_malloc(sizeof(m1_funcall));
	expr->expr.f->name = name;
	
	/* enter name of function to invoke into constant table. */
	sym_enter_chunk(&comp->currentchunk->constants, name);
    return expr;   
}


static m1_const *
const_decl(m1_valuetype type, char *name, m1_expression *expr) {
    m1_const *c = (m1_const *)m1_malloc(sizeof(m1_const));
    c->type     = type;
    c->name     = name;
    c->value    = expr;
    return c;    
}



m1_expression *
constdecl(M1_compiler *comp, m1_valuetype type, char *name, m1_expression *e) {
	m1_expression *expr = expression(comp, EXPR_CONSTDECL);
	expr->expr.c = const_decl(type, name, e);
	return expr;	
}

static void 
expr_set_for(m1_expression *node, m1_expression *init,
             m1_expression *cond, m1_expression *step,
             m1_expression *stat) 
{
    node->expr.o = (m1_forexpr *)m1_malloc(sizeof(m1_forexpr));
    
    node->expr.o->init  = init;
    node->expr.o->cond  = cond;
    node->expr.o->step  = step;
    node->expr.o->block = stat;
}   

m1_expression *
forexpr(M1_compiler *comp, m1_expression *init, m1_expression *cond, m1_expression *step, m1_expression *stat) {
	m1_expression *expr = expression(comp, EXPR_FOR);
	expr_set_for(expr, init, cond, step, stat);	
	return expr;
}



m1_expression *
inc_or_dec(M1_compiler *comp, m1_expression *obj, m1_unop optype) {	
	m1_expression *expr = expression(comp, EXPR_UNARY);
	expr_set_unexpr(comp, expr, obj, optype);
	return expr;
}

m1_expression *
assignexpr(M1_compiler *comp, m1_expression *lhs, int assignop, m1_expression *rhs) {
	m1_expression *expr = expression(comp, EXPR_ASSIGN); 
	expr_set_assign(comp, expr, lhs, assignop, rhs);
    return expr;
}

m1_expression *
objectexpr(M1_compiler *comp, m1_object *obj, m1_expr_type type) {
	m1_expression *expr = expression(comp, type);
	expr_set_obj(expr, obj); 
	return expr;
}

m1_expression *
returnexpr(M1_compiler *comp, m1_expression *retexp) {
	m1_expression *expr = expression(comp, EXPR_RETURN);
	expr_set_expr(expr, retexp);
	return expr;	
}

static void 
expr_set_while(M1_compiler *comp, m1_expression *node, m1_expression *cond, m1_expression *block) {
    node->expr.w        = (m1_whileexpr *)m1_malloc(sizeof(m1_whileexpr));    
    node->expr.w->cond  = cond;
    node->expr.w->block = block;                            
}   

static void
expr_set_if(M1_compiler *comp, m1_expression *node, m1_expression *cond, 
            m1_expression *ifblock, m1_expression *elseblock) 
{
    node->expr.i = (m1_ifexpr *)m1_malloc(sizeof(m1_ifexpr));              
    node->expr.i->cond      = cond;
    node->expr.i->ifblock   = ifblock;
    node->expr.i->elseblock = elseblock;
}

static void 
expr_set_expr(m1_expression *node, m1_expression *expr) {
    node->expr.e = expr;   
}

static void 
expr_set_obj(m1_expression *node, m1_object *obj) {
    node->expr.t = obj;    
}

static void 
expr_set_assign(M1_compiler *comp, m1_expression *node, m1_expression *lhs, int assignop, m1_expression *rhs) {
	/*
	a = b  => normal case
	a += b => a = a + b
	*/
    node->expr.a      = (m1_assignment *)m1_malloc(sizeof(m1_assignment));
    node->expr.a->lhs = lhs->expr.t; /* unwrap the m1_object representing lhs from its m1_expression wrapper. */
    
    switch (assignop) {
    	case OP_ASSIGN: /* normal case */
    		node->expr.a->rhs = rhs;        
    		break;
    	default: /* all other cases, such as: 
    	            a +=b => a = a + b; 
    	            make a new binary expression node for a + b 
    	          */
    		node->expr.a->rhs = binexpr(comp, lhs, assignop, rhs);
    		break;
    }

}


void 
obj_set_ident(m1_object *node, char *ident) {
    node->obj.name = ident;    
}

void 
obj_set_index(m1_object *node, m1_expression *index) {
    node->obj.index = index;
}

m1_object *
object(M1_compiler *comp, m1_object_type type) {
    m1_object *obj = (m1_object *)m1_malloc(sizeof(m1_object));
    obj->type      = type;
    return obj;    
}

m1_object *
lhsobj(M1_compiler *comp, m1_object *parent, m1_object *field) {
    m1_object *lhsobj = (m1_object *)m1_malloc(sizeof(m1_object));
    lhsobj->type      = OBJECT_LINK;
    
    lhsobj->obj.field = field;
    lhsobj->parent    = parent;
    return lhsobj;   
}

m1_structfield *
structfield(M1_compiler *comp, char *name, m1_valuetype type) {
    m1_structfield *fld = (m1_structfield *)m1_malloc(sizeof(m1_structfield));
    fld->name           = name;
    fld->type           = type;
    return fld;   
}

m1_struct *
newstruct(M1_compiler *comp, char *name, m1_structfield *fields) {
    m1_struct *str = (m1_struct *)m1_malloc(sizeof(m1_struct));    
    str->name      = name;
    str->fields    = fields;
    
    /* struct fields are linked in reverse order, so the first one is
       in fact the one last added, and therefore has the highest offset.
       The offset of the last field is the size of the whole struct
       except the size of the last field itself, so add that.
     */
    str->size = fields->offset + field_size(fields);
    
    return str;   
}


m1_expression *
vardecl(M1_compiler *comp, m1_valuetype type, m1_var *v) {
	m1_expression *expr = expression(comp, EXPR_VARDECL);
	
	expr->expr.v  = v;


	return expr;	
}

static m1_var *
make_var(M1_compiler *comp, char *name, m1_expression *init, unsigned size) {
    m1_var *v = (m1_var *)m1_malloc(sizeof(m1_var));
    v->name   = name;
    v->init   = init;
    v->size   = size;
    return v;    	
}

m1_var *
var(M1_compiler *comp, char *varname, m1_expression *init) {
    /* a single var is just size 1. */
	m1_var *v = make_var(comp, varname, init, 1);
    

	v->sym = sym_new_symbol(&(comp->currentchunk->locals),  /* enter in current chunk's symbol table */
	                       varname, 
	                       comp->parsingtype);              /* type of this variable */
		
	assert(v->sym != NULL);
		
	v->sym->var = v;
	return v;
}

m1_var *
array(M1_compiler *comp, char *varname, unsigned size, m1_expression *init) {
    /* XXX not sure about type of init */
    m1_var *v = make_var(comp, varname, init, size);
    v->sym = sym_new_symbol(&(comp->currentchunk->locals),  /* enter in current chunk's symbol table */
	                       varname, 
	                       comp->parsingtype);              /* type of this variable */
	
	assert(v->sym != NULL);
    v->sym->var = v;
	return v;
}

m1_expression *
ifexpr(M1_compiler *comp, m1_expression *cond, m1_expression *ifblock, m1_expression *elseblock) {
	m1_expression *expr = expression(comp, EXPR_IF);
	expr_set_if(comp, expr, cond, ifblock, elseblock);
	return expr;
}

m1_expression *
whileexpr(M1_compiler *comp, m1_expression *cond, m1_expression *block) {
	m1_expression *expr = expression(comp, EXPR_WHILE);
	expr_set_while(comp, expr, cond, block);
	return expr;	
}

m1_expression *
dowhileexpr(M1_compiler *comp, m1_expression *cond, m1_expression *block) {
	m1_expression *expr = expression(comp, EXPR_DOWHILE);
	expr_set_while(comp, expr, cond, block);
	return expr;	
}

unsigned 
field_size(struct m1_structfield *field) {
	switch (field->type) {
		case VAL_INT:
			return 4;
		case VAL_FLOAT:
			return 8;
		case VAL_STRING: /* pointer? */
			return 4;
		case VAL_CHUNK: /* pointer */
			return 4;
        case VAL_ADDRESS:
            return 4;
		default: /* look up size of type. XXX */
			return 4; /* fix this */	
	}	
}

static void
expr_set_switch(m1_expression *node, m1_expression *selector, m1_case *cases, m1_expression *defaultstat) {
	node->expr.s = (m1_switch *)m1_malloc(sizeof(m1_switch));
	node->expr.s->selector    = selector; 
	node->expr.s->cases       = cases;
	node->expr.s->defaultstat = defaultstat;
	
}

m1_expression *
switchexpr(M1_compiler *comp, m1_expression *selector, m1_case *cases, m1_expression *defaultstat) {
	m1_expression *node = expression(comp, EXPR_SWITCH);
	expr_set_switch(node, selector, cases, defaultstat); 
	return node;
}

m1_case *
switchcase(M1_compiler *comp, int selector, m1_expression *block) {
	m1_case *c  = (m1_case *)m1_malloc(sizeof(m1_case));
	c->selector = selector;
	c->block    = block;
	c->next     = NULL;
	return c;
}

m1_expression *
newexpr(M1_compiler *comp, char *type) {
	m1_expression *expr = expression(comp, EXPR_NEW);
	expr->expr.n        = (m1_newexpr *)m1_malloc(sizeof(m1_newexpr));
	expr->expr.n->type  = type;
	return expr;	
}

m1_expression *
castexpr(M1_compiler *comp, int type, m1_expression *castedexpr) {
    m1_expression *expr = expression(comp, EXPR_CAST);
    m1_castexpr *cast   = (m1_castexpr *)m1_malloc(sizeof(m1_castexpr));
    cast->type          = type;
    cast->expr          = castedexpr;
    expr->expr.cast     = cast;
    return expr;   
}
