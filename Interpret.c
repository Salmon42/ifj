//
// Created by Andrej on 19.11.2016.
//

// contains:
/*    #include<stdio.h>
      #include<stdlib.h>
      #include<string.h>
      #include<stdarg.h>
      #include<stdbool.h>
*/
#include "ifj16.h"  // Built-in functions & most libraries
#include "Stack.h"
#include "Interpret.h"
#include "BasicStructures.h"

int Interpret(tDLList *InstructionList, tDLList *globalFrame, tStack *stackOfLocalFrames);



// YET TO DO
void executeInstructionIf(INSTRUCTION *instr);

//..

int Interpret( tDLList *InstructionList, tDLList *globalFrame, tStack *stackOfLocalFrames ){
    if (InstructionList == NULL) return 99;
    printf("-----------------------------------------------------\n");
    printf("\n----- Welcome to hell v1.0\n");
    printf("-----------------------------------------------------\n");

    //NewPtr - pointer to list element (allowing work with instructions inside InstructionList)
    LIST_ELEMENT *NewPtr = malloc(sizeof(struct LIST_ELEMENT));   //TODO: how to free and exit?
    INSTRUCTION *Instr;

    tDLList *upcomingLocalFrame = NULL; // creating the pointer, yet not using it

    // DON'T EVER FORGET THIS
    ListFirst(InstructionList);

    while ( 1 ) {   // The great cycle
        // Copy the actual instruction from the list
        ListElementCopy(InstructionList, NewPtr);
        Instr = NewPtr->data.instr;

        if (Instr->type == Instruction_End_Interpret) {
            //TODO: return value, free all resources used by interpret (stack & globalframe)

            //VARIABLE *printer = findFrameVariable(globalFrame, "vysledok2");
            //printf("|%g|", printer->value.doubleValue);

            printf("-----------------------------------------------------\n");
            printf("----- I am ending!\n");
            return 0;
        }

        // Instruction to create both global frame & stack of local frames (the stack will be empty!)
        if (Instr->type == Instruction_Create_GlobalFrame_And_LocalStack) {
            globalFrame = createFrame();
            stackOfLocalFrames = createFrameStack();
            ListSuccessor(InstructionList);
            continue; // Jump to next instruction
        }

        if (Instr->type == Instruction_Create_Local_Frame) {
            upcomingLocalFrame = createFrame();
            ListSuccessor(InstructionList);
            continue; // Jump to next instruction
        }

        // Inserting variable into global frame (with or without value)
        if (Instr->type == Instruction_Create_Global_Variable || Instr->type == Instruction_Push_Global_Variable) {
            pushToFrame(globalFrame, Instr);
            ListSuccessor(InstructionList);
            continue; // Jump to next instruction
        }
        // Inserting variable into UPCOMING local frame (the frame is not in stack)
        if (Instr->type == Instruction_Create_Local_Variable || Instr->type == Instruction_Push_Local_Variable) {
            if (upcomingLocalFrame == NULL) {
                return 99;
            }
            pushToFrame(upcomingLocalFrame, Instr);
            ListSuccessor(InstructionList);
            continue; // Jump to next instruction
        }

        // Pushing upcoming local frame into stack of local frames
        // THINGS TO KEEP: global frame, stack of local frames
        if (Instr->type == Instruction_CallFunction) {
            printf("CALLING FUNCTION!!! GET REKT\n");
            //return 99;
            //TODO: Call Function, recursion inside
            // many things to do
            // but...
            pushFrameToStack(stackOfLocalFrames, upcomingLocalFrame);
            // HERE COMES THE FUCKING RECURSION
            Interpret((tDLList*)Instr->address_dst, globalFrame, stackOfLocalFrames);
        }

        if (Instr->type == Instruction_ReturnFunction) {
            printf("Returning from function!");
            return 0;
            //TODO end recursion, return 0 here
        }

        // other special instructions: IF & WHILE
        if (Instr->type == Instruction_IF) {
            printf("HANDLING IF_INSTRUCTION");
            return 42;
        }

        if (Instr->type == Instruction_WHILE) {
            printf("HANDLING WHILE_INSTRUCTION");
            return 42;
        }

        //--- Special instructions are captured, now we will execute the rest ---

        // EXECUTING OTHER INSTRUCTIONS, REPOINTING REQUIRED
        VARIABLE *dst = NULL;
        VARIABLE *src1 = NULL;
        VARIABLE *src2 = NULL;

        // Getting pointer to the top of local frame stack
        tDLList *actualLocalFrame = getActualLocalFrame(stackOfLocalFrames);
        if (actualLocalFrame != NULL) {
            //existing local frame
            dst = findFrameVariable(actualLocalFrame, Instr->address_dst);
            src1 = findFrameVariable(actualLocalFrame, Instr->address_src1);
            src2 = findFrameVariable(actualLocalFrame, Instr->address_src2);
        }
        else {
            // Not found in local frame
            dst = findFrameVariable(globalFrame, Instr->address_dst);
            src1 = findFrameVariable(globalFrame, Instr->address_src1);
            src2 = findFrameVariable(globalFrame, Instr->address_src2);
        }

        switch (Instr->type) {
            case Instruction_Assign:    // expecting DST & SRC variable name
                ;
                if ( dst ==NULL || src1 == NULL || src2 != NULL ){
                    //TODO: free?
                    return 99;
                }

                executeInstructionAssign(dst, src1);
                break;

            case Instruction_Addition:
            case Instruction_Subtraction:
            case Instruction_Multiply:
            case Instruction_Divide:
                ;
                if ( dst ==NULL || src1 == NULL || src2 == NULL ){
                    //TODO: dst, src1 or src2 not found
                    //free resources
                    return 99;
                }

                int mathRetValue = executeInstructionMathOperation(Instr->type, dst, src1, src2);
                if ( mathRetValue != 0 ){
                    printf("you have fucked this up AS HELL\n");
                    return mathRetValue;
                }

                break;

            case Instruction_Bool_Equals:
            case Instruction_Bool_EqualsNot:
            case Instruction_Bool_More:
            case Instruction_Bool_Less:
            case Instruction_Bool_MoreEqual:
            case Instruction_Bool_LessEqual:
                printf("Calling function for evaluation bool expression.\n");
                if ( dst ==NULL || src1 == NULL || src2 == NULL ){
                    //TODO: dst, src1 or src2 not found
                }

                // passing 3 pointers to variables in FRAMES!
                INSTRUCTION *exprInstruction = malloc(sizeof(INSTRUCTION));

                exprInstruction->type = Instr->type;
                // i hope this sorcery is fine
                *(VARIABLE *)exprInstruction->address_dst = *dst;
                *(VARIABLE *)exprInstruction->address_src1 = *src1;
                *(VARIABLE *)exprInstruction->address_src2 = *src2;

                executeInstructionExpressionEvaluation(exprInstruction);

                *dst = *(VARIABLE *)exprInstruction->address_dst;

                ;
                break;

            default:
                ;
                printf("TRYING TO HANDLE INSTRUCTION WITHOUT SOLUTION\n");
                //InstructionExecute(Instr);
                break;
        }

        // break condition
        if (InstructionList->Last->element.data.instr == Instr) break;
        ListSuccessor(InstructionList);
    } // end of the big cycle

    return 0; // I had no idea what have I done
}

