/*


*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "compiler.h"
#include "symtab.h"
#include "decl.h"
#include "stack.h"

m1_symboltable *
new_symtab(void) {
    m1_symboltable *table = (m1_symboltable *)calloc(1, sizeof (m1_symboltable));
    if (table == NULL) {
        fprintf(stderr, "cant alloc new symtab");
        exit(EXIT_FAILURE);   
    }
    table->syms = NULL;
    return table;   
}

void 
init_symtab(m1_symboltable *symtab) {
    symtab->syms        = NULL;
    symtab->parentscope = NULL;    
}

/*

Add symbol C<sym> to symboltable C<table>. 

*/
static void
link_sym(m1_symboltable *table, m1_symbol *sym) {
    m1_symbol *iter;
    
    assert(table != NULL);
    assert(sym != NULL);
    
    iter = table->syms;
    
    if (iter == NULL) {
        table->syms = sym;
    }
    else {
        
        /* go to end of list, and hook on symbol at the end 
        This is needed because the symbols in the constants segment 
        must be stored in-order. XXX Make this more efficient.
        
        */
        while (iter->next != NULL) {
            iter = iter->next;
        }      
        iter->next = sym;
    }

}

static m1_symbol *
mk_sym(void) {
    m1_symbol *sym = (m1_symbol *)calloc(1, sizeof(m1_symbol));
   	
    if (sym == NULL) {
        fprintf(stderr, "cant alloc mem for sym");
        exit(EXIT_FAILURE);
    }      
    return sym;
}

m1_symbol *
sym_new_symbol(M1_compiler *comp, m1_symboltable *table, char *varname, char *type, unsigned num_elems) {
    m1_symbol *sym = NULL;
    
    assert(varname != NULL);
    assert(type != NULL);
    assert(table != NULL);
    
    /* check whether symbol exists already. */
    sym = sym_lookup_symbol(table, varname);
    
    if (sym != NULL) {
        fprintf(stderr, "Error (line %d): already declared a variable '%s'\n", 
                        yyget_lineno(comp->yyscanner), varname); 
        ++comp->errors;  
        return sym;  
    }
    /* if it existed, the function would have returned by now. */
    sym = mk_sym();
    
    sym->num_elems = num_elems;  /* for arrays. */
    sym->name      = varname;    /* name of this symbol */
    sym->regno     = NO_REG_ALLOCATED_YET; /* need to allocate a register later. */  
    sym->next      = NULL;    /* symbols are stored in a list */
    sym->type_name = type;    /* store the name of the type, as it may not have been defined yet. */

    link_sym(table, sym);
    
    return sym;   
}

m1_symbol *
sym_lookup_symbol(m1_symboltable *table, char *name) {
    m1_symbol *sym = NULL;
    
    assert(table != NULL);
    assert(name != NULL);
    
    sym = table->syms;
        
    while (sym != NULL) {        
    
        assert(sym->name != NULL);
        assert(name != NULL);  
   
        if (strcmp(sym->name, name) == 0) {     
            return sym;
        }   
            
        sym = sym->next;   
    }
    
    /* try and find the symbol in the parent scope, recursively. */
    if (table->parentscope != NULL) {
        return sym_lookup_symbol(table->parentscope, name);
    }
        
    return NULL;
}


void
print_symboltable(m1_symboltable *table) {
    m1_symbol *iter = table->syms;
    fprintf(stderr, "SYMBOL TABLE\n");
    while (iter != NULL) {
        fprintf(stderr, "symbol '%s' [%s] has register %d and type %d\n", iter->value.sval, 
                                                                          iter->name, 
                                                                          iter->regno, 
                                                                          iter->valtype);
        iter = iter->next;   
    }   
}

m1_symbol *
sym_enter_str(M1_compiler *comp, m1_symboltable *table, char *str) {
    m1_symbol *sym;
    
    assert(table != NULL);
    
    sym = sym_find_str(table, str);
    
    if (sym) {     
    	return sym;
    }
    	
   	sym = mk_sym();   
    
    sym->value.sval = str;
    sym->valtype    = VAL_STRING;
    sym->constindex = comp->constindex++;
    
    link_sym(table, sym);
    return sym;    
}

m1_symbol *
sym_enter_chunk(M1_compiler *comp, m1_symboltable *table, char *name) {
    m1_symbol *sym;
    /* a chunk is just stored as a name, but override the type. */
    sym          = sym_enter_str(comp, table, name);        
    sym->valtype = VAL_CHUNK;
    return sym;       
}

m1_symbol *
sym_find_chunk(m1_symboltable *table, char *name) {
    return sym_find_str(table, name);   
}

m1_symbol *
sym_enter_num(M1_compiler *comp, m1_symboltable *table, double val) {
    m1_symbol *sym;
    
    sym = sym_find_num(table, val);
    if (sym)
    	return sym;
    	
    sym = mk_sym();
    
    sym->value.fval = val;
    sym->valtype    = VAL_FLOAT;
    sym->constindex = comp->constindex++;
    
    link_sym(table, sym);
    
    return sym;    
}

m1_symbol *
sym_enter_int(M1_compiler *comp, m1_symboltable *table, int val) {
    m1_symbol *sym;
           
    sym = sym_find_int(table, val);
    if (sym) {
    	return sym;
    }
        	
    sym = mk_sym();
    
    sym->value.ival = val;
    sym->valtype    = VAL_INT;    
    sym->constindex = comp->constindex++;
    sym->next       = NULL;
    
    link_sym(table, sym);
    return sym;    
}

m1_symbol *
sym_find_str(m1_symboltable *table, char *name) {
    m1_symbol *sym;
    
    assert(table != NULL);
    assert(name != NULL);
    
    sym = table->syms;
        
    while (sym != NULL) {        
        if (sym->valtype == VAL_STRING || sym->valtype == VAL_CHUNK) {
            assert(sym->value.sval != NULL);
            assert(name != NULL);  
                           
            if (strcmp(sym->value.sval, name) == 0) {     
                return sym;
            }   
        }
            
        sym = sym->next;   
    }
    return NULL;
}



m1_symbol *
sym_find_num(m1_symboltable *table, double fval) {
    m1_symbol *sym = table->syms;
    
    while (sym != NULL) {
        /* exact comparison on floats usually doesn't work, but 
           this does work for the exact literal constant floats
           that are used in a program.
         */
        if (sym->valtype == VAL_FLOAT) {
            if (sym->value.fval == fval) {        	
                return sym;            
            }
        }
            
        sym = sym->next;   
    }
    return NULL;
}

m1_symbol *
sym_find_int(m1_symboltable *table, int ival) {
    m1_symbol *sym = table->syms;
    
    while (sym != NULL) {
        if (sym->valtype == VAL_INT) {
            if (sym->value.ival == ival) {
                return sym;
            }
        }    
        sym = sym->next;   
    }
    return NULL;
}



