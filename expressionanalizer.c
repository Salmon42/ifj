//
// Created by rockuo on 19.11.16.
//

#include "expressionanalizer.h"
#include "lexicalanalyzerstructures.h"
#include "symboltable.h"
#include "stack.h"
#include "basicstructures.h"
#include "semanticalanalyzer.h"
#include "debug.h"
#include "ifj16.h"

unsigned long iterator = 0;
char *varName = NULL;
int generate3AddressCode(tDLList *threeAddressCode,tStack *stack, tStack *backStack, bool firstPass);
int brackets = 0;
void concatenateString();
DATA_TYPE returnType;

void setVarName(char *name){
    if(varName != NULL)
        varName = (char*)realloc(varName,sizeof(char)*(strlen(name)+1));
    else
        varName = (char*)malloc(sizeof(char)*(strlen(name)+1));
    strcpy(varName,name);
}

void setReturnType(DATA_TYPE type){
    if(type == TYPE_BOOL){
        returnType = TYPE_BOOL;
        return;
    }
    if(type == TYPE_VOID){
        exit(4);
    }
    if(returnType == TYPE_INT){
        returnType = type;
        return;
    }else if(returnType == TYPE_DOUBLE &&  type!=TYPE_INT){
        returnType = type;
        return;
    }else{
        returnType = TYPE_STRING;
        return;
    }
}

bool stopNow = false;

static char terminalTable[14][14] = {
           /*+*/ /*-*//***//*(*//*)*//*I*//*/*//*<*//*>*//*>=*//*>=*//*==*//*!=*//*$*/
        /*+*/{'>', '>', '<', '<', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>'},
        /*-*/
             {'>', '>', '<', '<', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>'},
        /***/
             {'>', '>', '>', '<', '>', '<', '>', '>', '>', '>', '>', '>', '>', '>'},
        /*(*/
             {'<', '<', '<', '<', '=', '<', '<', '<', '<', '<', '<', '<', '<', 'X'},
        /*)*/
             {'>', '>', '>', 'X', '>', 'X', '>', '>', '>', '>', '>', '>', '>', '>'},
        /*I*/
             {'>', '>', '>', 'F', '>', 'X', '>', '>', '>', '>', '>', '>', '>', '>'},
        /*/*/
             {'>', '>', '>', '<', '>', '<', '>', '>', '>', '>', '>', '>', '>', '>'},
        /*<*/
             {'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X', 'X', '>'},
        /*>*/
             {'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X', 'X', '>'},
        /*<=*/
             {'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X', 'X', '>'},
        /*>=*/
             {'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X', 'X', '>'},
        /*==*/
             {'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X', 'X', '>'},
        /*!=*/
             {'<', '<', '<', '<', '>', '<', '<', 'X', 'X', 'X', 'X', 'X', 'X', '>'},
        /*$*/
             {'<', '<', '<', '<', 'X', '<', '<', '<', '<', '<', '<', '<', '<', 'S'}
};

DATA_TYPE getOutputType(DATA_TYPE type1, DATA_TYPE type2) {
    if (type1 == TYPE_STRING || type2 == TYPE_STRING) {
        return TYPE_STRING;
    } else if (type1 == TYPE_DOUBLE || type2 == TYPE_DOUBLE) {
        return TYPE_DOUBLE;
    } else {
        return TYPE_INT;
    }
}

EA_TERMINAL_TYPE getTerminalDataType(TOKEN token) {
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
            if (strcmp(token.data.operatorLogic.name, "<") == 0) {
                return EA_IS_LESS;
            } else if (strcmp(token.data.operatorLogic.name, ">") == 0) {
                return EA_IS_MORE;
            } else if (strcmp(token.data.operatorLogic.name, ">=") == 0) {
                return EA_IS_MORE_EQUAL;
            } else if (strcmp(token.data.operatorLogic.name, "<=") == 0) {
                return EA_IS_LESS_EQUAL;
            } else if (strcmp(token.data.operatorLogic.name, "==") == 0) {
                return EA_IS_EQUAL;
            } else if (strcmp(token.data.operatorLogic.name, "!=") == 0) {
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
                    if (brackets > 0) {
                        brackets--;
                        return EA_RIGHT_BR;
                    } else {
                        return EA_START_END;
                    }
                default:
                    break;
            }
        }
        case SEPARATOR:
            return EA_START_END;
        default:
            break;
    }
    return EA_UNKNOWN;
}

