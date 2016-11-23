//
// Created by rockuo on 19.11.16.
//

#include "ExpressionAnalizer.h"
#include "LexicalAnalyzerStructures.h"
#include "SymbolTable.h"
#include "Stack.h"

unsigned long iterator = 0;
char varName[100];
bool generate3AddressCode(tStack *stack,tStack *backStack, SYMBOL_TABLE_NODEPtr *globalSymbolTable, SYMBOL_TABLE_NODEPtr *localSymbolTable, SYMBOL_TABLE_FUNCTION *calledFunction);
void concatenateString();
bool stopNow = false;

static char terminalTable[14][14] = {
             /*+*/ /*-*//***//*(*//*)*//*I*//*/*//*<*//*>*//*>=*//*>=*//*==*//*!=*//*$*/
        /*+*/{'>', '>', '<', '<', '>', '<', '<', '>', '>', '>', '>', '>',    '>',    '>'},
        /*-*/{'>', '>', '<', '<', '>', '<', '<', '>', '>', '>', '>', '>',    '>',    '>'},
        /***/{'>', '>', '>', '<', '>', '<', '>', '>', '>', '>', '>', '>',    '>',    '>'},
        /*(*/{'<', '<', '<', '<', '=', '<', '<', '<', '<', '<', '<', '<',    '<',    'X'},
        /*)*/{'>', '>', '>', 'X', '>', 'X', '>', '>', '>', '>', '>', '>',    '>',    '>'},
        /*I*/{'>', '>', '>', 'X', '>', 'X', '>', '>', '>', '>', '>', '>',    '>',    '>'},
        /*/*/{'>', '>', '>', '<', '>', '<', '>', '>', '>', '>', '>', '>',    '>',    '>'},
        /*<*/{'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X',    'X',    '>'},
        /*>*/{'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X',    'X',    '>'},
        /*<=*/{'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X',  'X', 'X',  'X',    '>'},
        /*>=*/{'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X',  'X', 'X',  'X',    '>'},
        /*==*/{'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X',  'X', 'X',  'X',    '>'},
        /*!=*/{'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X',  'X', 'X',  'X',    '>'},
        /*$*/{'<', '<', '<', '<', 'X', '<', '<', '<', '<', '<',  '<', '<',   '<',    'S'}
};

DATA_TYPE getOutputType(DATA_TYPE type1,DATA_TYPE type2){
    if(type1 == TYPE_STRING || type2 == TYPE_STRING){
        return TYPE_STRING;
    }else if(type1 == TYPE_DOUBLE || type2 == TYPE_DOUBLE){
        return TYPE_DOUBLE;
    }else{
        return TYPE_INT;
    }
}

EA_TERMINAL_TYPE getTerminalDataType(TOKEN token) {
    static int brackets = 0;
    switch (token.type) {
        case KEYWORD:
        case OPERATOR_ASSIGN:
        case END_OF_FILE:
            return EA_UNKNOWN;
        case SEMICOLON:
            return EA_START_END;
        case IDENTIFIER_FULL:
        case IDENTIFIER:
        case LITERAL_STRING:
        case LITERAL_INTEGER:
        case LITERAL_DOUBLE:
            return EA_I;
        case OPERATOR_ARITHMETIC:
            switch (token.data.operatorArithmetic.name) {
                case '+':
                    return EA_ADD;
                case '-':
                    return EA_SUB;
                case '*':
                    return EA_MUL;
                case '/':
                    return EA_DIV;
                default:
                    break;
            }
            break;
        case OPERATOR_LOGIC:
            if (strcmp(token.data.operatorLogic.name, "<")) {
                return EA_IS_LESS;
            } else if (strcmp(token.data.operatorLogic.name, ">")) {
                return EA_IS_MORE;
            } else if (strcmp(token.data.operatorLogic.name, ">=")) {
                return EA_IS_MORE_EQUAL;
            } else if (strcmp(token.data.operatorLogic.name, "<=")) {
                return EA_IS_LESS_EQUAL;
            } else if (strcmp(token.data.operatorLogic.name, "==")) {
                return EA_IS_EQUAL;
            }else if (strcmp(token.data.operatorLogic.name, "!=")) {
                return EA_IS_NOT_EQUAL;
            } else {
                break;
            }
        case BRACKET: {
            switch (token.data.bracket.name) {
                case '(':
                    brackets++;
                    return EA_LEFT_BR;
                case ')':
                    if(brackets > 0){
                        brackets--;
                        return EA_RIGHT_BR;
                    }else{
                        return EA_START_END;
                    }
                default:
                    break;
            }
        }
        default:
            break;
    }
    return EA_UNKNOWN;
}