//...

// to be squished fully
void InstructionExecute(INSTRUCTION *instr){
// Always: overiť či sedia typy premenných ktoré idem použiť na danú operáciu, inak bude behová chyba
    // error 7 (behová chyba pri načítaní číselnej hodnoty zo vstupu)
    // error 8 (behová chyba pri práci s neinicializovanou premennou)
    // error 9 (behová chyba delenia nulou)
    // error 10 (ostatné behové chyby) <- zlá premenná... možno sem?

    // code shortening
    VARIABLE *dstAddr = ((VARIABLE*)instr->address_dst);
    VARIABLE *srcAddr1 = ((VARIABLE*)instr->address_src1);
    VARIABLE *srcAddr2 = ((VARIABLE*)instr->address_src2);

    VARIABLE_VALUE *dstVal = &dstAddr->value;
    VARIABLE_VALUE *src1Val = &srcAddr1->value;
    VARIABLE_VALUE *src2Val = &srcAddr2->value;

    //The Giant Switch
    switch ( instr -> type ) {

        //----------------------------------------------------------------------------------------------------------------
        // BUILD-IN FUNCTIONS
        //----------------------------------------------------------------------------------------------------------------
        case Instruction_Function_readInt:
            // int ifj16_readInt();
            dstVal->intValue = ifj16_readInt();
            break;

        case Instruction_Function_readDouble:
            // double ifj16_readDouble();
            dstVal->doubleValue = ifj16_readDouble();
            break;

        case Instruction_Function_readString:
            // char *ifj16_readString();
            dstVal->stringValue = ifj16_readString();
            break;

        case Instruction_Function_Print:
            // void ifj16_print(char *s);
            //TODO: lots of things to figure out
            break;

        case Instruction_Function_Length:
            // int ifj16_length(char *);
            dstVal->intValue = ifj16_length( src1Val->stringValue );
            break;

        case Instruction_Function_Substr:
            // char *ifj16_substr(char *, int, int);
            // asi mame kurva problem
            break;

        case Instruction_Function_Compare:
            // int ifj16_compare(char *, char *);
            dstVal->intValue = ifj16_compare( src1Val->stringValue , src2Val->stringValue );
            break;

        case Instruction_Function_Find:
            // int ifj16_find(char *, char *);
            dstVal->intValue = ifj16_find( src1Val->stringValue , src2Val->stringValue );
            break;

        case Instruction_Function_Sort:
            // char *ifj16_sort(char *s);
            dstVal->stringValue = ifj16_sort( src1Val->stringValue );
            break;

        default: ;
    } // end of giant switch

} // end of InstructionExecute