int parseExpression(tDLList *threeAddressCode, char **returnValName, DATA_TYPE *returnValType,bool firstPass) {
    debugPrintf("\nDEBUG expression START\n");
    returnType = TYPE_INT;
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

    TOKEN *token = getCachedToken();
    if ((token->type == BRACKET && token->data.bracket.name == ')') ||
        token->type == KEYWORD || token->type == OPERATOR_ASSIGN ||
        token->type == SEMICOLON || token->type == END_OF_FILE) {
        returnCachedTokens(1);
        return 2;
    }
    returnCachedTokens(1);

    terminalData.type = EA_EMPTY;
    while (true) {
        if (terminalData.type == EA_EMPTY) {
            // new from cache START
            terminalData.token = *getCachedToken(); // BECAUSE reasons
            terminalData.type = getTerminalDataType(terminalData.token);
            // new from cache END
        }

        if (terminalData.type == EA_UNKNOWN)return 2;


        stackTop(stack, &stackElement);
        switch (stackElement.type) {
            case EA_TERMINAL: {
                if (lookingForTerminal) {
                    char action = terminalTable[stackElement.data.terminalData.type][terminalData.type];
                    if (stopNow && action != 'S') {
                        return 2;
                    }
                    switch (action) {
                        case '<':
                            stackElement.type = EA_TERMINAL_ACTION;
                            stackElement.data.actionType = terminalData.type;
                            stackPush(stack, stackElement);

                            while (!stackEmpty(backStack)) {
                                stackTop(backStack, &stackElement);
                                stackPop(backStack);
                                stackPush(stack, stackElement);
                            }

                            stackElement.type = EA_TERMINAL;

                            stackElement.data.terminalData = terminalData;
                            stackPush(stack, stackElement);
                            terminalData.type = EA_EMPTY;
                            break;
                        case '>':
                            lookingForTerminal = false;
                            break;
                        case '=':
                            while (!stackEmpty(backStack)) {
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
                            return 2;//tODO check error
                        case 'S':
                            returnCachedTokens(1);
                            debugPrintf("\nDEBUG expression END\n");
                            //reset globals
                            stopNow = false;
                            //TODO
                            if(!firstPass) {
                                (*returnValName) = stringConcat(varName, "");
                                (*returnValType) = returnType;
                            }
                            return 0;
                        case 'F':
                            if (!stackEmpty(stack)) {
                                stackTop(stack, &stackElement);
                                stackPop(stack);
                                if(stackElement.type!=EA_TERMINAL && stackElement.data.terminalData.type==EA_START_END)return 2;
                            } else return 2;
                            if (!stackEmpty(stack)) {
                                stackTop(stack, &stackElement);
                                stackPop(stack);
                                if(stackElement.type!=EA_TERMINAL_ACTION)return 2;
                            } else return 2;
                            if (!stackEmpty(stack)) {
                                stackTop(stack, &stackElement);
                                stackPop(stack);
                                if(stackElement.type!=EA_TERMINAL && stackElement.data.terminalData.type==EA_I)return 2;
                            } else return 2;
                            if(!stackEmpty(stack))return 2;
                            brackets = 0;
                            returnCachedTokens(2);
                            return -1;
                        default:
                            debugPrintf("DEBUG END ");
                            exit(99);
                    }
                    break;
                }
            } //NO BREAL
            case EA_NOT_TERMINAL:
                stackPop(stack);
                stackPush(backStack, stackElement);
                break;
            case EA_TERMINAL_ACTION:
                if (lookingForTerminal) {
                    exit(99);
                }
                lookingForTerminal = true;
                int genRetVal = generate3AddressCode(threeAddressCode, stack, backStack, firstPass);
                if (genRetVal != 0) return genRetVal;
                break;
            default:
                exit(99);
        }
    }

}

INSTRUCTION_TYPE actionToLogicInstruction(EA_TERMINAL_TYPE actionType) {
    switch (actionType) {
        case EA_IS_LESS:
            return Instruction_Bool_Less;
        case EA_IS_MORE:
            return Instruction_Bool_More;
        case EA_IS_MORE_EQUAL:
            return Instruction_Bool_MoreEqual;
        case EA_IS_LESS_EQUAL:
            return Instruction_Bool_LessEqual;
        case EA_IS_EQUAL:
            return Instruction_Bool_Equals;
        case EA_IS_NOT_EQUAL:
            Instruction_Bool_EqualsNot;
        default:
            exit(99);
    }
}
void tokenTypeToVarTypeAndValue(TOKEN token, DATA_TYPE *varType, VARIABLE_VALUE *varVal) {
    switch (token.type) {
        case LITERAL_DOUBLE:
            if(varType != NULL)*varType = TYPE_DOUBLE;
            if(varVal != NULL)varVal->doubleValue = token.data.numberDouble.value;
            break;
        case LITERAL_STRING:
            if(varType != NULL)*varType = TYPE_STRING;
            if(varVal != NULL)varVal->stringValue = token.data.literalString.name;
            break;
        case LITERAL_INTEGER:
            if(varType != NULL)*varType = TYPE_INT;
            if(varVal != NULL)varVal->intValue = token.data.numberInteger.value;
            break;
        default:
            exit(99);
    }
}


int generate3AddressCode(tDLList *threeAddressCode, tStack *stack, tStack *backStack, bool firstPass) {
    STACK_ELEMENT stackElement1;
    STACK_ELEMENT stackElement2;
    STACK_ELEMENT stackElement3;
    stackTop(stack, &stackElement1);
    stackPop(stack);
    EA_TERMINAL_TYPE actionType = stackElement1.data.actionType;
    stackElement2.type = EA_TERMINAL_ACTION; // fake value

    switch (actionType) {
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

            if (!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement1);
                stackPop(backStack);
            } else return 2;

            if (!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement2);
                stackPop(backStack);
            } else return 2;

            if (!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement3);
                stackPop(backStack);
            } else return 2;

            if (!stackEmpty(backStack)) {
                return 2;
            }

            break;
        case EA_I:
            if (!stackEmpty(backStack)) {
                stackTop(backStack, &stackElement1);
                stackPop(backStack);
            } else return 2;

            if (!stackEmpty(backStack)) {
                return 2;
            }
            break;
        default:
            exit(114);
    }

    switch (actionType) {
        case EA_ADD:
            if (stackElement1.type == EA_NOT_TERMINAL &&
                stackElement2.type == EA_TERMINAL &&
                stackElement2.data.terminalData.type == EA_ADD &&
                stackElement3.type == EA_NOT_TERMINAL) {
                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,
                                                     stackElement3.data.notTerminalData.type);
                setReturnType(outputType);
                if (!firstPass) {
                    concatenateString();
                    char *tempName = (char *) malloc(sizeof(char) * 30);
                    strcpy(tempName, varName);
                    INSTRUCTION *instruction1 = createLocalVariable(tempName, outputType);
                    ListInsertLast(threeAddressCode,createInstruction(instruction1));

                    INSTRUCTION *instruction2 = createInstrMath(Instruction_Addition, tempName,
                                                   stackElement1.data.notTerminalData.name,
                                                   stackElement3.data.notTerminalData.name);
                    ListInsertLast(threeAddressCode,createInstruction(instruction2));
                    stackElement1.data.notTerminalData.name = tempName;
                } else {
                    stackElement1.data.notTerminalData.name = "tempName";
                }
                debugPrintf("generate: E->E+E\n");
                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return 2;
            break;
        case EA_SUB:
            if (stackElement1.type == EA_NOT_TERMINAL &&
                stackElement2.type == EA_TERMINAL &&
                stackElement2.data.terminalData.type == EA_SUB &&
                stackElement3.type == EA_NOT_TERMINAL) {
                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,
                                                     stackElement3.data.notTerminalData.type);
                setReturnType(outputType);
                if (!firstPass) {

                    concatenateString();

                    if (outputType == TYPE_STRING) return 4;

                    char *tempName = (char *) malloc(sizeof(char) * 30);
                    strcpy(tempName, varName);
                    INSTRUCTION *instruction1 = createLocalVariable(tempName, outputType);
                    ListInsertLast(threeAddressCode,createInstruction(instruction1));

                    INSTRUCTION *instruction2 = createInstrMath(Instruction_Subtraction, tempName,
                                                   stackElement1.data.notTerminalData.name,
                                                   stackElement3.data.notTerminalData.name);
                    ListInsertLast(threeAddressCode,createInstruction(instruction2));
                    stackElement1.data.notTerminalData.name = tempName;
                } else {
                    stackElement1.data.notTerminalData.name = "tempName";
                }
                debugPrintf("generate: E->E-E\n");
                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return 2; //todo check
            break;
        case EA_MUL:
            if (stackElement1.type == EA_NOT_TERMINAL &&
                stackElement2.type == EA_TERMINAL &&
                stackElement2.data.terminalData.type == EA_MUL &&
                stackElement3.type == EA_NOT_TERMINAL) {
                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,
                                                     stackElement3.data.notTerminalData.type);
                setReturnType(outputType);
                if (!firstPass) {
                    concatenateString();

                    if (outputType == TYPE_STRING) return 4;

                    char *tempName = (char *) malloc(sizeof(char) * 30);
                    strcpy(tempName, varName);
                    INSTRUCTION *instruction1 = createLocalVariable(tempName, outputType);
                    ListInsertLast(threeAddressCode,createInstruction(instruction1));

                    INSTRUCTION *instruction2 = createInstrMath(Instruction_Multiply, tempName,
                                                   stackElement1.data.notTerminalData.name,
                                                   stackElement3.data.notTerminalData.name);
                    ListInsertLast(threeAddressCode,createInstruction(instruction2));

                    stackElement1.data.notTerminalData.name = tempName;
                } else {
                    stackElement1.data.notTerminalData.name = "tempName";
                }
                debugPrintf("generate: E->E*E\n");

                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return 2; //todo check

            break;
        case EA_DIV:
            if (stackElement1.type == EA_NOT_TERMINAL &&
                stackElement2.type == EA_TERMINAL &&
                stackElement2.data.terminalData.type == EA_DIV &&
                stackElement3.type == EA_NOT_TERMINAL) {

                DATA_TYPE outputType = getOutputType(stackElement1.data.notTerminalData.type,
                                                     stackElement3.data.notTerminalData.type);
                setReturnType(outputType);
                if (!firstPass) {
                    concatenateString();

                    if (outputType == TYPE_STRING) return 4; else outputType = TYPE_DOUBLE;

                    char *tempName = (char *) malloc(sizeof(char) * 30);
                    strcpy(tempName, varName);

                    INSTRUCTION *instruction1 = createActualLocalVariable(tempName, outputType);
                    ListInsertLast(threeAddressCode,createInstruction(instruction1));

                    INSTRUCTION *instruction2 = createActualLocalVariable(tempName, outputType);
                    createInstrMath(Instruction_Multiply, tempName,
                                    stackElement1.data.notTerminalData.name,
                                    stackElement3.data.notTerminalData.name);
                    ListInsertLast(threeAddressCode,createInstruction(instruction2));

                    stackElement1.data.notTerminalData.name = tempName;
                } else {
                    stackElement1.data.notTerminalData.name = "tempName";
                }
                debugPrintf("generate: E->E/E\n");
                stackElement1.data.notTerminalData.type = outputType;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return 2; //todo check
            break;
        case EA_LEFT_BR:
            if (stackElement1.type == EA_TERMINAL &&
                stackElement1.data.terminalData.type == EA_LEFT_BR &&
                stackElement2.type == EA_NOT_TERMINAL &&
                stackElement3.data.terminalData.type == EA_RIGHT_BR &&
                stackElement3.type == EA_TERMINAL) {
                debugPrintf("generate: E->(E)\n");
                stackElement1 = stackElement2;
            } else return 2; //todo check

            break;
        case EA_I:
            //TODO tests/8/test2.txt
            if (stackElement1.type == EA_TERMINAL &&
                stackElement1.data.terminalData.type == EA_I) {
//                stackElement2 = temp :)

                if (stackElement1.data.terminalData.token.type == IDENTIFIER) {
                    stackElement2.data.notTerminalData.name = stackElement1.data.terminalData.token.data.identifier.name;
                    setVarName(stackElement2.data.notTerminalData.name);
                    if (!firstPass) {
                        SYMBOL_TABLE_VARIABLE *symbolTableVariable= semantic_getInitializedVariable(stackElement2.data.notTerminalData.name);
                        stackElement2.data.notTerminalData.type = symbolTableVariable->type;
                    } else {
                        stackElement2.data.notTerminalData.type = TYPE_INT;
                    }

                    debugPrintf("generate: E->i where i = ID\n");

                    setReturnType(stackElement2.data.notTerminalData.type);
                } else if (stackElement1.data.terminalData.token.type == IDENTIFIER_FULL) {
                    if (!firstPass) {
                        char *tempName = (char *) malloc(
                                sizeof(char) * (1
                                                +
                                                strlen(stackElement1.data.terminalData.token.data.identifierFull.class)
                                                +
                                                strlen(stackElement1.data.terminalData.token.data.identifierFull.class)
                                )
                        );
                        sprintf(
                                tempName,
                                "%s.%s",
                                stackElement1.data.terminalData.token.data.identifierFull.class,
                                stackElement1.data.terminalData.token.data.identifierFull.name
                        );
                        setVarName(tempName);
                        stackElement2.data.notTerminalData.name = tempName;
                        SYMBOL_TABLE_VARIABLE *symbolTableVariable= semantic_getInitializedVariable(tempName);
                        stackElement2.data.notTerminalData.type = symbolTableVariable->type;
                    } else {
                        stackElement2.data.notTerminalData.type = TYPE_INT;
                        stackElement2.data.notTerminalData.name = "tepName";
                    }
                    setReturnType(stackElement2.data.notTerminalData.type );
                    debugPrintf("generate: E->i where i = ID_FULL\n");
                } else {
                    if (!firstPass) {
                        concatenateString();
                        char *tempName = (char *) malloc(sizeof(char) * 30);
                        strcpy(tempName, varName);
                        VARIABLE_VALUE *varVal = (VARIABLE_VALUE*)malloc(sizeof(VARIABLE_VALUE));
                        DATA_TYPE *varType = (DATA_TYPE*)malloc(sizeof(DATA_TYPE));
                        tokenTypeToVarTypeAndValue(stackElement1.data.terminalData.token, varType,varVal);
                        INSTRUCTION *instruction1 = pushActualLocalVariable(tempName,*varType,*varVal);
                        ListInsertLast(threeAddressCode,createInstruction(instruction1));

                        stackElement2.data.notTerminalData.type = *varType;
                        free(varVal);
                        free(varType);
                        stackElement2.data.notTerminalData.name = tempName;
                    } else {
                        stackElement2.data.notTerminalData.type = TYPE_INT;
                        stackElement2.data.notTerminalData.name = "tepName";
                    }
                    setReturnType(stackElement2.data.notTerminalData.type );
                    debugPrintf("generate: E->i where i = LIT\n");
                }

                stackElement2.type = EA_NOT_TERMINAL;
                stackElement1 = stackElement2;
            } else return 2; //todo check
            break;
        case EA_IS_LESS:
        case EA_IS_MORE:
        case EA_IS_MORE_EQUAL:
        case EA_IS_LESS_EQUAL:
        case EA_IS_EQUAL:
        case EA_IS_NOT_EQUAL:
            if (stackElement1.type == EA_NOT_TERMINAL &&
                stackElement2.type == EA_TERMINAL &&
                //               stackElement2.data.terminalData.type == EA_ADD &&
                stackElement3.type == EA_NOT_TERMINAL) {
                if (!firstPass) {
                    concatenateString();
                    char *tempName = (char *) malloc(sizeof(char) * 30);
                    strcpy(tempName, varName);
                    INSTRUCTION *instruction1 = createActualLocalVariable(tempName, TYPE_INT);
                    ListInsertLast(threeAddressCode,createInstruction(instruction1));
                    INSTRUCTION *instruction2 =  createInstrExprEval(actionToLogicInstruction(actionType), tempName,
                                                          stackElement1.data.notTerminalData.name,
                                                          stackElement3.data.notTerminalData.name);
                    ListInsertLast(threeAddressCode,createInstruction(instruction2));
                    debugPrintf("generate: E->E_LOGIC_E\n");
                    stackElement1.data.notTerminalData.name = tempName;
                } else {
                    stackElement1.data.notTerminalData.name = "tempName";
                }
                stackElement1.data.notTerminalData.type = TYPE_INT;
                stackElement1.type = EA_NOT_TERMINAL;
            } else return 2; //todo check
            break;
        default:
            exit(99);
    }

    stackPush(stack, stackElement1);
    return 0;
};


void concatenateString() {
    if(varName != NULL)
        varName = (char*)realloc(varName,sizeof(char)*100);
    else
        varName = (char*)malloc(sizeof(char)*100);
    iterator++;
    sprintf(varName, "#ExpressionAnalyzerVar%lu", iterator);
}