bool parseExpression(tDLList *threeAddressCode, char *returnVal, SYMBOL_TABLE_NODEPtr *globalSymbolTable, SYMBOL_TABLE_NODEPtr *localSymbolTable, SYMBOL_TABLE_FUNCTION *calledFunction) {
    printf("\nDEBUG expression START\n");
    bool lookingForTerminal = true;
    STACK_ELEMENT stackElement;
    tStack *stack = malloc(sizeof(tStack));
    tStack *backStack = malloc(sizeof(tStack));
    EA_TERMINAL_DATA terminalData;
    stackInit(stack);
    stackInit(backStack);
    // push $
    stackElement.type = EA_TERMINAL;
    stackElement.data.terminalData.type = EA_START_END;

    stackPush(stack, stackElement);

    //todo: validate
    TOKEN *token = getCachedToken();
    if((token->type == BRACKET && token->data.bracket.name == ')') ||
            token->type == KEYWORD || token->type == OPERATOR_ASSIGN ||
            token->type == SEMICOLON || token->type == END_OF_FILE){
        returnCachedTokens(1);
        return false;
    }
    returnCachedTokens(1);

    terminalData.type = EA_EMPTY;
    while (true) {
        if(terminalData.type == EA_EMPTY) {
            // new from cache START
            terminalData.token = *getCachedToken(); // BECAUSE reasons
            terminalData.type = getTerminalDataType(terminalData.token);
            // new from cache END
        }

        if(terminalData.type == EA_UNKNOWN)return false;


        stackTop(stack, &stackElement);
        switch (stackElement.type){
            case EA_TERMINAL: {
                if(lookingForTerminal) {
                    char  action = terminalTable[stackElement.data.terminalData.type][terminalData.type];
                    if(stopNow && action != 'S'){
                        return false;
                    }
                    switch (action) {
                        case '<':
                            stackElement.type = EA_TERMINAL_ACTION;
                            stackElement.data.actionType=terminalData.type;
                            stackPush(stack,stackElement);

                            while(!stackEmpty(backStack)){
                                stackTop(backStack, &stackElement);
                                stackPop(backStack);
                                stackPush(stack, stackElement);
                            }

                            stackElement.type = EA_TERMINAL;

                            stackElement.data.terminalData = terminalData;
                            stackPush(stack,stackElement);
                            terminalData.type = EA_EMPTY;
                            break;
                        case '>':
                            lookingForTerminal = false;
                            break;
                        case '=':
                            while(!stackEmpty(backStack)) {
                                stackTop(backStack, &stackElement);
                                stackPop(backStack);
                                stackPush(stack, stackElement);
                            }
                            stackElement.type = EA_TERMINAL;
                            stackElement.data.terminalData = terminalData;
                            stackPush(stack, stackElement);

                            terminalData.type = EA_EMPTY;
                            break;
                        case 'X':
                            return false;
                        case 'S':
                            returnCachedTokens(1);
                            printf("\nDEBUG expression END\n");
                            //reset globals
                            stopNow = false;
                            //TODO vhen NOT NULL
//                            varName => returnVal
                            return true;

                        default:
                            exit(136);
                    }
                    break;
                }
            } //NO BREAL
            case EA_NOT_TERMINAL:
                stackPop(stack);
                stackPush(backStack, stackElement);
                break;
            case EA_TERMINAL_ACTION:
                if(lookingForTerminal){
                    exit(112);
                }
                lookingForTerminal = true;
                if(!generate3AddressCode(stack, backStack, localSymbolTable,globalSymbolTable, calledFunction)){
                    return false;
                }
                break;
            default:
                exit(113);
        }
    }

}