//...

int executeInstructionMathOperation(INSTRUCTION_TYPE instrType, VARIABLE *dst, VARIABLE *src1, VARIABLE *src2) {

    switch ( instrType ) {

    case Instruction_Addition:
        ;
        if (src1->type==TYPE_STRING && src2->type==TYPE_STRING) {
            // Concatenate strings, withtout casting

            //tell interpret the execution has failed
            if (dst->type!=TYPE_STRING) {
                printf("Wrong destination format.\n");
                return 99;
            }

            dst->value.stringValue = stringConcat(src1->value.stringValue, src2->value.stringValue);
            return 0; // end instruction
        }
        if ((src1->type==TYPE_STRING && src2->type!=TYPE_STRING) || (src1->type!=TYPE_STRING && src2->type==TYPE_STRING)) {
            // Concatenate strings, with casting

            //tell interpret the execution has failed
            if (dst->type!=TYPE_STRING) {
                printf("Wrong destination format.\n");
                return 99;
            }

            char tempString[100];
            char *typeCastSrc = NULL;

            if ( src1->type!=TYPE_STRING ){
                // Switch data type of src1 to string
                if ( src1->type==TYPE_INT ) {
                    sprintf(tempString,"%d", src1->value.intValue);
                    int len = (int)strlen(tempString);
                    typeCastSrc = malloc( sizeof(char)*len+1 );
                    strcpy(typeCastSrc, tempString);
                }
                else if ( src1->type==TYPE_DOUBLE ) {
                    sprintf(tempString,"%g", src1->value.doubleValue);
                    int len = (int)strlen(tempString) +1;
                    typeCastSrc = malloc( sizeof(char)*len );
                    strcpy(typeCastSrc, tempString);
                }
            }   //end of "type casting" of SRC1 into 'typeCastSrc'

            if ( src2->type!=TYPE_STRING ) {
                // Switch data type of src2 to string
                if ( src2->type==TYPE_INT ) {
                    sprintf(tempString,"%d", src2->value.intValue);
                    int len = (int)strlen(tempString) +1;
                    typeCastSrc = malloc( sizeof(char)*len );
                    strcpy(typeCastSrc, tempString);
                }
                else if ( src2->type==TYPE_DOUBLE ) {
                    sprintf(tempString,"%g", src2->value.doubleValue);
                    int len = (int)strlen(tempString) +1;
                    typeCastSrc = malloc( sizeof(char)*len );
                    strcpy(typeCastSrc, tempString);
                }
            }   //end of "type casting" of SRC2 into 'typeCastSrc'

            // Performing concatenation
            if ( src1->type!=TYPE_STRING )
                dst->value.stringValue = stringConcat(typeCastSrc, src2->value.stringValue);
            else
                dst->value.stringValue = stringConcat(src1->value.stringValue, typeCastSrc);

            return 0; // end instruction
        }
        switch ( src1->type ) {   // both are Double or Ints

            case TYPE_INT:  // SRC1 is int
                ; // SRC2 is int
                if (src2->type==TYPE_INT) dst->value.intValue = src1->value.intValue + src2->value.intValue;
                else {  // SRC2 is double
                    if (dst->type==TYPE_INT){ // DST is int
                        dst->type = TYPE_DOUBLE;
                        dst->value.doubleValue = (double)dst->value.intValue;
                    }
                    dst->value.doubleValue = (double)src1->value.intValue + src2->value.doubleValue;
                }
            return 0; // end instruction

            case TYPE_DOUBLE:
                ;
                if (dst->type==TYPE_INT) {
                    dst->type = TYPE_DOUBLE;  // remember the type cast
                    dst->value.doubleValue = (double)dst->value.intValue;
                }
                if (src2->type==TYPE_DOUBLE)
                    dst->value.doubleValue = src1->value.doubleValue + src2->value.doubleValue;
                else
                    dst->value.doubleValue = src1->value.doubleValue + (double)src2->value.intValue;

            return 0; // end instruction

            default: ;
        }
    break;

    case Instruction_Subtraction:
        ;
        if (src1->type == TYPE_STRING || src2->type == TYPE_STRING)
            return 10; // cannot subtract strings
        switch ( src1->type ) {   // both are Double or Ints

            case TYPE_INT:  // SRC1 is int
                ; // SRC2 is int
                if (src2->type==TYPE_INT) dst->value.intValue = src1->value.intValue - src2->value.intValue;
                else {  // SRC2 is double
                    if (dst->type==TYPE_INT){ // DST is int
                        dst->type = TYPE_DOUBLE;  // remember the type cast
                        dst->value.doubleValue = (double)dst->value.intValue;
                    }
                    dst->value.doubleValue = (double)src1->value.intValue - src2->value.doubleValue;
                }
            return 0; // end instruction

            case TYPE_DOUBLE:
                ;
                if (dst->type==TYPE_INT) {
                    dst->type = TYPE_DOUBLE;  // remember the type cast
                    dst->value.doubleValue = (double)dst->value.intValue;
                }
                if (src2->type==TYPE_DOUBLE) dst->value.doubleValue = src1->value.doubleValue - src2->value.doubleValue;
                else dst->value.doubleValue = src1->value.doubleValue - (double)src2->value.intValue;
            return 0; // end instruction

            default: ;
        }
    break;

    case Instruction_Multiply:
        ;
        if ( src1->type == TYPE_STRING || src2->type == TYPE_STRING)
            return 10;  // cannot multiply strings
        switch ( src1->type ) {   // both are Double or Ints

            case TYPE_INT:  // SRC1 is int
                ; // SRC2 is int
                if (src2->type==TYPE_INT) dst->value.intValue = src1->value.intValue * src2->value.intValue;
                else {  // SRC2 is double
                    if (dst->type==TYPE_INT){ // DST is int
                        dst->type = TYPE_DOUBLE;
                        dst->value.doubleValue = (double)dst->value.intValue;
                    }
                    dst->value.doubleValue = (double)src1->value.intValue * src2->value.doubleValue;
                }
            return 0; // end instruction

            case TYPE_DOUBLE:
                ;
                if (dst->type==TYPE_INT) {
                    dst->type = TYPE_DOUBLE;
                    dst->value.doubleValue = (double)dst->value.intValue;
                }
                if (src2->type==TYPE_DOUBLE) dst->value.doubleValue = src1->value.doubleValue * src2->value.doubleValue;
                else dst->value.doubleValue = src1->value.doubleValue * (double)src2->value.intValue;
            return 0; // end instruction

            default: ;
        }
    break;

    case Instruction_Divide:
        ;
        if ( src1->type == TYPE_STRING || src2->type == TYPE_STRING )
            return 10; // cannot divide strings

        switch ( src1->type ) {   // both are Double or Ints

            case TYPE_INT:  // SRC1 is int
                ; // SRC2 is int
                if ( src2->type == TYPE_INT ) {
                    if ( src2->value.intValue == 0 ) return 9; // dividing by zero
                    dst->value.intValue = src1->value.intValue / src2->value.intValue;
                }
                else {  // SRC2 is double
                    if ( dst->type == TYPE_INT ){ // DST is int
                        dst->type = TYPE_DOUBLE;
                        dst->value.doubleValue = (double)dst->value.intValue;
                    }
                    if ( src2->value.doubleValue == 0.0) return 9; // dividing by zero
                    dst->value.doubleValue = (double)src1->value.intValue / src2->value.doubleValue;
                }
            return 0; // end instruction

            case TYPE_DOUBLE:
                ;
                if ( dst->type == TYPE_INT ) {
                    dst->type = TYPE_DOUBLE;
                    dst->value.doubleValue = (double)dst->value.intValue;
                }
                if ( src2->type == TYPE_DOUBLE ) {
                    if ( src2->value.doubleValue == 0.0 ) return 9; // dividing by zero
                    dst->value.doubleValue = src1->value.doubleValue / src2->value.doubleValue;
                }
                else {
                    if ( src2->value.intValue == 0 ) return 9; // dividing by zero
                    dst->value.doubleValue = src1->value.doubleValue / (double)src2->value.intValue;
                }
            return 0; // end instruction

            default: ;
        }
    break;

    default: ;
    }   // end if Math Instructions switch

    return 0;
}   // end of Executing

