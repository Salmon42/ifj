//
// Created by rockuo on 9.10.16.
// This is library for implementation of IFJ16 class
//


#include "LexicalAnalyzer.h"

FILE *fp;


//local
void Error1() {
    fprintf(stderr, "Lexical Analyze Error \n");
    exit(1);
}

void equating(TOKEN *);

void start(TOKEN *);

void exclamationMark(TOKEN *);

void logical1(TOKEN *, int);

void slash(TOKEN *);

void inLineComment(TOKEN *);

void multiLineComment(TOKEN *);

void string1(TOKEN *);

void num(TOKEN *, int);

void doubleNum(TOKEN *, char * , int);

void doubleNumE(TOKEN *, char * , int);

void id(TOKEN *, int);

void idFull(TOKEN *, char *);

void magicRecognizer(TOKEN*,char*);

bool testValid(int);
// end local

void initializeStream(char *file) {
    fp = fopen(file, "r");

    if(fp == NULL) {
        printf("could not open source file");
        exit(99);
    }
}

void destroyStream() {
    fclose(fp);
}


TOKEN *getToken() {
    TOKEN *token = (TOKEN *) malloc(sizeof(TOKEN));
    start(token);
    return token;
}

void start(TOKEN *token) {
    int c = getc(fp);
    if (c == EOF) {
        token->type = END_OF_FILE;
    } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$') {
        id(token, c);
    } else if (c >= '0' && c <= '9') {
        num(token,c);
    } else if (c == '"') {
        string1(token);
    } else if (c == '\t' || c == '\n' || c == ' ') {
        start(token);
    } else if (c == '/') {
        slash(token);
    } else if (c == '*' || c == '+' || c == '-') {
        token->type = OPERATOR_ARITHMETIC;
        token->data.operatorArithmetic.name = (char) c;
    } else if (c == '<' || c == '>') {
        logical1(token, c);
    } else if (c == '=') {
        equating(token);
    } else if (c == '!') {
        exclamationMark(token);
    } else if (c == ';') {
        token->type = SEMICOLON;
    } else if (c == '(' || c == '{' || c == ')' || c == '}') {
        token->type = BRACKET;
        token->data.bracket.name = (char) c;
    } else if (c == ',') {
        token->type = SEPARATOR;
    } else {
        token->type = LEX_ERROR;
    }
}

void exclamationMark(TOKEN *token) {
    int c = getc(fp);
    if (c == '=') {
        token->type = OPERATOR_LOGIC;
        token->data.operatorLogic.name = "!=";
    } else {
        token->type = LEX_ERROR;
    }
}

void equating(TOKEN *token) {
    int c = getc(fp);
    if (c == '=' || c == '<' || c == '>') {
        char *str = "=.";
        str[1] = (char) c;
        token->type = OPERATOR_LOGIC;
        token->data.operatorLogic.name = str;
    } else if (testValid(c)) {
        token->type = OPERATOR_ASSIGN;
        ungetc(c, fp);
    } else {
        token->type = LEX_ERROR;
    }
}

void logical1(TOKEN *token, int c1) {
    int c2 = getc(fp);
    char *str = (char *) malloc(sizeof(char) * 3);
    str[0] = (char) c1;
    str[2] = '\0';
    if ((c2 == '=' || c2 == '<' || c2 == '>') && c2 != c1) {
        str[1] = (char) c2;
        token->type = OPERATOR_LOGIC;
        token->data.operatorLogic.name = str;
    } else if (testValid(c2)) {
        str[1] = '\0'; // plýtvání bytem
        token->type = OPERATOR_LOGIC;
        token->data.operatorLogic.name = str;
    } else {
        token->type = LEX_ERROR;
    }
}

void slash(TOKEN *token) {
    int c = getc(fp);
    if (c == '/') {
        inLineComment(token);
    } else if (c == '*') {
        multiLineComment(token);
    } else if (testValid(c)) {
        token->type = OPERATOR_ARITHMETIC;
        token->data.operatorArithmetic.name = (char) c;
    } else {
        token->type = LEX_ERROR;
    }
};

void inLineComment(TOKEN *token) {
    int c = getc(fp);
    while (c != '\n' && c != EOF) {
        c = getc(fp);
    }
    if (c == EOF) {
        token->type = END_OF_FILE;
    } else {
        start(token);
    }
}

void multiLineComment(TOKEN *token) {
    int c;
    bool canClose = false;
    while (true) {
        c = getc(fp);
        if (c == '*') {
            canClose = true;
        } else if (c == '/' && canClose) {
            start(token);
            return;
        } else if (c == EOF) {
            token->type = LEX_ERROR;
            return;
        } else {
            canClose = false;
        }
    }
}


void string1(TOKEN *token) {
    int c, i = 0;
    bool canEnd = true;
    char *str = (char*)malloc(sizeof(char));
    while (true) {
        c = getc(fp);
        if (c == '"' && canEnd) {
            str[i] = '\0';
            token->type = LITERAL_STRING;
            token->data.literalString.name = str;
            break;
        } else if (c == EOF) {
            free(str);
            token->type = LEX_ERROR;
            return;
        } else {
            str[i] = (char)c;
            i++;
            str = realloc(str,sizeof(char) * (i+1));
        }

        if(c == '\\' && canEnd){
            canEnd = false;
        } else {
            canEnd = true;
        }
    }
}