bool generate3AddressCode(tStack *stack,tStack *backStack, SYMBOL_TABLE_NODEPtr *globalSymbolTable, SYMBOL_TABLE_NODEPtr *localSymbolTable, SYMBOL_TABLE_FUNCTION *calledFunction){
    STACK_ELEMENT stackElement1;
    STACK_ELEMENT stackElement2;
    STACK_ELEMENT stackElement3;
    stackTop(stack,&stackElement1);
    stackPop(stack);
    EA_TERMINAL_TYPE actionType = stackElement1.data.actionType;
    stackElement2.type = EA_TERMINAL_ACTION; // fake value

    switch (actionType){
        case EA_IS_LESS:
        case EA_IS_MORE:
        case EA_IS_MORE_EQUAL:
        case EA_IS_LESS_EQUAL:
        case EA_IS_EQUAL:
        case EA_IS_NOT_EQUAL:
            stopNow = true;
        case EA_SUB:
        case EA_MUL:
        case EA_DIV:
        case EA_LEFT_BR:
        case EA_ADD:

            if(!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement1);
                stackPop(backStack);
            }else return false;

            if(!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement2);
                stackPop(backStack);
            }else return false;

            if(!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement3);
                stackPop(backStack);
            }else return false;

            if(!stackEmpty(backStack)){
                return false;
            }

            break;
        case EA_I:
            if(!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement1);
                stackPop(backStack);
            }else return false;

            if(!stackEmpty(backStack)){
                return false;
            }
            break;
        default:
            exit(114);
    }

    switch (actionType){
        case EA_ADD:
            if(stackElement1.type == EA_NOT_TERMINAL &&
                    stackElement2.type == EA_TERMINAL &&
                    stackElement2.data.terminalData.type == EA_ADD &&
                    stackElement3.type == EA_NOT_TERMINAL)
            {
                concatenateString();
                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,stackElement3.data.notTerminalData.type);

                char *tempName = (char*)malloc(sizeof(char)*30);
                strcpy(tempName,varName);
                // TODO push variable
                createInstructionMathOperation(Instruction_Addition,tempName,stackElement1.data.notTerminalData.name,stackElement2.data.notTerminalData.name);

                printf("generate: E->E+E\n");
                stackElement1.data.notTerminalData.name =tempName;
                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return  false;
            break;
        case EA_SUB:
            if(stackElement1.type == EA_NOT_TERMINAL &&
               stackElement2.type == EA_TERMINAL &&
               stackElement2.data.terminalData.type == EA_SUB &&
               stackElement3.type == EA_NOT_TERMINAL)
            {
                concatenateString();
                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,stackElement3.data.notTerminalData.type);

                if(outputType == TYPE_STRING) exit(3);

                char *tempName = (char*)malloc(sizeof(char)*30);
                strcpy(tempName,varName);
                // TODO push variable

                createInstructionMathOperation(Instruction_Subtraction,tempName,stackElement1.data.notTerminalData.name,stackElement2.data.notTerminalData.name);

                printf("generate: E->E-E\n");
                stackElement1.data.notTerminalData.name =tempName;
                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return  false;
            break;
        case EA_MUL:
            if(stackElement1.type == EA_NOT_TERMINAL &&
               stackElement2.type == EA_TERMINAL &&
               stackElement2.data.terminalData.type == EA_MUL &&
               stackElement3.type == EA_NOT_TERMINAL)
            {
                concatenateString();

                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,stackElement3.data.notTerminalData.type);

                if(outputType == TYPE_STRING) exit(3);

                char *tempName = (char*)malloc(sizeof(char)*30);
                strcpy(tempName,varName);
                // TODO push variable

                createInstructionMathOperation(Instruction_Multiply,tempName,stackElement1.data.notTerminalData.name,stackElement2.data.notTerminalData.name);
                printf("generate: E->E*E\n");
                stackElement1.data.notTerminalData.name =tempName;
                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            }else return  false;

            break;
        case EA_DIV:
            if(stackElement1.type == EA_NOT_TERMINAL &&
               stackElement2.type == EA_TERMINAL &&
               stackElement2.data.terminalData.type == EA_DIV &&
               stackElement3.type == EA_NOT_TERMINAL)
            {
                concatenateString();

                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,stackElement3.data.notTerminalData.type);

                if(outputType == TYPE_STRING) exit(3); else outputType = TYPE_DOUBLE;

                char *tempName = (char*)malloc(sizeof(char)*30);
                strcpy(tempName,varName);
                // TODO push variable
                createInstructionMathOperation(Instruction_Multiply,tempName,stackElement1.data.notTerminalData.name,stackElement2.data.notTerminalData.name);
                printf("generate: E->E/E\n");
                stackElement1.data.notTerminalData.name =tempName;
                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            }else  return  false;
            break;
        case EA_LEFT_BR:
            if(stackElement1.type == EA_TERMINAL &&
               stackElement1.data.terminalData.type == EA_LEFT_BR &&
               stackElement2.type == EA_NOT_TERMINAL &&
               stackElement3.data.terminalData.type == EA_RIGHT_BR &&
               stackElement3.type == EA_TERMINAL)
            {
                printf("generate: E->(E)\n");
                stackElement1 = stackElement2;
            }else return  false;

            break;
        case EA_I:
            if(stackElement1.type == EA_TERMINAL &&
               stackElement1.data.terminalData.type == EA_I)
            {
//                stackElement2 = temp :)

                if(stackElement1.data.terminalData.token.type == IDENTIFIER){
                    stackElement2.data.notTerminalData.name = stackElement1.data.terminalData.token.data.identifier.name;
//                     TODO when not null
                    SYMBOL_TABLE_VARIABLE *symbolTableVariable = getVariable(localSymbolTable, globalSymbolTable, calledFunction, stackElement2.data.notTerminalData.name);
                    stackElement2.data.notTerminalData.type = symbolTableVariable->type;
                    printf("generate: E->i where i = ID\n");
                }else if(stackElement1.data.terminalData.token.type == IDENTIFIER_FULL){
                    char *tempName = (char*)malloc(sizeof(char) * (1+strlen(stackElement1.data.terminalData.token.data.identifierFull.class) + strlen(stackElement1.data.terminalData.token.data.identifierFull.class)));
                    sprintf(
                            tempName,
                            "%s.%s",
                            stackElement1.data.terminalData.token.data.identifierFull.class,
                            stackElement1.data.terminalData.token.data.identifierFull.name
                    );
                    stackElement2.data.notTerminalData.name = tempName;
//                    todo when not null
                    SYMBOL_TABLE_VARIABLE *symbolTableVariable = getVariable(localSymbolTable, globalSymbolTable, calledFunction, stackElement2.data.notTerminalData.name);
                    stackElement2.data.notTerminalData.type = symbolTableVariable->type;
                    printf("generate: E->i where i = ID_FULL\n");
                }else {
                    concatenateString();
                    char *tempName = (char*)malloc(sizeof(char)*30);
                    strcpy(tempName,varName);
                    DATA_TYPE varType;
                    switch (stackElement1.data.terminalData.token.type){
                        case LITERAL_DOUBLE:
                            varType = TYPE_DOUBLE;
                            break;
                        case LITERAL_STRING:
                            varType = TYPE_STRING;
                            break;
                        case LITERAL_INTEGER:
                            varType = TYPE_INT;
                            break;
                        default:
                            exit(115);
                    }
                    //todo create local var

                    stackElement2.data.notTerminalData.type = varType;
                    stackElement2.data.notTerminalData.name = tempName;
                }
                printf("generate: E->i where i = LIT\n");
                stackElement2.type = EA_NOT_TERMINAL;
                stackElement1=stackElement2;
            }else return  false;
            break;
        case EA_IS_LESS:
        case EA_IS_MORE:
        case EA_IS_MORE_EQUAL:
        case EA_IS_LESS_EQUAL:
        case EA_IS_EQUAL:
        case EA_IS_NOT_EQUAL:
            if(stackElement1.type == EA_NOT_TERMINAL &&
               stackElement2.type == EA_TERMINAL &&
//               stackElement2.data.terminalData.type == EA_ADD &&
               stackElement3.type == EA_NOT_TERMINAL)
            {
                concatenateString();

                char *tempName = (char*)malloc(sizeof(char)*30);
                strcpy(tempName,varName);
                // TODO push variable
                // TDO generate LogicOporation

                printf("generate: E->E_LOGIC_E\n");
                stackElement1.data.notTerminalData.name =tempName;
                stackElement1.data.notTerminalData.type = TYPE_INT;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return  false;
            break;
        default:
            exit(116);
    }

    stackPush(stack,stackElement1);
    return  true;
};


void concatenateString(){
    iterator++;
    sprintf(varName, "#ExpressionAnalyzerVar%lu", iterator);
}

