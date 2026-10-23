//
// Created by Filip Halaš on 18/10/2026.
//

#ifndef IFJ_SYMTABLE_H
#define IFJ_SYMTABLE_H

typedef char *TKey;

typedef struct {
    char *data; //todo
} TData;

typedef struct SymTable SymTable;

SymTable *create_symtable(void);
void insert(SymTable *symtable, TKey k, TData d);
void symtable_delete(SymTable *symtable, TKey k);
TData *search(SymTable *symtable, TKey k);
void destroy_symtable(SymTable *symtable);

#endif // IFJ_SYMTABLE_H