void num(TOKEN *token, int c) {
    char *str = (char*)malloc(sizeof(char) *2);
    str[0] = (char)c;
    int i = 1;
    while(true){
        c = getc(fp);
        if(c >= '0' && c <= '9'){
            str[i] = (char)c;
            i++;
            str = realloc(str, sizeof(char) * (i+1));
        }else if (c == '.'){
            str[i] = '.';
            i++;
            str = realloc(str, sizeof(char) * (i+1));
            doubleNum(token,str,i);
            break;
        } else if (c == 'e' || c == 'E'){
            str[i] = 'E';
            i++;
            str = realloc(str, sizeof(char) * (i+1));
            doubleNumE(token,str, i);
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_'){ // proměnná za číslem (bez oddělovače) je lexikální chyba...asi?
            free(str);
            token->type = LEX_ERROR;
            return;
        } else if (testValid(c)){
            ungetc(c,fp);
            str[i] = '\0';
            token->type = LITERAL_INTEGER;
            sscanf(str,"%d",&(token->data.numberInteger.value));
            break;
        } else {
            free(str);
            token->type = LEX_ERROR;
            return;
        }
    }
}

void doubleNum(TOKEN * token, char * str, int i){
    int c;
    bool first = true;
    while(true){
        c = getc(fp);
        if(first){
            if(c >= '0' && c <= '9'){
                first = false;
            }else {
                free(str);
                token->type = END_OF_FILE;
                return;
            }
        }

        if(c >= '0' && c <= '9'){
            str[i] = (char)c;
            i++;
            str = realloc(str, sizeof(char) * (i+1));
        } else if (c == 'e' || c == 'E'){
            str[i] = 'E';
            i++;
            str = realloc(str, sizeof(char) * (i+1));
            doubleNumE(token,str, i);
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_' || c == '.') {
            free(str);
            token->type = LEX_ERROR;
            return;
        } else if (testValid(c)){
            ungetc(c,fp);
            str[i] = '\0';
            token->type = LITERAL_DOUBLE;
            sscanf(str,"%lf",&(token->data.numberDouble.value));
            break;
        } else {
            free(str);
            token->type = LEX_ERROR;
            return;
        }
    }
}

void doubleNumE(TOKEN * token, char * str, int i){
    int c;
    bool canOperator = true, hasNumber= false;
    while(true){
        c = getc(fp);
        if(c == '+' || c == '-'){
            if(canOperator)
                canOperator = false;
            else{
                free(str);
                token->type = LEX_ERROR;
                return;
            }
        } else if (c >= '0' && c <='9'){
            hasNumber = true;
            canOperator = false;
            str[i] = (char)c;
            i++;
            str = realloc(str, sizeof(char) * (i+1));
        }  else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_' || c == '.') {
            free(str);
            token->type = LEX_ERROR;
            return;
        } else if (testValid(c) && hasNumber){
            ungetc(c,fp);
            str[i] = '\0';
            token->type = LITERAL_DOUBLE;
            sscanf(str,"%lf",&(token->data.numberDouble.value));
            break;
        }  else {
            free(str);
            token->type = LEX_ERROR;
            return;
        }
    }
}

void id(TOKEN *token, int c) {
    char *str = (char*)malloc(sizeof(char) *2);
    str[0] = (char)c;
    int i = 1;
    while(true){
        c = getc(fp);
        if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || (c >= '0' && c <= '9')){
            str[i] = (char)c;
            i++;
            str = realloc(str, sizeof(char) * (i+1));
        }else if (c == '.'){
            str[i] = '\0';
            idFull(token,str);
            break;
        }else if (testValid(c)){
            ungetc(c, fp);
            str[i] = '\0';
            magicRecognizer(token, str);
            break;
        }else{
            free(str);
            token->type = LEX_ERROR;
            return;
        }
    }
}

void idFull(TOKEN *token, char *str1) {
    char *str2 = (char*)malloc(sizeof(char) );
    int c, i = 0;

    c = getc(fp);
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$'){
        str2[i] = (char)c;
        i++;
        str2 = realloc(str2, sizeof(char) * (i+1));
    }else {
        free(str2);
        token->type = LEX_ERROR;
        return;
    }

    while(true){
        c = getc(fp);
        if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || (c >= '0' && c <= '9')){
            str2[i] = (char)c;
            i++;
            str2 = realloc(str2, sizeof(char) * (i+1));
//        }else if (c == '.'){   todo co teď?????
//            str2[i] = '\0';
//            idFull(token,str2);
        }else if (testValid(c)){
            ungetc(c, fp);
            str2[i] = '\0';
            token->type = IDENTIFIER_FULL;
            token->data.identifierFull.class = str1;
            token->data.identifierFull.name = str2;
            break;
        }else{
            free(str2);
            token->type = LEX_ERROR;
            return;
        }
    }
}

void magicRecognizer(TOKEN* token,char* str){
    if(
            strcmp(str,"break") == 0 || strcmp(str,"class") == 0 || strcmp(str,"continue") == 0 || strcmp(str,"do") == 0 ||
            strcmp(str,"double") == 0 || strcmp(str,"else") == 0 || strcmp(str,"false") == 0 || strcmp(str,"for") == 0 ||
            strcmp(str,"if") == 0 || strcmp(str,"int") == 0 || strcmp(str,"return") == 0 || strcmp(str,"String") == 0 ||
            strcmp(str,"static") == 0 || strcmp(str,"true") == 0 || strcmp(str,"void") == 0 || strcmp(str,"while" ) == 0
    ) {
//        printf("keyword");
        token->type = KEYWORD;
        token->data.keyword.name = str;
    } else {
        token->type = IDENTIFIER;
        token->data.identifier.name = str;
    }

}

bool testValid(int c) {
    if (c == EOF || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
        c == '$' || (c >= '0' && c <= '9') || c == '"' || c == '\t' || c == '\n' ||
        c == ' ' || c == '/' || c == '*' || c == '+' || c == '-' || c == '<' ||
        c == '>' || c == '=' || c == '!' || c == ';' || c == '(' || c == '{' ||
        c == ')' || c == '}' || c == ',') {
        return true;
    }
    return false;
}