//...

void executeInstructionExpressionEvaluation(INSTRUCTION *instr) {
    // code shortening: access to data value
    VARIABLE_VALUE *dstVal = &((VARIABLE*)instr->address_dst)->value;
    VARIABLE_VALUE *src1Val = &((VARIABLE*)instr->address_src1)->value;
    VARIABLE_VALUE *src2Val = &((VARIABLE*)instr->address_src2)->value;
    // code shortening: access to data type
    DATA_TYPE dstType = ((VARIABLE*)instr->address_dst)->type;
    DATA_TYPE src1Type = ((VARIABLE*)instr->address_src1)->type;
    DATA_TYPE src2Type = ((VARIABLE*)instr->address_src2)->type;

    switch ( instr->type ) {
        case Instruction_Bool_Equals:   // if ( src1 == src2 ) dst=TRUE else dst=FALSE;
            ;
            if (src1Type == TYPE_STRING || src2Type == TYPE_STRING) exitInterpret(10);

            switch (src1Type){
                case TYPE_INT:
                    ;
                    if (src2Type==TYPE_INT){ // src1->int == src2->int
                        if (src1Val->intValue == src2Val->intValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_DOUBLE) { // src1->int == src2->double
                        src1Val->doubleValue = (double) src1Val->intValue;
                        if (src1Val->doubleValue == src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                case TYPE_DOUBLE:
                    ;
                    if (src2Type==TYPE_DOUBLE){ // src1->double == src2->double
                        if (src1Val->doubleValue == src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_INT) { // src1->double == src2->int
                        src2Val->doubleValue = (double) src2Val->intValue;
                        if (src1Val->doubleValue == src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                default: ;
            }

            break;
        case Instruction_Bool_EqualsNot:    // if ( src1 != src2 ) dst=TRUE else dst=FALSE;
            ;
            if (src1Type == TYPE_STRING || src2Type == TYPE_STRING) exitInterpret(10);

            switch (src1Type){
                case TYPE_INT:
                    ;
                    if (src2Type==TYPE_INT){ // src1->int != src2->int
                        if (src1Val->intValue != src2Val->intValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_DOUBLE) { // src1->int != src2->double
                        src1Val->doubleValue = (double) src1Val->intValue;
                        if (src1Val->doubleValue != src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                case TYPE_DOUBLE:
                    ;
                    if (src2Type==TYPE_DOUBLE){ // src1->double != src2->double
                        if (src1Val->doubleValue == src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_INT) { // src1->double != src2->int
                        src2Val->doubleValue = (double) src2Val->intValue;
                        if (src1Val->doubleValue == src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                default: ;
            }

            break;
        case Instruction_Bool_More: // if ( src1 > src2 ) dst=TRUE else dst=FALSE;
            ;
            if (src1Type == TYPE_STRING || src2Type == TYPE_STRING) exitInterpret(10);

            switch (src1Type){
                case TYPE_INT:
                    ;
                    if (src2Type==TYPE_INT){ // src1->int > src2->int
                        if (src1Val->intValue > src2Val->intValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_DOUBLE) { // src1->int > src2->double
                        src1Val->doubleValue = (double) src1Val->intValue;
                        if (src1Val->doubleValue > src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                case TYPE_DOUBLE:
                    ;
                    if (src2Type==TYPE_DOUBLE){ // src1->double > src2->double
                        if (src1Val->doubleValue > src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_INT) { // src1->double > src2->int
                        src2Val->doubleValue = (double) src2Val->intValue;
                        if (src1Val->doubleValue > src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                default: ;
            }
            break;
        case Instruction_Bool_Less: // if ( src1 < src2 ) dst=TRUE else dst=FALSE;
            ;
            if (src1Type == TYPE_STRING || src2Type == TYPE_STRING) exitInterpret(10);

            switch (src1Type){
                case TYPE_INT:
                    ;
                    if (src2Type==TYPE_INT){ // src1->int < src2->int
                        if (src1Val->intValue < src2Val->intValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_DOUBLE) { // src1->int < src2->double
                        src1Val->doubleValue = (double) src1Val->intValue;
                        if (src1Val->doubleValue < src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                case TYPE_DOUBLE:
                    ;
                    if (src2Type==TYPE_DOUBLE){ // src1->double < src2->double
                        if (src1Val->doubleValue < src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_INT) { // src1->double < src2->int
                        src2Val->doubleValue = (double) src2Val->intValue;
                        if (src1Val->doubleValue < src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                default: ;
            }
            break;
        case Instruction_Bool_MoreEqual:    // if ( src1 >= src2 ) dst=TRUE else dst=FALSE;
            ;
            if (src1Type == TYPE_STRING || src2Type == TYPE_STRING) exitInterpret(10);

            switch (src1Type){
                case TYPE_INT:
                    ;
                    if (src2Type==TYPE_INT){ // src1->int >= src2->int
                        if (src1Val->intValue >= src2Val->intValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_DOUBLE) { // src1->int >= src2->double
                        src1Val->doubleValue = (double) src1Val->intValue;
                        if (src1Val->doubleValue >= src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                case TYPE_DOUBLE:
                    ;
                    if (src2Type==TYPE_DOUBLE){ // src1->double >= src2->double
                        if (src1Val->doubleValue >= src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_INT) { // src1->double >= src2->int
                        src2Val->doubleValue = (double) src2Val->intValue;
                        if (src1Val->doubleValue >= src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                default: ;
            }
            break;
        case Instruction_Bool_LessEqual:    // if ( src1 <= src2 ) dst=TRUE else dst=FALSE;
            ;
            if (src1Type == TYPE_STRING || src2Type == TYPE_STRING) exitInterpret(10);

            switch (src1Type){
                case TYPE_INT:
                    ;
                    if (src2Type==TYPE_INT){ // src1->int <= src2->int
                        if (src1Val->intValue <= src2Val->intValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_DOUBLE) { // src1->int <= src2->double
                        src1Val->doubleValue = (double) src1Val->intValue;
                        if (src1Val->doubleValue <= src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                case TYPE_DOUBLE:
                    ;
                    if (src2Type==TYPE_DOUBLE){ // src1->double >= src2->double
                        if (src1Val->doubleValue <= src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";

                    } else if (src2Type==TYPE_INT) { // src1->double >= src2->int
                        src2Val->doubleValue = (double) src2Val->intValue;
                        if (src1Val->doubleValue <= src2Val->doubleValue) dstVal->stringValue = "TRUE";
                        else dstVal->stringValue = "FALSE";
                    } else {
                        // error: unsupported format
                        exitInterpret(10);
                    }
                    break;

                default: ;
            }
            break;

        default: ;
    }


}

//...

void executeInstructionAssign(VARIABLE *dst, VARIABLE *src) {
    int type_dst = dst->type;
    int type_src = src->type;

    switch (type_dst) {

        case TYPE_INT:
            if (type_src==TYPE_INT) dst->value.intValue = src->value.intValue;
            else exitInterpret(6); // Incompatibile assign
            break;

        case TYPE_DOUBLE:
            if (type_src==TYPE_DOUBLE) { // Correct assignemt
                dst->value.doubleValue = src->value.doubleValue;
            }
            else if (type_src==TYPE_INT) { // Type cast INT->DOUBLE & assign
                dst->value.doubleValue = (double)src->value.intValue;
            }
            else exitInterpret(6); // Incompatibile assign
            break;

        case TYPE_STRING:
            if (type_src==TYPE_STRING) dst->value.stringValue = src->value.stringValue;
            else exitInterpret(6);
            break;

        default: ;
    }
}   // end of Assign instruction

//...

void executeInstructionIf(INSTRUCTION *instr) {

    // yet to do

}

//...

void exitInterpret(int errNumber) {
    switch (errNumber) {
        case 6:
            fprintf(stderr, "Semantic error: other.\n");
            break;
        case 7:
            fprintf(stderr, "Runtime error: loading values from input.\n");
            break;
        case 8:
            fprintf(stderr, "Runtime error: working with uninitialized variables\n");
            break;
        case 9:
            fprintf(stderr, "Runtime error: dividing by zero.\n");
            break;
        case 10:
            fprintf(stderr, "Interpret: Other runtime error.\n");
            break;
        case 99:
            fprintf(stderr, "Intern error: Unable to allocate memory.");
            break;
        default: ;
    } // end of switch

    //TODO: free all allocated memory

    exit(errNumber);
}

//...

void checkMalloc(void *ptr){
    if ( ptr == NULL ) {
        fprintf(stderr, "Intern error: Unable to allocate memory.\n");
        exitInterpret(99);
    }
}