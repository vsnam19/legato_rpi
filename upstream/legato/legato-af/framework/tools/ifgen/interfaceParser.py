# $ANTLR 3.5.3 interface.g 2025-03-20 23:29:06

import sys
from antlr3 import *
from antlr3.compat import set, frozenset


import os
import sys

import interfaceIR
from interfaceLexer import interfaceLexer

def UnquoteString(quotedString):
    return quotedString[1:-1].decode('string_escape')

def StripPostComment(comment):
    return comment[4:]

def StripPreComment(comment):
    """Get text from C-style doxygen comment."""
    commentLines=comment[3:-2].split('\n')
    strippedCommentLines=[]
    inVerbatim=False
    for commentLine in commentLines:
        if commentLine.startswith (' *'):
             commentLine = commentLine[2:]
        strippedCommentLines.append(commentLine)
        if commentLine.strip() == '@verbatim':
            inVerbatim = True
        elif commentLine.strip() == '@endverbatim':
            inVerbatim = False

    result = '\n'.join(strippedCommentLines)
    return result



# for convenience in actions
HIDDEN = BaseRecognizer.HIDDEN

# token types
EOF=-1
T__33=33
T__34=34
T__35=35
T__36=36
T__37=37
T__38=38
T__39=39
T__40=40
T__41=41
T__42=42
T__43=43
T__44=44
T__45=45
ALPHA=4
ALPHANUM=5
API_MSG_SIZE=6
API_VERSION=7
BITMASK=8
CPP_COMMENT=9
C_COMMENT=10
DEC_NUMBER=11
DEFINE=12
DOC_POST_COMMENT=13
DOC_PRE_COMMENT=14
ENUM=15
EVENT=16
FUNCTION=17
HANDLER=18
HEXNUM=19
HEX_NUMBER=20
IDENTIFIER=21
IN=22
NUM=23
OUT=24
QUOTED_STRING=25
REFERENCE=26
SCOPED_IDENTIFIER=27
SEMICOLON=28
SERVING_VERSION=29
STRUCT=30
USETYPES=31
WS=32

# token names
tokenNames = [
    "<invalid>", "<EOR>", "<DOWN>", "<UP>",
    "ALPHA", "ALPHANUM", "API_MSG_SIZE", "API_VERSION", "BITMASK", "CPP_COMMENT", 
    "C_COMMENT", "DEC_NUMBER", "DEFINE", "DOC_POST_COMMENT", "DOC_PRE_COMMENT", 
    "ENUM", "EVENT", "FUNCTION", "HANDLER", "HEXNUM", "HEX_NUMBER", "IDENTIFIER", 
    "IN", "NUM", "OUT", "QUOTED_STRING", "REFERENCE", "SCOPED_IDENTIFIER", 
    "SEMICOLON", "SERVING_VERSION", "STRUCT", "USETYPES", "WS", "'('", "')'", 
    "'*'", "'+'", "','", "'-'", "'..'", "'/'", "'='", "'['", "']'", "'{'", 
    "'}'"
]




class interfaceParser(Parser):
    grammarFileName = "interface.g"
    api_version = 1
    tokenNames = tokenNames

    def __init__(self, input, state=None, *args, **kwargs):
        if state is None:
            state = RecognizerSharedState()

        super(interfaceParser, self).__init__(input, state, *args, **kwargs)

        self.dfa14 = self.DFA14(
            self, 14,
            eot = self.DFA14_eot,
            eof = self.DFA14_eof,
            min = self.DFA14_min,
            max = self.DFA14_max,
            accept = self.DFA14_accept,
            special = self.DFA14_special,
            transition = self.DFA14_transition
            )

        self.dfa16 = self.DFA16(
            self, 16,
            eot = self.DFA16_eot,
            eof = self.DFA16_eof,
            min = self.DFA16_min,
            max = self.DFA16_max,
            accept = self.DFA16_accept,
            special = self.DFA16_special,
            transition = self.DFA16_transition
            )

        self.dfa20 = self.DFA20(
            self, 20,
            eot = self.DFA20_eot,
            eof = self.DFA20_eof,
            min = self.DFA20_min,
            max = self.DFA20_max,
            accept = self.DFA20_accept,
            special = self.DFA20_special,
            transition = self.DFA20_transition
            )




        self.iface = interfaceIR.Interface()
        self.searchPath = []
        self.compileErrors = 0


        self.delegates = []





    INVALID_NUMBER = 0xcdcdcdcd

    def getTokenErrorDisplay(self, t):
        """
        Display a token for an error message.  Since doc comments can be multi-line, only display
        the first line of the token.
        """
        s = t.text
        if s is None:
            # No text -- fallback to default handler
            return super(self, Parser).getTokenErrorDisplay(t)
        else:
            s = s.split('\n', 2)[0]

        return repr(s)

    def getErrorHeader(self, e):
        """
        What is the error header, normally line/character position information?
        """

        source_name = self.getSourceName()
        if source_name is not None:
            return "{}:{}:{}: error:".format(source_name, e.line, e.charPositionInLine)
        return "{}:{}: error:".format(e.line, e.charPositionInLine)

    def getErrorHeaderForToken(self, token):
        """
        What is the error header for a token, normally line/character position information?
        """

        source_name = self.getSourceName()
        if source_name is not None:
            return "{}:{}:{}: error:".format(source_name, token.line, token.charPositionInLine)
        return "{}:{}: error:".format(token.line, token.charPositionInLine)

    def getWarningHeaderForToken(self, token):
        """
        What is the warning header for a token, normally line/character position information?
        """

        source_name = self.getSourceName()
        if source_name is not None:
            return "{}:{}:{}: warning:".format(source_name, token.line, token.charPositionInLine)
        return "{}:{}: warning:".format(token.line, token.charPositionInLine)

    def getLocationTuple(self, token):
        """
        Returns a tuple specifying the exact location of a token:
        (path, line, col)
        """
        return (self.getSourceName(),
            token.line,
            token.charPositionInLine)




    # $ANTLR start "apiVersion"
    # interface.g:245:1: apiVersion returns [int version] : API_VERSION '=' d= DEC_NUMBER ';' ;
    def apiVersion(self, ):
        version = None


        d = None

        try:
            try:
                # interface.g:246:5: ( API_VERSION '=' d= DEC_NUMBER ';' )
                # interface.g:246:7: API_VERSION '=' d= DEC_NUMBER ';'
                pass 
                self.match(self.input, API_VERSION, self.FOLLOW_API_VERSION_in_apiVersion547)

                self.match(self.input, 41, self.FOLLOW_41_in_apiVersion549)

                d = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_apiVersion553)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_apiVersion555)

                #action start
                version = int(d.text) 
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return version

    # $ANTLR end "apiVersion"



    # $ANTLR start "servingVersion"
    # interface.g:250:1: servingVersion returns [int version] : SERVING_VERSION '=' d= DEC_NUMBER ';' ;
    def servingVersion(self, ):
        version = None


        d = None

        try:
            try:
                # interface.g:251:5: ( SERVING_VERSION '=' d= DEC_NUMBER ';' )
                # interface.g:251:7: SERVING_VERSION '=' d= DEC_NUMBER ';'
                pass 
                self.match(self.input, SERVING_VERSION, self.FOLLOW_SERVING_VERSION_in_servingVersion587)

                self.match(self.input, 41, self.FOLLOW_41_in_servingVersion589)

                d = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_servingVersion593)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_servingVersion595)

                #action start
                version = int(d.text) 
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return version

    # $ANTLR end "servingVersion"



    # $ANTLR start "apiMsgSize"
    # interface.g:255:1: apiMsgSize returns [int msgSize] : API_MSG_SIZE '=' d= DEC_NUMBER ';' ;
    def apiMsgSize(self, ):
        msgSize = None


        d = None

        try:
            try:
                # interface.g:256:5: ( API_MSG_SIZE '=' d= DEC_NUMBER ';' )
                # interface.g:256:7: API_MSG_SIZE '=' d= DEC_NUMBER ';'
                pass 
                self.match(self.input, API_MSG_SIZE, self.FOLLOW_API_MSG_SIZE_in_apiMsgSize626)

                self.match(self.input, 41, self.FOLLOW_41_in_apiMsgSize628)

                d = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_apiMsgSize632)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_apiMsgSize634)

                #action start
                msgSize = int(d.text) 
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return msgSize

    # $ANTLR end "apiMsgSize"



    # $ANTLR start "direction"
    # interface.g:263:1: direction returns [direction] : ( IN | OUT );
    def direction(self, ):
        direction = None


        try:
            try:
                # interface.g:264:5: ( IN | OUT )
                alt1 = 2
                LA1_0 = self.input.LA(1)

                if (LA1_0 == IN) :
                    alt1 = 1
                elif (LA1_0 == OUT) :
                    alt1 = 2
                else:
                    nvae = NoViableAltException("", 1, 0, self.input)

                    raise nvae


                if alt1 == 1:
                    # interface.g:264:7: IN
                    pass 
                    self.match(self.input, IN, self.FOLLOW_IN_in_direction671)

                    #action start
                    direction = interfaceIR.DIR_IN 
                    #action end



                elif alt1 == 2:
                    # interface.g:265:7: OUT
                    pass 
                    self.match(self.input, OUT, self.FOLLOW_OUT_in_direction685)

                    #action start
                    direction = interfaceIR.DIR_OUT 
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return direction

    # $ANTLR end "direction"



    # $ANTLR start "number"
    # interface.g:269:1: number returns [value] : ( DEC_NUMBER | HEX_NUMBER );
    def number(self, ):
        value = None


        DEC_NUMBER1 = None
        HEX_NUMBER2 = None

        try:
            try:
                # interface.g:270:5: ( DEC_NUMBER | HEX_NUMBER )
                alt2 = 2
                LA2_0 = self.input.LA(1)

                if (LA2_0 == DEC_NUMBER) :
                    alt2 = 1
                elif (LA2_0 == HEX_NUMBER) :
                    alt2 = 2
                else:
                    nvae = NoViableAltException("", 2, 0, self.input)

                    raise nvae


                if alt2 == 1:
                    # interface.g:270:7: DEC_NUMBER
                    pass 
                    DEC_NUMBER1 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_number713)

                    #action start
                    value = int(DEC_NUMBER1.text, 10) 
                    #action end



                elif alt2 == 2:
                    # interface.g:271:7: HEX_NUMBER
                    pass 
                    HEX_NUMBER2 = self.match(self.input, HEX_NUMBER, self.FOLLOW_HEX_NUMBER_in_number726)

                    #action start
                    value = int(HEX_NUMBER2.text, 16) 
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "number"



    # $ANTLR start "integer"
    # interface.g:274:1: integer returns [value] : ( '+' DEC_NUMBER | '-' DEC_NUMBER | DEC_NUMBER | HEX_NUMBER );
    def integer(self, ):
        value = None


        DEC_NUMBER3 = None
        DEC_NUMBER4 = None
        DEC_NUMBER5 = None
        HEX_NUMBER6 = None

        try:
            try:
                # interface.g:275:5: ( '+' DEC_NUMBER | '-' DEC_NUMBER | DEC_NUMBER | HEX_NUMBER )
                alt3 = 4
                LA3 = self.input.LA(1)
                if LA3 == 36:
                    alt3 = 1
                elif LA3 == 38:
                    alt3 = 2
                elif LA3 == DEC_NUMBER:
                    alt3 = 3
                elif LA3 == HEX_NUMBER:
                    alt3 = 4
                else:
                    nvae = NoViableAltException("", 3, 0, self.input)

                    raise nvae


                if alt3 == 1:
                    # interface.g:275:7: '+' DEC_NUMBER
                    pass 
                    self.match(self.input, 36, self.FOLLOW_36_in_integer752)

                    DEC_NUMBER3 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer754)

                    #action start
                    value = int(DEC_NUMBER3.text, 10) 
                    #action end



                elif alt3 == 2:
                    # interface.g:276:7: '-' DEC_NUMBER
                    pass 
                    self.match(self.input, 38, self.FOLLOW_38_in_integer766)

                    DEC_NUMBER4 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer768)

                    #action start
                    value = -int(DEC_NUMBER4.text, 10) 
                    #action end



                elif alt3 == 3:
                    # interface.g:277:7: DEC_NUMBER
                    pass 
                    DEC_NUMBER5 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_integer780)

                    #action start
                    value = int(DEC_NUMBER5.text, 10) 
                    #action end



                elif alt3 == 4:
                    # interface.g:278:7: HEX_NUMBER
                    pass 
                    HEX_NUMBER6 = self.match(self.input, HEX_NUMBER, self.FOLLOW_HEX_NUMBER_in_integer796)

                    #action start
                    value = int(HEX_NUMBER6.text, 16) 
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "integer"


    class defineValue_return(ParserRuleReturnScope):
        def __init__(self):
            super(interfaceParser.defineValue_return, self).__init__()

            self.value = None





    # $ANTLR start "defineValue"
    # interface.g:281:1: defineValue returns [value] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def defineValue(self, ):
        retval = self.defineValue_return()
        retval.start = self.input.LT(1)


        IDENTIFIER7 = None
        SCOPED_IDENTIFIER8 = None

        try:
            try:
                # interface.g:282:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt4 = 2
                LA4_0 = self.input.LA(1)

                if (LA4_0 == IDENTIFIER) :
                    alt4 = 1
                elif (LA4_0 == SCOPED_IDENTIFIER) :
                    alt4 = 2
                else:
                    nvae = NoViableAltException("", 4, 0, self.input)

                    raise nvae


                if alt4 == 1:
                    # interface.g:282:7: IDENTIFIER
                    pass 
                    IDENTIFIER7 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_defineValue825)

                    #action start
                    retval.value = int(self.iface.findDefinition(IDENTIFIER7.text).value) 
                    #action end



                elif alt4 == 2:
                    # interface.g:283:7: SCOPED_IDENTIFIER
                    pass 
                    SCOPED_IDENTIFIER8 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_defineValue842)

                    #action start
                    retval.value = int(self.iface.findDefinition(SCOPED_IDENTIFIER8.text).value) 
                    #action end



                retval.stop = self.input.LT(-1)



            except KeyError as e:

                self.emitErrorMessage(self.getErrorHeaderForToken(retval.start) +
                                      " No definition for {}".format(self.input.toString(retval.start, self.input.LT(-1))))
                self.compileErrors += 1


            except TypeError as e:

                self.emitErrorMessage(self.getErrorHeaderForToken(retval.start) +
                                      " {} is not an integer definition".format(self.input.toString(retval.start, self.input.LT(-1))))
                self.compileErrors += 1



        finally:
            pass
        return retval

    # $ANTLR end "defineValue"


    class simpleNumber_return(ParserRuleReturnScope):
        def __init__(self):
            super(interfaceParser.simpleNumber_return, self).__init__()

            self.value = None





    # $ANTLR start "simpleNumber"
    # interface.g:299:1: simpleNumber returns [value] : ( number | defineValue );
    def simpleNumber(self, ):
        retval = self.simpleNumber_return()
        retval.start = self.input.LT(1)


        number9 = None
        defineValue10 = None

        try:
            try:
                # interface.g:300:5: ( number | defineValue )
                alt5 = 2
                LA5_0 = self.input.LA(1)

                if (LA5_0 == DEC_NUMBER or LA5_0 == HEX_NUMBER) :
                    alt5 = 1
                elif (LA5_0 == IDENTIFIER or LA5_0 == SCOPED_IDENTIFIER) :
                    alt5 = 2
                else:
                    nvae = NoViableAltException("", 5, 0, self.input)

                    raise nvae


                if alt5 == 1:
                    # interface.g:300:7: number
                    pass 
                    self._state.following.append(self.FOLLOW_number_in_simpleNumber879)
                    number9 = self.number()

                    self._state.following.pop()

                    #action start
                    retval.value = number9 
                    #action end



                elif alt5 == 2:
                    # interface.g:301:7: defineValue
                    pass 
                    self._state.following.append(self.FOLLOW_defineValue_in_simpleNumber900)
                    defineValue10 = self.defineValue()

                    self._state.following.pop()

                    #action start
                    retval.value = ((defineValue10 is not None) and [defineValue10.value] or [None])[0] 
                    #action end



                retval.stop = self.input.LT(-1)



            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return retval

    # $ANTLR end "simpleNumber"



    # $ANTLR start "simpleExpression"
    # interface.g:304:1: simpleExpression returns [value] : ( integer | defineValue | '(' sumExpression ')' );
    def simpleExpression(self, ):
        value = None


        integer11 = None
        defineValue12 = None
        sumExpression13 = None

        try:
            try:
                # interface.g:305:5: ( integer | defineValue | '(' sumExpression ')' )
                alt6 = 3
                LA6 = self.input.LA(1)
                if LA6 == DEC_NUMBER or LA6 == HEX_NUMBER or LA6 == 36 or LA6 == 38:
                    alt6 = 1
                elif LA6 == IDENTIFIER or LA6 == SCOPED_IDENTIFIER:
                    alt6 = 2
                elif LA6 == 33:
                    alt6 = 3
                else:
                    nvae = NoViableAltException("", 6, 0, self.input)

                    raise nvae


                if alt6 == 1:
                    # interface.g:305:7: integer
                    pass 
                    self._state.following.append(self.FOLLOW_integer_in_simpleExpression929)
                    integer11 = self.integer()

                    self._state.following.pop()

                    #action start
                    value = integer11 
                    #action end



                elif alt6 == 2:
                    # interface.g:306:7: defineValue
                    pass 
                    self._state.following.append(self.FOLLOW_defineValue_in_simpleExpression953)
                    defineValue12 = self.defineValue()

                    self._state.following.pop()

                    #action start
                    value = ((defineValue12 is not None) and [defineValue12.value] or [None])[0] 
                    #action end



                elif alt6 == 3:
                    # interface.g:307:7: '(' sumExpression ')'
                    pass 
                    self.match(self.input, 33, self.FOLLOW_33_in_simpleExpression973)

                    self._state.following.append(self.FOLLOW_sumExpression_in_simpleExpression975)
                    sumExpression13 = self.sumExpression()

                    self._state.following.pop()

                    self.match(self.input, 34, self.FOLLOW_34_in_simpleExpression977)

                    #action start
                    value = sumExpression13 
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "simpleExpression"



    # $ANTLR start "productExpression"
    # interface.g:310:1: productExpression returns [value] : initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )* ;
    def productExpression(self, ):
        value = None


        initialValue = None
        mulValue = None
        divValue = None

        try:
            try:
                # interface.g:311:5: (initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )* )
                # interface.g:311:7: initialValue= simpleExpression ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )*
                pass 
                self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression1002)
                initialValue = self.simpleExpression()

                self._state.following.pop()

                #action start
                value = initialValue 
                #action end


                # interface.g:312:9: ( '*' mulValue= simpleExpression | '/' divValue= simpleExpression )*
                while True: #loop7
                    alt7 = 3
                    LA7_0 = self.input.LA(1)

                    if (LA7_0 == 35) :
                        alt7 = 1
                    elif (LA7_0 == 40) :
                        alt7 = 2


                    if alt7 == 1:
                        # interface.g:312:11: '*' mulValue= simpleExpression
                        pass 
                        self.match(self.input, 35, self.FOLLOW_35_in_productExpression1020)

                        self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression1024)
                        mulValue = self.simpleExpression()

                        self._state.following.pop()

                        #action start
                        value *= mulValue 
                        #action end



                    elif alt7 == 2:
                        # interface.g:313:11: '/' divValue= simpleExpression
                        pass 
                        self.match(self.input, 40, self.FOLLOW_40_in_productExpression1038)

                        self._state.following.append(self.FOLLOW_simpleExpression_in_productExpression1042)
                        divValue = self.simpleExpression()

                        self._state.following.pop()

                        #action start
                        value /= divValue 
                        #action end



                    else:
                        break #loop7





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "productExpression"



    # $ANTLR start "sumExpression"
    # interface.g:317:1: sumExpression returns [value] : initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )* ;
    def sumExpression(self, ):
        value = None


        initialValue = None
        addValue = None
        subValue = None

        try:
            try:
                # interface.g:318:5: (initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )* )
                # interface.g:318:7: initialValue= productExpression ( '+' addValue= productExpression | '-' subValue= productExpression )*
                pass 
                self._state.following.append(self.FOLLOW_productExpression_in_sumExpression1078)
                initialValue = self.productExpression()

                self._state.following.pop()

                #action start
                value = initialValue 
                #action end


                # interface.g:319:9: ( '+' addValue= productExpression | '-' subValue= productExpression )*
                while True: #loop8
                    alt8 = 3
                    LA8_0 = self.input.LA(1)

                    if (LA8_0 == 36) :
                        alt8 = 1
                    elif (LA8_0 == 38) :
                        alt8 = 2


                    if alt8 == 1:
                        # interface.g:319:11: '+' addValue= productExpression
                        pass 
                        self.match(self.input, 36, self.FOLLOW_36_in_sumExpression1096)

                        self._state.following.append(self.FOLLOW_productExpression_in_sumExpression1100)
                        addValue = self.productExpression()

                        self._state.following.pop()

                        #action start
                        value += addValue 
                        #action end



                    elif alt8 == 2:
                        # interface.g:320:11: '-' subValue= productExpression
                        pass 
                        self.match(self.input, 38, self.FOLLOW_38_in_sumExpression1114)

                        self._state.following.append(self.FOLLOW_productExpression_in_sumExpression1118)
                        subValue = self.productExpression()

                        self._state.following.pop()

                        #action start
                        value -= subValue 
                        #action end



                    else:
                        break #loop8





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "sumExpression"



    # $ANTLR start "valueExpression"
    # interface.g:324:1: valueExpression returns [value] : ( sumExpression | QUOTED_STRING );
    def valueExpression(self, ):
        value = None


        QUOTED_STRING15 = None
        sumExpression14 = None

        try:
            try:
                # interface.g:325:5: ( sumExpression | QUOTED_STRING )
                alt9 = 2
                LA9_0 = self.input.LA(1)

                if (LA9_0 == DEC_NUMBER or (HEX_NUMBER <= LA9_0 <= IDENTIFIER) or LA9_0 == SCOPED_IDENTIFIER or LA9_0 == 33 or LA9_0 == 36 or LA9_0 == 38) :
                    alt9 = 1
                elif (LA9_0 == QUOTED_STRING) :
                    alt9 = 2
                else:
                    nvae = NoViableAltException("", 9, 0, self.input)

                    raise nvae


                if alt9 == 1:
                    # interface.g:325:7: sumExpression
                    pass 
                    self._state.following.append(self.FOLLOW_sumExpression_in_valueExpression1152)
                    sumExpression14 = self.sumExpression()

                    self._state.following.pop()

                    #action start
                    value = sumExpression14 
                    #action end



                elif alt9 == 2:
                    # interface.g:326:7: QUOTED_STRING
                    pass 
                    QUOTED_STRING15 = self.match(self.input, QUOTED_STRING, self.FOLLOW_QUOTED_STRING_in_valueExpression1165)

                    #action start
                    value = UnquoteString(QUOTED_STRING15.text) 
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return value

    # $ANTLR end "valueExpression"



    # $ANTLR start "arrayExpression"
    # interface.g:332:1: arrayExpression returns [size] : ( '[' simpleNumber ']' | '[' minSize= simpleNumber '..' maxSize= simpleNumber ']' );
    def arrayExpression(self, ):
        size = None


        minSize = None
        maxSize = None
        simpleNumber16 = None

        try:
            try:
                # interface.g:333:5: ( '[' simpleNumber ']' | '[' minSize= simpleNumber '..' maxSize= simpleNumber ']' )
                alt10 = 2
                LA10_0 = self.input.LA(1)

                if (LA10_0 == 42) :
                    LA10 = self.input.LA(2)
                    if LA10 == DEC_NUMBER:
                        LA10_2 = self.input.LA(3)

                        if (LA10_2 == 43) :
                            alt10 = 1
                        elif (LA10_2 == 39) :
                            alt10 = 2
                        else:
                            nvae = NoViableAltException("", 10, 2, self.input)

                            raise nvae


                    elif LA10 == HEX_NUMBER:
                        LA10_3 = self.input.LA(3)

                        if (LA10_3 == 43) :
                            alt10 = 1
                        elif (LA10_3 == 39) :
                            alt10 = 2
                        else:
                            nvae = NoViableAltException("", 10, 3, self.input)

                            raise nvae


                    elif LA10 == IDENTIFIER:
                        LA10_4 = self.input.LA(3)

                        if (LA10_4 == 43) :
                            alt10 = 1
                        elif (LA10_4 == 39) :
                            alt10 = 2
                        else:
                            nvae = NoViableAltException("", 10, 4, self.input)

                            raise nvae


                    elif LA10 == SCOPED_IDENTIFIER:
                        LA10_5 = self.input.LA(3)

                        if (LA10_5 == 43) :
                            alt10 = 1
                        elif (LA10_5 == 39) :
                            alt10 = 2
                        else:
                            nvae = NoViableAltException("", 10, 5, self.input)

                            raise nvae


                    else:
                        nvae = NoViableAltException("", 10, 1, self.input)

                        raise nvae


                else:
                    nvae = NoViableAltException("", 10, 0, self.input)

                    raise nvae


                if alt10 == 1:
                    # interface.g:333:7: '[' simpleNumber ']'
                    pass 
                    self.match(self.input, 42, self.FOLLOW_42_in_arrayExpression1194)

                    self._state.following.append(self.FOLLOW_simpleNumber_in_arrayExpression1196)
                    simpleNumber16 = self.simpleNumber()

                    self._state.following.pop()

                    self.match(self.input, 43, self.FOLLOW_43_in_arrayExpression1198)

                    #action start
                    size = ((simpleNumber16 is not None) and [simpleNumber16.value] or [None])[0] 
                    #action end



                elif alt10 == 2:
                    # interface.g:335:7: '[' minSize= simpleNumber '..' maxSize= simpleNumber ']'
                    pass 
                    self.match(self.input, 42, self.FOLLOW_42_in_arrayExpression1216)

                    self._state.following.append(self.FOLLOW_simpleNumber_in_arrayExpression1220)
                    minSize = self.simpleNumber()

                    self._state.following.pop()

                    self.match(self.input, 39, self.FOLLOW_39_in_arrayExpression1222)

                    self._state.following.append(self.FOLLOW_simpleNumber_in_arrayExpression1226)
                    maxSize = self.simpleNumber()

                    self._state.following.pop()

                    self.match(self.input, 43, self.FOLLOW_43_in_arrayExpression1228)

                    #action start
                            
                    self.emitErrorMessage(self.getWarningHeaderForToken(minSize.start) +
                                          " Minimum lengths are deprecated and will be ignored.")
                    size = ((maxSize is not None) and [maxSize.value] or [None])[0]
                            
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return size

    # $ANTLR end "arrayExpression"



    # $ANTLR start "typeIdentifier"
    # interface.g:343:1: typeIdentifier returns [typeObj] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def typeIdentifier(self, ):
        typeObj = None


        IDENTIFIER17 = None
        SCOPED_IDENTIFIER18 = None

        try:
            try:
                # interface.g:344:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt11 = 2
                LA11_0 = self.input.LA(1)

                if (LA11_0 == IDENTIFIER) :
                    alt11 = 1
                elif (LA11_0 == SCOPED_IDENTIFIER) :
                    alt11 = 2
                else:
                    nvae = NoViableAltException("", 11, 0, self.input)

                    raise nvae


                if alt11 == 1:
                    # interface.g:344:7: IDENTIFIER
                    pass 
                    IDENTIFIER17 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_typeIdentifier1259)

                    #action start
                            
                    try:
                        typeObj = self.iface.findType(IDENTIFIER17.text)
                    except KeyError as e:
                        self.compileErrors += 1
                        self.emitErrorMessage(self.getErrorHeaderForToken(IDENTIFIER17) +
                                              " Unknown type {}".format(e))
                        typeObj = interfaceIR.ERROR_TYPE
                            
                    #action end



                elif alt11 == 2:
                    # interface.g:354:7: SCOPED_IDENTIFIER
                    pass 
                    SCOPED_IDENTIFIER18 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_typeIdentifier1277)

                    #action start
                            
                    try:
                        typeObj = self.iface.findType(SCOPED_IDENTIFIER18.text)
                    except KeyError as e:
                        self.compileErrors += 1
                        self.emitErrorMessage(self.getErrorHeaderForToken(SCOPED_IDENTIFIER18) +
                                              " Unknown type {}".format(e))
                        typeObj = interfaceIR.ERROR_TYPE
                            
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return typeObj

    # $ANTLR end "typeIdentifier"



    # $ANTLR start "docPostComments"
    # interface.g:366:1: docPostComments returns [comments] : ( DOC_POST_COMMENT )* ;
    def docPostComments(self, ):
        comments = None


        DOC_POST_COMMENT19 = None

        try:
            try:
                # interface.g:367:5: ( ( DOC_POST_COMMENT )* )
                # interface.g:367:7: ( DOC_POST_COMMENT )*
                pass 
                #action start
                comments = [] 
                #action end


                # interface.g:368:7: ( DOC_POST_COMMENT )*
                while True: #loop12
                    alt12 = 2
                    LA12_0 = self.input.LA(1)

                    if (LA12_0 == DOC_POST_COMMENT) :
                        alt12 = 1


                    if alt12 == 1:
                        # interface.g:368:9: DOC_POST_COMMENT
                        pass 
                        DOC_POST_COMMENT19 = self.match(self.input, DOC_POST_COMMENT, self.FOLLOW_DOC_POST_COMMENT_in_docPostComments1318)

                        #action start
                        comments.append(StripPostComment(DOC_POST_COMMENT19.text)) 
                        #action end



                    else:
                        break #loop12





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return comments

    # $ANTLR end "docPostComments"



    # $ANTLR start "docPreComment"
    # interface.g:373:1: docPreComment returns [comment] : DOC_PRE_COMMENT ;
    def docPreComment(self, ):
        comment = None


        DOC_PRE_COMMENT20 = None

        try:
            try:
                # interface.g:374:5: ( DOC_PRE_COMMENT )
                # interface.g:374:7: DOC_PRE_COMMENT
                pass 
                DOC_PRE_COMMENT20 = self.match(self.input, DOC_PRE_COMMENT, self.FOLLOW_DOC_PRE_COMMENT_in_docPreComment1362)

                #action start
                comment = StripPreComment(DOC_PRE_COMMENT20.text) 
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return comment

    # $ANTLR end "docPreComment"



    # $ANTLR start "defineDecl"
    # interface.g:377:1: defineDecl returns [define] : DEFINE IDENTIFIER '=' valueExpression ';' ;
    def defineDecl(self, ):
        define = None


        IDENTIFIER21 = None
        valueExpression22 = None

        try:
            try:
                # interface.g:378:5: ( DEFINE IDENTIFIER '=' valueExpression ';' )
                # interface.g:378:7: DEFINE IDENTIFIER '=' valueExpression ';'
                pass 
                self.match(self.input, DEFINE, self.FOLLOW_DEFINE_in_defineDecl1385)

                IDENTIFIER21 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_defineDecl1387)

                self.match(self.input, 41, self.FOLLOW_41_in_defineDecl1389)

                self._state.following.append(self.FOLLOW_valueExpression_in_defineDecl1391)
                valueExpression22 = self.valueExpression()

                self._state.following.pop()

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_defineDecl1393)

                #action start
                        
                define = interfaceIR.Definition(IDENTIFIER21.text,
                                                self.getLocationTuple(IDENTIFIER21),
                                                valueExpression22)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return define

    # $ANTLR end "defineDecl"



    # $ANTLR start "namedValue"
    # interface.g:386:1: namedValue returns [namedValue] : IDENTIFIER ( '=' integer )? docPostComments ;
    def namedValue(self, ):
        namedValue = None


        IDENTIFIER23 = None
        integer24 = None
        docPostComments25 = None

        try:
            try:
                # interface.g:387:5: ( IDENTIFIER ( '=' integer )? docPostComments )
                # interface.g:387:7: IDENTIFIER ( '=' integer )? docPostComments
                pass 
                IDENTIFIER23 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_namedValue1424)

                # interface.g:387:18: ( '=' integer )?
                alt13 = 2
                LA13_0 = self.input.LA(1)

                if (LA13_0 == 41) :
                    alt13 = 1
                if alt13 == 1:
                    # interface.g:387:20: '=' integer
                    pass 
                    self.match(self.input, 41, self.FOLLOW_41_in_namedValue1428)

                    self._state.following.append(self.FOLLOW_integer_in_namedValue1430)
                    integer24 = self.integer()

                    self._state.following.pop()




                self._state.following.append(self.FOLLOW_docPostComments_in_namedValue1435)
                docPostComments25 = self.docPostComments()

                self._state.following.pop()

                #action start
                        
                namedValue = interfaceIR.NamedValue(IDENTIFIER23.text,
                                                    self.getLocationTuple(IDENTIFIER23),
                                                    integer24)
                namedValue.comments = docPostComments25
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return namedValue

    # $ANTLR end "namedValue"



    # $ANTLR start "namedValueList"
    # interface.g:396:1: namedValueList returns [values] : (| namedValue | namedValue ',' docPostComments rest= namedValueList );
    def namedValueList(self, ):
        values = None


        rest = None
        namedValue26 = None
        namedValue27 = None
        docPostComments28 = None

        try:
            try:
                # interface.g:397:5: (| namedValue | namedValue ',' docPostComments rest= namedValueList )
                alt14 = 3
                alt14 = self.dfa14.predict(self.input)
                if alt14 == 1:
                    # interface.g:397:51: 
                    pass 
                    #action start
                    values = [ ] 
                    #action end



                elif alt14 == 2:
                    # interface.g:398:7: namedValue
                    pass 
                    self._state.following.append(self.FOLLOW_namedValue_in_namedValueList1510)
                    namedValue26 = self.namedValue()

                    self._state.following.pop()

                    #action start
                    values = [ namedValue26 ] 
                    #action end



                elif alt14 == 3:
                    # interface.g:399:7: namedValue ',' docPostComments rest= namedValueList
                    pass 
                    self._state.following.append(self.FOLLOW_namedValue_in_namedValueList1553)
                    namedValue27 = self.namedValue()

                    self._state.following.pop()

                    self.match(self.input, 37, self.FOLLOW_37_in_namedValueList1555)

                    self._state.following.append(self.FOLLOW_docPostComments_in_namedValueList1557)
                    docPostComments28 = self.docPostComments()

                    self._state.following.pop()

                    self._state.following.append(self.FOLLOW_namedValueList_in_namedValueList1561)
                    rest = self.namedValueList()

                    self._state.following.pop()

                    #action start
                            
                    namedValue27.comments.extend(docPostComments28)
                    values = [ namedValue27 ]
                    values.extend(rest)
                            
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return values

    # $ANTLR end "namedValueList"



    # $ANTLR start "enumDecl"
    # interface.g:407:1: enumDecl returns [enum] : ENUM IDENTIFIER '{' namedValueList '}' ';' ;
    def enumDecl(self, ):
        enum = None


        IDENTIFIER29 = None
        namedValueList30 = None

        try:
            try:
                # interface.g:408:5: ( ENUM IDENTIFIER '{' namedValueList '}' ';' )
                # interface.g:408:7: ENUM IDENTIFIER '{' namedValueList '}' ';'
                pass 
                self.match(self.input, ENUM, self.FOLLOW_ENUM_in_enumDecl1592)

                IDENTIFIER29 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_enumDecl1594)

                self.match(self.input, 44, self.FOLLOW_44_in_enumDecl1596)

                self._state.following.append(self.FOLLOW_namedValueList_in_enumDecl1598)
                namedValueList30 = self.namedValueList()

                self._state.following.pop()

                self.match(self.input, 45, self.FOLLOW_45_in_enumDecl1600)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_enumDecl1602)

                #action start
                        
                enum = interfaceIR.EnumType(IDENTIFIER29.text,
                                            self.getLocationTuple(IDENTIFIER29),
                                            namedValueList30)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return enum

    # $ANTLR end "enumDecl"



    # $ANTLR start "bitmaskDecl"
    # interface.g:416:1: bitmaskDecl returns [bitmask] : BITMASK IDENTIFIER '{' namedValueList '}' ';' ;
    def bitmaskDecl(self, ):
        bitmask = None


        IDENTIFIER31 = None
        namedValueList32 = None

        try:
            try:
                # interface.g:417:5: ( BITMASK IDENTIFIER '{' namedValueList '}' ';' )
                # interface.g:417:7: BITMASK IDENTIFIER '{' namedValueList '}' ';'
                pass 
                self.match(self.input, BITMASK, self.FOLLOW_BITMASK_in_bitmaskDecl1633)

                IDENTIFIER31 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_bitmaskDecl1635)

                self.match(self.input, 44, self.FOLLOW_44_in_bitmaskDecl1637)

                self._state.following.append(self.FOLLOW_namedValueList_in_bitmaskDecl1639)
                namedValueList32 = self.namedValueList()

                self._state.following.pop()

                self.match(self.input, 45, self.FOLLOW_45_in_bitmaskDecl1641)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_bitmaskDecl1643)

                #action start
                        
                bitmask = interfaceIR.BitmaskType(IDENTIFIER31.text,
                                                    self.getLocationTuple(IDENTIFIER31),
                                                    namedValueList32)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return bitmask

    # $ANTLR end "bitmaskDecl"



    # $ANTLR start "referenceDecl"
    # interface.g:425:1: referenceDecl returns [ref] : REFERENCE IDENTIFIER ';' ;
    def referenceDecl(self, ):
        ref = None


        IDENTIFIER33 = None

        try:
            try:
                # interface.g:426:5: ( REFERENCE IDENTIFIER ';' )
                # interface.g:426:7: REFERENCE IDENTIFIER ';'
                pass 
                self.match(self.input, REFERENCE, self.FOLLOW_REFERENCE_in_referenceDecl1674)

                IDENTIFIER33 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_referenceDecl1676)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_referenceDecl1678)

                #action start
                        
                ref = interfaceIR.ReferenceType(IDENTIFIER33.text,
                                                self.getLocationTuple(IDENTIFIER33))
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return ref

    # $ANTLR end "referenceDecl"



    # $ANTLR start "compoundMember"
    # interface.g:433:1: compoundMember returns [member] : typeIdentifier IDENTIFIER ( arrayExpression )? ';' docPostComments ;
    def compoundMember(self, ):
        member = None


        IDENTIFIER35 = None
        typeIdentifier34 = None
        arrayExpression36 = None

        try:
            try:
                # interface.g:434:5: ( typeIdentifier IDENTIFIER ( arrayExpression )? ';' docPostComments )
                # interface.g:434:7: typeIdentifier IDENTIFIER ( arrayExpression )? ';' docPostComments
                pass 
                self._state.following.append(self.FOLLOW_typeIdentifier_in_compoundMember1709)
                typeIdentifier34 = self.typeIdentifier()

                self._state.following.pop()

                IDENTIFIER35 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_compoundMember1711)

                # interface.g:434:33: ( arrayExpression )?
                alt15 = 2
                LA15_0 = self.input.LA(1)

                if (LA15_0 == 42) :
                    alt15 = 1
                if alt15 == 1:
                    # interface.g:434:33: arrayExpression
                    pass 
                    self._state.following.append(self.FOLLOW_arrayExpression_in_compoundMember1713)
                    arrayExpression36 = self.arrayExpression()

                    self._state.following.pop()




                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_compoundMember1716)

                self._state.following.append(self.FOLLOW_docPostComments_in_compoundMember1718)
                self.docPostComments()

                self._state.following.pop()

                #action start
                        
                if typeIdentifier34 == interfaceIR.OLD_HANDLER_TYPE or \
                   isinstance(typeIdentifier34, interfaceIR.HandlerReferenceType) or \
                   isinstance(typeIdentifier34, interfaceIR.HandlerType):
                    self.compileErrors += 1
                    self.emitErrorMessage(self.getErrorHeaderForToken(typeIdentifier34) +
                                          " Cannot include handlers in a structure")
                    member = None
                else:
                    member = interfaceIR.MakeStructMember(self.iface,
                                                           typeIdentifier34,
                                                           IDENTIFIER35.text,
                                                           self.getLocationTuple(IDENTIFIER35),
                                                           arrayExpression36)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return member

    # $ANTLR end "compoundMember"



    # $ANTLR start "compoundMemberList"
    # interface.g:452:1: compoundMemberList returns [members] : ( compoundMember | compoundMember rest= compoundMemberList );
    def compoundMemberList(self, ):
        members = None


        rest = None
        compoundMember37 = None
        compoundMember38 = None

        try:
            try:
                # interface.g:453:5: ( compoundMember | compoundMember rest= compoundMemberList )
                alt16 = 2
                alt16 = self.dfa16.predict(self.input)
                if alt16 == 1:
                    # interface.g:453:7: compoundMember
                    pass 
                    self._state.following.append(self.FOLLOW_compoundMember_in_compoundMemberList1749)
                    compoundMember37 = self.compoundMember()

                    self._state.following.pop()

                    #action start
                    members = [ compoundMember37 ] 
                    #action end



                elif alt16 == 2:
                    # interface.g:454:7: compoundMember rest= compoundMemberList
                    pass 
                    self._state.following.append(self.FOLLOW_compoundMember_in_compoundMemberList1778)
                    compoundMember38 = self.compoundMember()

                    self._state.following.pop()

                    self._state.following.append(self.FOLLOW_compoundMemberList_in_compoundMemberList1782)
                    rest = self.compoundMemberList()

                    self._state.following.pop()

                    #action start
                            
                    members = [ compoundMember38 ]
                    members.extend(rest)
                            
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return members

    # $ANTLR end "compoundMemberList"



    # $ANTLR start "structDecl"
    # interface.g:461:1: structDecl returns [struct] : STRUCT IDENTIFIER '{' ( compoundMemberList )? '}' ';' ;
    def structDecl(self, ):
        struct = None


        IDENTIFIER39 = None
        compoundMemberList40 = None

        try:
            try:
                # interface.g:462:5: ( STRUCT IDENTIFIER '{' ( compoundMemberList )? '}' ';' )
                # interface.g:462:7: STRUCT IDENTIFIER '{' ( compoundMemberList )? '}' ';'
                pass 
                self.match(self.input, STRUCT, self.FOLLOW_STRUCT_in_structDecl1813)

                IDENTIFIER39 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_structDecl1815)

                self.match(self.input, 44, self.FOLLOW_44_in_structDecl1817)

                # interface.g:462:29: ( compoundMemberList )?
                alt17 = 2
                LA17_0 = self.input.LA(1)

                if (LA17_0 == IDENTIFIER or LA17_0 == SCOPED_IDENTIFIER) :
                    alt17 = 1
                if alt17 == 1:
                    # interface.g:462:29: compoundMemberList
                    pass 
                    self._state.following.append(self.FOLLOW_compoundMemberList_in_structDecl1819)
                    compoundMemberList40 = self.compoundMemberList()

                    self._state.following.pop()




                self.match(self.input, 45, self.FOLLOW_45_in_structDecl1822)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_structDecl1824)

                #action start
                        
                struct = interfaceIR.StructType(IDENTIFIER39.text,
                                                self.getLocationTuple(IDENTIFIER39),
                                                compoundMemberList40)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return struct

    # $ANTLR end "structDecl"



    # $ANTLR start "formalParameter"
    # interface.g:470:1: formalParameter returns [parameter] : typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments ;
    def formalParameter(self, ):
        parameter = None


        IDENTIFIER42 = None
        typeIdentifier41 = None
        arrayExpression43 = None
        direction44 = None
        docPostComments45 = None

        try:
            try:
                # interface.g:471:5: ( typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments )
                # interface.g:471:7: typeIdentifier IDENTIFIER ( arrayExpression )? ( direction )? docPostComments
                pass 
                self._state.following.append(self.FOLLOW_typeIdentifier_in_formalParameter1855)
                typeIdentifier41 = self.typeIdentifier()

                self._state.following.pop()

                IDENTIFIER42 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_formalParameter1857)

                # interface.g:471:33: ( arrayExpression )?
                alt18 = 2
                LA18_0 = self.input.LA(1)

                if (LA18_0 == 42) :
                    alt18 = 1
                if alt18 == 1:
                    # interface.g:471:33: arrayExpression
                    pass 
                    self._state.following.append(self.FOLLOW_arrayExpression_in_formalParameter1859)
                    arrayExpression43 = self.arrayExpression()

                    self._state.following.pop()




                # interface.g:471:50: ( direction )?
                alt19 = 2
                LA19_0 = self.input.LA(1)

                if (LA19_0 == IN or LA19_0 == OUT) :
                    alt19 = 1
                if alt19 == 1:
                    # interface.g:471:50: direction
                    pass 
                    self._state.following.append(self.FOLLOW_direction_in_formalParameter1862)
                    direction44 = self.direction()

                    self._state.following.pop()




                self._state.following.append(self.FOLLOW_docPostComments_in_formalParameter1865)
                docPostComments45 = self.docPostComments()

                self._state.following.pop()

                #action start
                        
                if typeIdentifier41 == interfaceIR.OLD_HANDLER_TYPE:
                    self.emitErrorMessage(self.getWarningHeaderForToken(IDENTIFIER42) +
                                          " Parameters with type 'handler' are deprecated. "
                                          " Use the name of the handler instead: e.g. {} handler"
                                          .format(IDENTIFIER42.text))

                parameter = interfaceIR.MakeParameter(self.iface,
                                                       typeIdentifier41,
                                                       IDENTIFIER42.text,
                                                       self.getLocationTuple(IDENTIFIER42),
                                                       arrayExpression43,
                                                       direction44)
                parameter.comments = docPostComments45
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return parameter

    # $ANTLR end "formalParameter"



    # $ANTLR start "formalParameterList"
    # interface.g:489:1: formalParameterList returns [parameters] : ( formalParameter | formalParameter ',' docPostComments rest= formalParameterList );
    def formalParameterList(self, ):
        parameters = None


        rest = None
        formalParameter46 = None
        formalParameter47 = None
        docPostComments48 = None

        try:
            try:
                # interface.g:490:5: ( formalParameter | formalParameter ',' docPostComments rest= formalParameterList )
                alt20 = 2
                alt20 = self.dfa20.predict(self.input)
                if alt20 == 1:
                    # interface.g:490:7: formalParameter
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameter_in_formalParameterList1896)
                    formalParameter46 = self.formalParameter()

                    self._state.following.pop()

                    #action start
                            
                    parameters = [ formalParameter46 ]
                            
                    #action end



                elif alt20 == 2:
                    # interface.g:494:7: formalParameter ',' docPostComments rest= formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameter_in_formalParameterList1914)
                    formalParameter47 = self.formalParameter()

                    self._state.following.pop()

                    self.match(self.input, 37, self.FOLLOW_37_in_formalParameterList1916)

                    self._state.following.append(self.FOLLOW_docPostComments_in_formalParameterList1918)
                    docPostComments48 = self.docPostComments()

                    self._state.following.pop()

                    self._state.following.append(self.FOLLOW_formalParameterList_in_formalParameterList1922)
                    rest = self.formalParameterList()

                    self._state.following.pop()

                    #action start
                            
                    formalParameter47.comments.extend(docPostComments48)
                    parameters = [ formalParameter47 ]
                    parameters.extend(rest)
                            
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return parameters

    # $ANTLR end "formalParameterList"



    # $ANTLR start "functionDecl"
    # interface.g:502:1: functionDecl returns [function] : FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' '=' d= DEC_NUMBER ';' ;
    def functionDecl(self, ):
        function = None


        d = None
        IDENTIFIER51 = None
        formalParameterList49 = None
        typeIdentifier50 = None

        try:
            try:
                # interface.g:503:5: ( FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' '=' d= DEC_NUMBER ';' )
                # interface.g:503:7: FUNCTION ( typeIdentifier )? IDENTIFIER '(' ( formalParameterList )? ')' '=' d= DEC_NUMBER ';'
                pass 
                self.match(self.input, FUNCTION, self.FOLLOW_FUNCTION_in_functionDecl1953)

                # interface.g:503:16: ( typeIdentifier )?
                alt21 = 2
                LA21_0 = self.input.LA(1)

                if (LA21_0 == IDENTIFIER) :
                    LA21_1 = self.input.LA(2)

                    if (LA21_1 == IDENTIFIER) :
                        alt21 = 1
                elif (LA21_0 == SCOPED_IDENTIFIER) :
                    alt21 = 1
                if alt21 == 1:
                    # interface.g:503:16: typeIdentifier
                    pass 
                    self._state.following.append(self.FOLLOW_typeIdentifier_in_functionDecl1955)
                    typeIdentifier50 = self.typeIdentifier()

                    self._state.following.pop()




                IDENTIFIER51 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_functionDecl1958)

                self.match(self.input, 33, self.FOLLOW_33_in_functionDecl1960)

                # interface.g:503:47: ( formalParameterList )?
                alt22 = 2
                LA22_0 = self.input.LA(1)

                if (LA22_0 == IDENTIFIER or LA22_0 == SCOPED_IDENTIFIER) :
                    alt22 = 1
                if alt22 == 1:
                    # interface.g:503:47: formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameterList_in_functionDecl1962)
                    formalParameterList49 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 34, self.FOLLOW_34_in_functionDecl1965)

                self.match(self.input, 41, self.FOLLOW_41_in_functionDecl1967)

                d = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_functionDecl1971)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_functionDecl1974)

                #action start
                        
                if formalParameterList49 == None:
                    parameterList = []
                else:
                    parameterList = formalParameterList49
                msgId = int(d.text)
                function = interfaceIR.Function(typeIdentifier50,
                                                  IDENTIFIER51.text,
                                                  self.getLocationTuple(IDENTIFIER51),
                                                  parameterList,
                 msgId
                 )
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return function

    # $ANTLR end "functionDecl"



    # $ANTLR start "handlerDecl"
    # interface.g:519:1: handlerDecl returns [handler] : HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';' ;
    def handlerDecl(self, ):
        handler = None


        IDENTIFIER53 = None
        formalParameterList52 = None

        try:
            try:
                # interface.g:520:5: ( HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';' )
                # interface.g:520:7: HANDLER IDENTIFIER '(' ( formalParameterList )? ')' ';'
                pass 
                self.match(self.input, HANDLER, self.FOLLOW_HANDLER_in_handlerDecl2005)

                IDENTIFIER53 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_handlerDecl2007)

                self.match(self.input, 33, self.FOLLOW_33_in_handlerDecl2009)

                # interface.g:520:30: ( formalParameterList )?
                alt23 = 2
                LA23_0 = self.input.LA(1)

                if (LA23_0 == IDENTIFIER or LA23_0 == SCOPED_IDENTIFIER) :
                    alt23 = 1
                if alt23 == 1:
                    # interface.g:520:30: formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameterList_in_handlerDecl2011)
                    formalParameterList52 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 34, self.FOLLOW_34_in_handlerDecl2014)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_handlerDecl2016)

                #action start
                        
                if formalParameterList52 == None:
                    parameterList = []
                else:
                    parameterList = formalParameterList52
                handler = interfaceIR.HandlerType(IDENTIFIER53.text,
                                                    self.getLocationTuple(IDENTIFIER53),
                                                    parameterList)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return handler

    # $ANTLR end "handlerDecl"



    # $ANTLR start "eventDecl"
    # interface.g:532:1: eventDecl returns [event] : EVENT IDENTIFIER '(' ( formalParameterList )? ')' '=' '(' d1= DEC_NUMBER ',' d2= DEC_NUMBER ')' ';' ;
    def eventDecl(self, ):
        event = None


        d1 = None
        d2 = None
        IDENTIFIER55 = None
        formalParameterList54 = None

        try:
            try:
                # interface.g:533:5: ( EVENT IDENTIFIER '(' ( formalParameterList )? ')' '=' '(' d1= DEC_NUMBER ',' d2= DEC_NUMBER ')' ';' )
                # interface.g:533:7: EVENT IDENTIFIER '(' ( formalParameterList )? ')' '=' '(' d1= DEC_NUMBER ',' d2= DEC_NUMBER ')' ';'
                pass 
                self.match(self.input, EVENT, self.FOLLOW_EVENT_in_eventDecl2047)

                IDENTIFIER55 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_eventDecl2049)

                self.match(self.input, 33, self.FOLLOW_33_in_eventDecl2051)

                # interface.g:533:28: ( formalParameterList )?
                alt24 = 2
                LA24_0 = self.input.LA(1)

                if (LA24_0 == IDENTIFIER or LA24_0 == SCOPED_IDENTIFIER) :
                    alt24 = 1
                if alt24 == 1:
                    # interface.g:533:28: formalParameterList
                    pass 
                    self._state.following.append(self.FOLLOW_formalParameterList_in_eventDecl2053)
                    formalParameterList54 = self.formalParameterList()

                    self._state.following.pop()




                self.match(self.input, 34, self.FOLLOW_34_in_eventDecl2056)

                self.match(self.input, 41, self.FOLLOW_41_in_eventDecl2058)

                self.match(self.input, 33, self.FOLLOW_33_in_eventDecl2060)

                d1 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_eventDecl2064)

                self.match(self.input, 37, self.FOLLOW_37_in_eventDecl2066)

                d2 = self.match(self.input, DEC_NUMBER, self.FOLLOW_DEC_NUMBER_in_eventDecl2070)

                self.match(self.input, 34, self.FOLLOW_34_in_eventDecl2072)

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_eventDecl2074)

                #action start
                        
                if formalParameterList54 == None:
                    parameterList = []
                else:
                    parameterList = formalParameterList54
                addMsgId = int(d1.text)
                remMsgId = int(d2.text)
                event = interfaceIR.Event(IDENTIFIER55.text,
                                            self.getLocationTuple(IDENTIFIER55),
                                            parameterList,
                                            addMsgId,
                                            remMsgId)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return event

    # $ANTLR end "eventDecl"



    # $ANTLR start "declaration"
    # interface.g:552:1: declaration returns [declaration] : ( enumDecl | bitmaskDecl | referenceDecl | structDecl | functionDecl | handlerDecl | eventDecl | defineDecl );
    def declaration(self, ):
        declaration = None


        enumDecl56 = None
        bitmaskDecl57 = None
        referenceDecl58 = None
        structDecl59 = None
        functionDecl60 = None
        handlerDecl61 = None
        eventDecl62 = None
        defineDecl63 = None

        try:
            try:
                # interface.g:553:5: ( enumDecl | bitmaskDecl | referenceDecl | structDecl | functionDecl | handlerDecl | eventDecl | defineDecl )
                alt25 = 8
                LA25 = self.input.LA(1)
                if LA25 == ENUM:
                    alt25 = 1
                elif LA25 == BITMASK:
                    alt25 = 2
                elif LA25 == REFERENCE:
                    alt25 = 3
                elif LA25 == STRUCT:
                    alt25 = 4
                elif LA25 == FUNCTION:
                    alt25 = 5
                elif LA25 == HANDLER:
                    alt25 = 6
                elif LA25 == EVENT:
                    alt25 = 7
                elif LA25 == DEFINE:
                    alt25 = 8
                else:
                    nvae = NoViableAltException("", 25, 0, self.input)

                    raise nvae


                if alt25 == 1:
                    # interface.g:553:7: enumDecl
                    pass 
                    self._state.following.append(self.FOLLOW_enumDecl_in_declaration2108)
                    enumDecl56 = self.enumDecl()

                    self._state.following.pop()

                    #action start
                    declaration = enumDecl56 
                    #action end



                elif alt25 == 2:
                    # interface.g:554:7: bitmaskDecl
                    pass 
                    self._state.following.append(self.FOLLOW_bitmaskDecl_in_declaration2125)
                    bitmaskDecl57 = self.bitmaskDecl()

                    self._state.following.pop()

                    #action start
                    declaration = bitmaskDecl57 
                    #action end



                elif alt25 == 3:
                    # interface.g:555:7: referenceDecl
                    pass 
                    self._state.following.append(self.FOLLOW_referenceDecl_in_declaration2139)
                    referenceDecl58 = self.referenceDecl()

                    self._state.following.pop()

                    #action start
                    declaration = referenceDecl58 
                    #action end



                elif alt25 == 4:
                    # interface.g:556:7: structDecl
                    pass 
                    self._state.following.append(self.FOLLOW_structDecl_in_declaration2151)
                    structDecl59 = self.structDecl()

                    self._state.following.pop()

                    #action start
                    declaration = structDecl59 
                    #action end



                elif alt25 == 5:
                    # interface.g:557:7: functionDecl
                    pass 
                    self._state.following.append(self.FOLLOW_functionDecl_in_declaration2166)
                    functionDecl60 = self.functionDecl()

                    self._state.following.pop()

                    #action start
                    declaration = functionDecl60 
                    #action end



                elif alt25 == 6:
                    # interface.g:558:7: handlerDecl
                    pass 
                    self._state.following.append(self.FOLLOW_handlerDecl_in_declaration2179)
                    handlerDecl61 = self.handlerDecl()

                    self._state.following.pop()

                    #action start
                    declaration = handlerDecl61 
                    #action end



                elif alt25 == 7:
                    # interface.g:559:7: eventDecl
                    pass 
                    self._state.following.append(self.FOLLOW_eventDecl_in_declaration2193)
                    eventDecl62 = self.eventDecl()

                    self._state.following.pop()

                    #action start
                    declaration = eventDecl62 
                    #action end



                elif alt25 == 8:
                    # interface.g:560:7: defineDecl
                    pass 
                    self._state.following.append(self.FOLLOW_defineDecl_in_declaration2209)
                    defineDecl63 = self.defineDecl()

                    self._state.following.pop()

                    #action start
                    declaration = defineDecl63 
                    #action end




            except RecognitionException as re:

                # Report recognition exceptions here with default handling.  Do not want them to get caught
                # by generic handler below
                self.reportError(re)
                self.recover(self.input, re)


            except Exception as e:

                self.emitErrorMessage(self.getErrorHeaderForToken(self.getCurrentInputSymbol(self.input))
                                      + " " + str(e))
                self.compileErrors += 1



        finally:
            pass
        return declaration

    # $ANTLR end "declaration"



    # $ANTLR start "documentedDeclaration"
    # interface.g:576:1: documentedDeclaration returns [declaration] : docPreComment declaration ;
    def documentedDeclaration(self, ):
        declaration = None


        declaration64 = None
        docPreComment65 = None

        try:
            try:
                # interface.g:577:5: ( docPreComment declaration )
                # interface.g:577:7: docPreComment declaration
                pass 
                self._state.following.append(self.FOLLOW_docPreComment_in_documentedDeclaration2249)
                docPreComment65 = self.docPreComment()

                self._state.following.pop()

                self._state.following.append(self.FOLLOW_declaration_in_documentedDeclaration2251)
                declaration64 = self.declaration()

                self._state.following.pop()

                #action start
                        
                declaration = declaration64
                if declaration:
                    declaration.comment = docPreComment65
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return declaration

    # $ANTLR end "documentedDeclaration"



    # $ANTLR start "filename"
    # interface.g:587:1: filename returns [filename] : ( IDENTIFIER | SCOPED_IDENTIFIER );
    def filename(self, ):
        filename = None


        IDENTIFIER66 = None
        SCOPED_IDENTIFIER67 = None

        try:
            try:
                # interface.g:588:5: ( IDENTIFIER | SCOPED_IDENTIFIER )
                alt26 = 2
                LA26_0 = self.input.LA(1)

                if (LA26_0 == IDENTIFIER) :
                    alt26 = 1
                elif (LA26_0 == SCOPED_IDENTIFIER) :
                    alt26 = 2
                else:
                    nvae = NoViableAltException("", 26, 0, self.input)

                    raise nvae


                if alt26 == 1:
                    # interface.g:588:7: IDENTIFIER
                    pass 
                    IDENTIFIER66 = self.match(self.input, IDENTIFIER, self.FOLLOW_IDENTIFIER_in_filename2284)

                    #action start
                    filename = IDENTIFIER66.text 
                    #action end



                elif alt26 == 2:
                    # interface.g:589:7: SCOPED_IDENTIFIER
                    pass 
                    SCOPED_IDENTIFIER67 = self.match(self.input, SCOPED_IDENTIFIER, self.FOLLOW_SCOPED_IDENTIFIER_in_filename2302)

                    #action start
                    filename = SCOPED_IDENTIFIER67.text 
                    #action end




            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return filename

    # $ANTLR end "filename"



    # $ANTLR start "usetypesStmt"
    # interface.g:592:1: usetypesStmt : USETYPES filename ';' ;
    def usetypesStmt(self, ):
        filename68 = None

        try:
            try:
                # interface.g:593:5: ( USETYPES filename ';' )
                # interface.g:593:7: USETYPES filename ';'
                pass 
                self.match(self.input, USETYPES, self.FOLLOW_USETYPES_in_usetypesStmt2322)

                self._state.following.append(self.FOLLOW_filename_in_usetypesStmt2324)
                filename68 = self.filename()

                self._state.following.pop()

                self.match(self.input, SEMICOLON, self.FOLLOW_SEMICOLON_in_usetypesStmt2326)

                #action start
                        
                if filename68.endswith(".api"):
                    basename=filename68[:-4]
                    fullFilename=filename68
                else:
                    basename=filename68
                    fullFilename=filename68 + ".api"

                self.iface.imports[basename] = ParseCode(fullFilename, self.searchPath)
                        
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return 

    # $ANTLR end "usetypesStmt"



    # $ANTLR start "apiDocument"
    # interface.g:609:1: apiDocument returns [iface] : ( ( docPreComment )+ ( usetypesStmt | (firstVer= apiVersion firstServingVer= servingVersion ) | (firstMsgSize= apiMsgSize ) |firstDecl= documentedDeclaration ) )? ( usetypesStmt | (laterVer= apiVersion laterServingVer= servingVersion ) | (laterMsgSize= apiMsgSize ) | declaration |laterDecl= documentedDeclaration )* EOF ;
    def apiDocument(self, ):
        iface = None


        firstVer = None
        firstServingVer = None
        firstMsgSize = None
        firstDecl = None
        laterVer = None
        laterServingVer = None
        laterMsgSize = None
        laterDecl = None
        docPreComment69 = None
        declaration70 = None

        try:
            try:
                # interface.g:610:5: ( ( ( docPreComment )+ ( usetypesStmt | (firstVer= apiVersion firstServingVer= servingVersion ) | (firstMsgSize= apiMsgSize ) |firstDecl= documentedDeclaration ) )? ( usetypesStmt | (laterVer= apiVersion laterServingVer= servingVersion ) | (laterMsgSize= apiMsgSize ) | declaration |laterDecl= documentedDeclaration )* EOF )
                # interface.g:610:7: ( ( docPreComment )+ ( usetypesStmt | (firstVer= apiVersion firstServingVer= servingVersion ) | (firstMsgSize= apiMsgSize ) |firstDecl= documentedDeclaration ) )? ( usetypesStmt | (laterVer= apiVersion laterServingVer= servingVersion ) | (laterMsgSize= apiMsgSize ) | declaration |laterDecl= documentedDeclaration )* EOF
                pass 
                # interface.g:610:7: ( ( docPreComment )+ ( usetypesStmt | (firstVer= apiVersion firstServingVer= servingVersion ) | (firstMsgSize= apiMsgSize ) |firstDecl= documentedDeclaration ) )?
                alt29 = 2
                LA29_0 = self.input.LA(1)

                if (LA29_0 == DOC_PRE_COMMENT) :
                    LA29_1 = self.input.LA(2)

                    if ((API_MSG_SIZE <= LA29_1 <= API_VERSION) or LA29_1 == DOC_PRE_COMMENT or LA29_1 == USETYPES) :
                        alt29 = 1
                if alt29 == 1:
                    # interface.g:610:9: ( docPreComment )+ ( usetypesStmt | (firstVer= apiVersion firstServingVer= servingVersion ) | (firstMsgSize= apiMsgSize ) |firstDecl= documentedDeclaration )
                    pass 
                    # interface.g:610:9: ( docPreComment )+
                    cnt27 = 0
                    while True: #loop27
                        alt27 = 2
                        LA27_0 = self.input.LA(1)

                        if (LA27_0 == DOC_PRE_COMMENT) :
                            LA27_2 = self.input.LA(2)

                            if ((API_MSG_SIZE <= LA27_2 <= API_VERSION) or LA27_2 == DOC_PRE_COMMENT or LA27_2 == USETYPES) :
                                alt27 = 1




                        if alt27 == 1:
                            # interface.g:610:11: docPreComment
                            pass 
                            self._state.following.append(self.FOLLOW_docPreComment_in_apiDocument2364)
                            docPreComment69 = self.docPreComment()

                            self._state.following.pop()

                            #action start
                            self.iface.comments.append(docPreComment69) 
                            #action end



                        else:
                            if cnt27 >= 1:
                                break #loop27

                            eee = EarlyExitException(27, self.input)
                            raise eee

                        cnt27 += 1


                    # interface.g:611:13: ( usetypesStmt | (firstVer= apiVersion firstServingVer= servingVersion ) | (firstMsgSize= apiMsgSize ) |firstDecl= documentedDeclaration )
                    alt28 = 4
                    LA28 = self.input.LA(1)
                    if LA28 == USETYPES:
                        alt28 = 1
                    elif LA28 == API_VERSION:
                        alt28 = 2
                    elif LA28 == API_MSG_SIZE:
                        alt28 = 3
                    elif LA28 == DOC_PRE_COMMENT:
                        alt28 = 4
                    else:
                        nvae = NoViableAltException("", 28, 0, self.input)

                        raise nvae


                    if alt28 == 1:
                        # interface.g:611:16: usetypesStmt
                        pass 
                        self._state.following.append(self.FOLLOW_usetypesStmt_in_apiDocument2386)
                        self.usetypesStmt()

                        self._state.following.pop()


                    elif alt28 == 2:
                        # interface.g:612:6: (firstVer= apiVersion firstServingVer= servingVersion )
                        pass 
                        # interface.g:612:6: (firstVer= apiVersion firstServingVer= servingVersion )
                        # interface.g:613:7: firstVer= apiVersion firstServingVer= servingVersion
                        pass 
                        self._state.following.append(self.FOLLOW_apiVersion_in_apiDocument2404)
                        firstVer = self.apiVersion()

                        self._state.following.pop()

                        #action start
                        self.iface.api_version = firstVer
                        #action end


                        self._state.following.append(self.FOLLOW_servingVersion_in_apiDocument2416)
                        firstServingVer = self.servingVersion()

                        self._state.following.pop()

                        #action start
                        self.iface.serving_version = firstServingVer
                        #action end






                    elif alt28 == 3:
                        # interface.g:616:6: (firstMsgSize= apiMsgSize )
                        pass 
                        # interface.g:616:6: (firstMsgSize= apiMsgSize )
                        # interface.g:617:7: firstMsgSize= apiMsgSize
                        pass 
                        self._state.following.append(self.FOLLOW_apiMsgSize_in_apiDocument2445)
                        firstMsgSize = self.apiMsgSize()

                        self._state.following.pop()

                        #action start
                        self.iface.apiMsgMaxSize = firstMsgSize
                        #action end






                    elif alt28 == 4:
                        # interface.g:619:15: firstDecl= documentedDeclaration
                        pass 
                        self._state.following.append(self.FOLLOW_documentedDeclaration_in_apiDocument2472)
                        firstDecl = self.documentedDeclaration()

                        self._state.following.pop()

                        #action start
                                        
                        if firstDecl:
                            self.iface.addDeclaration(firstDecl)
                                        
                        #action end








                # interface.g:627:7: ( usetypesStmt | (laterVer= apiVersion laterServingVer= servingVersion ) | (laterMsgSize= apiMsgSize ) | declaration |laterDecl= documentedDeclaration )*
                while True: #loop30
                    alt30 = 6
                    LA30 = self.input.LA(1)
                    if LA30 == USETYPES:
                        alt30 = 1
                    elif LA30 == API_VERSION:
                        alt30 = 2
                    elif LA30 == API_MSG_SIZE:
                        alt30 = 3
                    elif LA30 == BITMASK or LA30 == DEFINE or LA30 == ENUM or LA30 == EVENT or LA30 == FUNCTION or LA30 == HANDLER or LA30 == REFERENCE or LA30 == STRUCT:
                        alt30 = 4
                    elif LA30 == DOC_PRE_COMMENT:
                        alt30 = 5

                    if alt30 == 1:
                        # interface.g:627:9: usetypesStmt
                        pass 
                        self._state.following.append(self.FOLLOW_usetypesStmt_in_apiDocument2519)
                        self.usetypesStmt()

                        self._state.following.pop()


                    elif alt30 == 2:
                        # interface.g:628:5: (laterVer= apiVersion laterServingVer= servingVersion )
                        pass 
                        # interface.g:628:5: (laterVer= apiVersion laterServingVer= servingVersion )
                        # interface.g:629:13: laterVer= apiVersion laterServingVer= servingVersion
                        pass 
                        self._state.following.append(self.FOLLOW_apiVersion_in_apiDocument2542)
                        laterVer = self.apiVersion()

                        self._state.following.pop()

                        #action start
                        self.iface.api_version = laterVer
                        #action end


                        self._state.following.append(self.FOLLOW_servingVersion_in_apiDocument2552)
                        laterServingVer = self.servingVersion()

                        self._state.following.pop()

                        #action start
                        self.iface.serving_version = laterServingVer
                        #action end






                    elif alt30 == 3:
                        # interface.g:632:5: (laterMsgSize= apiMsgSize )
                        pass 
                        # interface.g:632:5: (laterMsgSize= apiMsgSize )
                        # interface.g:633:14: laterMsgSize= apiMsgSize
                        pass 
                        self._state.following.append(self.FOLLOW_apiMsgSize_in_apiDocument2587)
                        laterMsgSize = self.apiMsgSize()

                        self._state.following.pop()

                        #action start
                        self.iface.apiMsgMaxSize = laterMsgSize
                        #action end






                    elif alt30 == 4:
                        # interface.g:636:11: declaration
                        pass 
                        self._state.following.append(self.FOLLOW_declaration_in_apiDocument2613)
                        declaration70 = self.declaration()

                        self._state.following.pop()

                        #action start
                                     
                        if declaration70:
                             self.iface.addDeclaration(declaration70)
                                     
                        #action end



                    elif alt30 == 5:
                        # interface.g:641:11: laterDecl= documentedDeclaration
                        pass 
                        self._state.following.append(self.FOLLOW_documentedDeclaration_in_apiDocument2642)
                        laterDecl = self.documentedDeclaration()

                        self._state.following.pop()

                        #action start
                                     
                        if laterDecl:
                             self.iface.addDeclaration(laterDecl)
                                     
                        #action end



                    else:
                        break #loop30


                self.match(self.input, EOF, self.FOLLOW_EOF_in_apiDocument2669)

                #action start
                iface = self.iface 
                #action end





            except RecognitionException as re:
                self.reportError(re)
                self.recover(self.input, re)

        finally:
            pass
        return iface

    # $ANTLR end "apiDocument"



    # lookup tables for DFA #14

    DFA14_eot = DFA.unpack(
        "\15\uffff"
        )

    DFA14_eof = DFA.unpack(
        "\15\uffff"
        )

    DFA14_min = DFA.unpack(
        "\1\25\1\uffff\1\15\1\13\1\15\2\uffff\2\13\4\15"
        )

    DFA14_max = DFA.unpack(
        "\1\55\1\uffff\1\55\1\46\1\55\2\uffff\2\13\4\55"
        )

    DFA14_accept = DFA.unpack(
        "\1\uffff\1\1\3\uffff\1\2\1\3\6\uffff"
        )

    DFA14_special = DFA.unpack(
        "\15\uffff"
        )


    DFA14_transition = [
        DFA.unpack("\1\2\27\uffff\1\1"),
        DFA.unpack(""),
        DFA.unpack("\1\4\27\uffff\1\6\3\uffff\1\3\3\uffff\1\5"),
        DFA.unpack("\1\11\10\uffff\1\12\17\uffff\1\7\1\uffff\1\10"),
        DFA.unpack("\1\4\27\uffff\1\6\7\uffff\1\5"),
        DFA.unpack(""),
        DFA.unpack(""),
        DFA.unpack("\1\13"),
        DFA.unpack("\1\14"),
        DFA.unpack("\1\4\27\uffff\1\6\7\uffff\1\5"),
        DFA.unpack("\1\4\27\uffff\1\6\7\uffff\1\5"),
        DFA.unpack("\1\4\27\uffff\1\6\7\uffff\1\5"),
        DFA.unpack("\1\4\27\uffff\1\6\7\uffff\1\5")
    ]

    # class definition for DFA #14

    class DFA14(DFA):
        pass


    # lookup tables for DFA #16

    DFA16_eot = DFA.unpack(
        "\24\uffff"
        )

    DFA16_eof = DFA.unpack(
        "\24\uffff"
        )

    DFA16_min = DFA.unpack(
        "\3\25\1\34\1\13\1\15\4\47\1\15\2\uffff\1\34\1\13\4\53\1\34"
        )

    DFA16_max = DFA.unpack(
        "\1\33\2\25\1\52\1\33\1\55\4\53\1\55\2\uffff\1\34\1\33\4\53\1\34"
        )

    DFA16_accept = DFA.unpack(
        "\13\uffff\1\1\1\2\7\uffff"
        )

    DFA16_special = DFA.unpack(
        "\24\uffff"
        )


    DFA16_transition = [
        DFA.unpack("\1\1\5\uffff\1\2"),
        DFA.unpack("\1\3"),
        DFA.unpack("\1\3"),
        DFA.unpack("\1\5\15\uffff\1\4"),
        DFA.unpack("\1\6\10\uffff\1\7\1\10\5\uffff\1\11"),
        DFA.unpack("\1\12\7\uffff\1\14\5\uffff\1\14\21\uffff\1\13"),
        DFA.unpack("\1\16\3\uffff\1\15"),
        DFA.unpack("\1\16\3\uffff\1\15"),
        DFA.unpack("\1\16\3\uffff\1\15"),
        DFA.unpack("\1\16\3\uffff\1\15"),
        DFA.unpack("\1\12\7\uffff\1\14\5\uffff\1\14\21\uffff\1\13"),
        DFA.unpack(""),
        DFA.unpack(""),
        DFA.unpack("\1\5"),
        DFA.unpack("\1\17\10\uffff\1\20\1\21\5\uffff\1\22"),
        DFA.unpack("\1\23"),
        DFA.unpack("\1\23"),
        DFA.unpack("\1\23"),
        DFA.unpack("\1\23"),
        DFA.unpack("\1\5")
    ]

    # class definition for DFA #16

    class DFA16(DFA):
        pass


    # lookup tables for DFA #20

    DFA20_eot = DFA.unpack(
        "\25\uffff"
        )

    DFA20_eof = DFA.unpack(
        "\25\uffff"
        )

    DFA20_min = DFA.unpack(
        "\3\25\1\15\1\13\3\15\2\uffff\4\47\1\15\1\13\4\53\1\15"
        )

    DFA20_max = DFA.unpack(
        "\1\33\2\25\1\52\1\33\3\45\2\uffff\4\53\1\45\1\33\4\53\1\45"
        )

    DFA20_accept = DFA.unpack(
        "\10\uffff\1\1\1\2\13\uffff"
        )

    DFA20_special = DFA.unpack(
        "\25\uffff"
        )


    DFA20_transition = [
        DFA.unpack("\1\1\5\uffff\1\2"),
        DFA.unpack("\1\3"),
        DFA.unpack("\1\3"),
        DFA.unpack("\1\7\10\uffff\1\5\1\uffff\1\6\11\uffff\1\10\2\uffff"
        "\1\11\4\uffff\1\4"),
        DFA.unpack("\1\12\10\uffff\1\13\1\14\5\uffff\1\15"),
        DFA.unpack("\1\7\24\uffff\1\10\2\uffff\1\11"),
        DFA.unpack("\1\7\24\uffff\1\10\2\uffff\1\11"),
        DFA.unpack("\1\7\24\uffff\1\10\2\uffff\1\11"),
        DFA.unpack(""),
        DFA.unpack(""),
        DFA.unpack("\1\17\3\uffff\1\16"),
        DFA.unpack("\1\17\3\uffff\1\16"),
        DFA.unpack("\1\17\3\uffff\1\16"),
        DFA.unpack("\1\17\3\uffff\1\16"),
        DFA.unpack("\1\7\10\uffff\1\5\1\uffff\1\6\11\uffff\1\10\2\uffff"
        "\1\11"),
        DFA.unpack("\1\20\10\uffff\1\21\1\22\5\uffff\1\23"),
        DFA.unpack("\1\24"),
        DFA.unpack("\1\24"),
        DFA.unpack("\1\24"),
        DFA.unpack("\1\24"),
        DFA.unpack("\1\7\10\uffff\1\5\1\uffff\1\6\11\uffff\1\10\2\uffff"
        "\1\11")
    ]

    # class definition for DFA #20

    class DFA20(DFA):
        pass


 

    FOLLOW_API_VERSION_in_apiVersion547 = frozenset([41])
    FOLLOW_41_in_apiVersion549 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_apiVersion553 = frozenset([28])
    FOLLOW_SEMICOLON_in_apiVersion555 = frozenset([1])
    FOLLOW_SERVING_VERSION_in_servingVersion587 = frozenset([41])
    FOLLOW_41_in_servingVersion589 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_servingVersion593 = frozenset([28])
    FOLLOW_SEMICOLON_in_servingVersion595 = frozenset([1])
    FOLLOW_API_MSG_SIZE_in_apiMsgSize626 = frozenset([41])
    FOLLOW_41_in_apiMsgSize628 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_apiMsgSize632 = frozenset([28])
    FOLLOW_SEMICOLON_in_apiMsgSize634 = frozenset([1])
    FOLLOW_IN_in_direction671 = frozenset([1])
    FOLLOW_OUT_in_direction685 = frozenset([1])
    FOLLOW_DEC_NUMBER_in_number713 = frozenset([1])
    FOLLOW_HEX_NUMBER_in_number726 = frozenset([1])
    FOLLOW_36_in_integer752 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_integer754 = frozenset([1])
    FOLLOW_38_in_integer766 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_integer768 = frozenset([1])
    FOLLOW_DEC_NUMBER_in_integer780 = frozenset([1])
    FOLLOW_HEX_NUMBER_in_integer796 = frozenset([1])
    FOLLOW_IDENTIFIER_in_defineValue825 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_defineValue842 = frozenset([1])
    FOLLOW_number_in_simpleNumber879 = frozenset([1])
    FOLLOW_defineValue_in_simpleNumber900 = frozenset([1])
    FOLLOW_integer_in_simpleExpression929 = frozenset([1])
    FOLLOW_defineValue_in_simpleExpression953 = frozenset([1])
    FOLLOW_33_in_simpleExpression973 = frozenset([11, 20, 21, 27, 33, 36, 38])
    FOLLOW_sumExpression_in_simpleExpression975 = frozenset([34])
    FOLLOW_34_in_simpleExpression977 = frozenset([1])
    FOLLOW_simpleExpression_in_productExpression1002 = frozenset([1, 35, 40])
    FOLLOW_35_in_productExpression1020 = frozenset([11, 20, 21, 27, 33, 36, 38])
    FOLLOW_simpleExpression_in_productExpression1024 = frozenset([1, 35, 40])
    FOLLOW_40_in_productExpression1038 = frozenset([11, 20, 21, 27, 33, 36, 38])
    FOLLOW_simpleExpression_in_productExpression1042 = frozenset([1, 35, 40])
    FOLLOW_productExpression_in_sumExpression1078 = frozenset([1, 36, 38])
    FOLLOW_36_in_sumExpression1096 = frozenset([11, 20, 21, 27, 33, 36, 38])
    FOLLOW_productExpression_in_sumExpression1100 = frozenset([1, 36, 38])
    FOLLOW_38_in_sumExpression1114 = frozenset([11, 20, 21, 27, 33, 36, 38])
    FOLLOW_productExpression_in_sumExpression1118 = frozenset([1, 36, 38])
    FOLLOW_sumExpression_in_valueExpression1152 = frozenset([1])
    FOLLOW_QUOTED_STRING_in_valueExpression1165 = frozenset([1])
    FOLLOW_42_in_arrayExpression1194 = frozenset([11, 20, 21, 27])
    FOLLOW_simpleNumber_in_arrayExpression1196 = frozenset([43])
    FOLLOW_43_in_arrayExpression1198 = frozenset([1])
    FOLLOW_42_in_arrayExpression1216 = frozenset([11, 20, 21, 27])
    FOLLOW_simpleNumber_in_arrayExpression1220 = frozenset([39])
    FOLLOW_39_in_arrayExpression1222 = frozenset([11, 20, 21, 27])
    FOLLOW_simpleNumber_in_arrayExpression1226 = frozenset([43])
    FOLLOW_43_in_arrayExpression1228 = frozenset([1])
    FOLLOW_IDENTIFIER_in_typeIdentifier1259 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_typeIdentifier1277 = frozenset([1])
    FOLLOW_DOC_POST_COMMENT_in_docPostComments1318 = frozenset([1, 13])
    FOLLOW_DOC_PRE_COMMENT_in_docPreComment1362 = frozenset([1])
    FOLLOW_DEFINE_in_defineDecl1385 = frozenset([21])
    FOLLOW_IDENTIFIER_in_defineDecl1387 = frozenset([41])
    FOLLOW_41_in_defineDecl1389 = frozenset([11, 20, 21, 25, 27, 33, 36, 38])
    FOLLOW_valueExpression_in_defineDecl1391 = frozenset([28])
    FOLLOW_SEMICOLON_in_defineDecl1393 = frozenset([1])
    FOLLOW_IDENTIFIER_in_namedValue1424 = frozenset([13, 41])
    FOLLOW_41_in_namedValue1428 = frozenset([11, 20, 36, 38])
    FOLLOW_integer_in_namedValue1430 = frozenset([13])
    FOLLOW_docPostComments_in_namedValue1435 = frozenset([1])
    FOLLOW_namedValue_in_namedValueList1510 = frozenset([1])
    FOLLOW_namedValue_in_namedValueList1553 = frozenset([37])
    FOLLOW_37_in_namedValueList1555 = frozenset([13, 21])
    FOLLOW_docPostComments_in_namedValueList1557 = frozenset([21])
    FOLLOW_namedValueList_in_namedValueList1561 = frozenset([1])
    FOLLOW_ENUM_in_enumDecl1592 = frozenset([21])
    FOLLOW_IDENTIFIER_in_enumDecl1594 = frozenset([44])
    FOLLOW_44_in_enumDecl1596 = frozenset([21, 45])
    FOLLOW_namedValueList_in_enumDecl1598 = frozenset([45])
    FOLLOW_45_in_enumDecl1600 = frozenset([28])
    FOLLOW_SEMICOLON_in_enumDecl1602 = frozenset([1])
    FOLLOW_BITMASK_in_bitmaskDecl1633 = frozenset([21])
    FOLLOW_IDENTIFIER_in_bitmaskDecl1635 = frozenset([44])
    FOLLOW_44_in_bitmaskDecl1637 = frozenset([21, 45])
    FOLLOW_namedValueList_in_bitmaskDecl1639 = frozenset([45])
    FOLLOW_45_in_bitmaskDecl1641 = frozenset([28])
    FOLLOW_SEMICOLON_in_bitmaskDecl1643 = frozenset([1])
    FOLLOW_REFERENCE_in_referenceDecl1674 = frozenset([21])
    FOLLOW_IDENTIFIER_in_referenceDecl1676 = frozenset([28])
    FOLLOW_SEMICOLON_in_referenceDecl1678 = frozenset([1])
    FOLLOW_typeIdentifier_in_compoundMember1709 = frozenset([21])
    FOLLOW_IDENTIFIER_in_compoundMember1711 = frozenset([28, 42])
    FOLLOW_arrayExpression_in_compoundMember1713 = frozenset([28])
    FOLLOW_SEMICOLON_in_compoundMember1716 = frozenset([13])
    FOLLOW_docPostComments_in_compoundMember1718 = frozenset([1])
    FOLLOW_compoundMember_in_compoundMemberList1749 = frozenset([1])
    FOLLOW_compoundMember_in_compoundMemberList1778 = frozenset([21, 27])
    FOLLOW_compoundMemberList_in_compoundMemberList1782 = frozenset([1])
    FOLLOW_STRUCT_in_structDecl1813 = frozenset([21])
    FOLLOW_IDENTIFIER_in_structDecl1815 = frozenset([44])
    FOLLOW_44_in_structDecl1817 = frozenset([21, 27, 45])
    FOLLOW_compoundMemberList_in_structDecl1819 = frozenset([45])
    FOLLOW_45_in_structDecl1822 = frozenset([28])
    FOLLOW_SEMICOLON_in_structDecl1824 = frozenset([1])
    FOLLOW_typeIdentifier_in_formalParameter1855 = frozenset([21])
    FOLLOW_IDENTIFIER_in_formalParameter1857 = frozenset([13, 22, 24, 42])
    FOLLOW_arrayExpression_in_formalParameter1859 = frozenset([13, 22, 24])
    FOLLOW_direction_in_formalParameter1862 = frozenset([13])
    FOLLOW_docPostComments_in_formalParameter1865 = frozenset([1])
    FOLLOW_formalParameter_in_formalParameterList1896 = frozenset([1])
    FOLLOW_formalParameter_in_formalParameterList1914 = frozenset([37])
    FOLLOW_37_in_formalParameterList1916 = frozenset([13, 21, 27])
    FOLLOW_docPostComments_in_formalParameterList1918 = frozenset([21, 27])
    FOLLOW_formalParameterList_in_formalParameterList1922 = frozenset([1])
    FOLLOW_FUNCTION_in_functionDecl1953 = frozenset([21, 27])
    FOLLOW_typeIdentifier_in_functionDecl1955 = frozenset([21])
    FOLLOW_IDENTIFIER_in_functionDecl1958 = frozenset([33])
    FOLLOW_33_in_functionDecl1960 = frozenset([21, 27, 34])
    FOLLOW_formalParameterList_in_functionDecl1962 = frozenset([34])
    FOLLOW_34_in_functionDecl1965 = frozenset([41])
    FOLLOW_41_in_functionDecl1967 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_functionDecl1971 = frozenset([28])
    FOLLOW_SEMICOLON_in_functionDecl1974 = frozenset([1])
    FOLLOW_HANDLER_in_handlerDecl2005 = frozenset([21])
    FOLLOW_IDENTIFIER_in_handlerDecl2007 = frozenset([33])
    FOLLOW_33_in_handlerDecl2009 = frozenset([21, 27, 34])
    FOLLOW_formalParameterList_in_handlerDecl2011 = frozenset([34])
    FOLLOW_34_in_handlerDecl2014 = frozenset([28])
    FOLLOW_SEMICOLON_in_handlerDecl2016 = frozenset([1])
    FOLLOW_EVENT_in_eventDecl2047 = frozenset([21])
    FOLLOW_IDENTIFIER_in_eventDecl2049 = frozenset([33])
    FOLLOW_33_in_eventDecl2051 = frozenset([21, 27, 34])
    FOLLOW_formalParameterList_in_eventDecl2053 = frozenset([34])
    FOLLOW_34_in_eventDecl2056 = frozenset([41])
    FOLLOW_41_in_eventDecl2058 = frozenset([33])
    FOLLOW_33_in_eventDecl2060 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_eventDecl2064 = frozenset([37])
    FOLLOW_37_in_eventDecl2066 = frozenset([11])
    FOLLOW_DEC_NUMBER_in_eventDecl2070 = frozenset([34])
    FOLLOW_34_in_eventDecl2072 = frozenset([28])
    FOLLOW_SEMICOLON_in_eventDecl2074 = frozenset([1])
    FOLLOW_enumDecl_in_declaration2108 = frozenset([1])
    FOLLOW_bitmaskDecl_in_declaration2125 = frozenset([1])
    FOLLOW_referenceDecl_in_declaration2139 = frozenset([1])
    FOLLOW_structDecl_in_declaration2151 = frozenset([1])
    FOLLOW_functionDecl_in_declaration2166 = frozenset([1])
    FOLLOW_handlerDecl_in_declaration2179 = frozenset([1])
    FOLLOW_eventDecl_in_declaration2193 = frozenset([1])
    FOLLOW_defineDecl_in_declaration2209 = frozenset([1])
    FOLLOW_docPreComment_in_documentedDeclaration2249 = frozenset([8, 12, 15, 16, 17, 18, 26, 30])
    FOLLOW_declaration_in_documentedDeclaration2251 = frozenset([1])
    FOLLOW_IDENTIFIER_in_filename2284 = frozenset([1])
    FOLLOW_SCOPED_IDENTIFIER_in_filename2302 = frozenset([1])
    FOLLOW_USETYPES_in_usetypesStmt2322 = frozenset([21, 27])
    FOLLOW_filename_in_usetypesStmt2324 = frozenset([28])
    FOLLOW_SEMICOLON_in_usetypesStmt2326 = frozenset([1])
    FOLLOW_docPreComment_in_apiDocument2364 = frozenset([6, 7, 14, 31])
    FOLLOW_usetypesStmt_in_apiDocument2386 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_apiVersion_in_apiDocument2404 = frozenset([29])
    FOLLOW_servingVersion_in_apiDocument2416 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_apiMsgSize_in_apiDocument2445 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_documentedDeclaration_in_apiDocument2472 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_usetypesStmt_in_apiDocument2519 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_apiVersion_in_apiDocument2542 = frozenset([29])
    FOLLOW_servingVersion_in_apiDocument2552 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_apiMsgSize_in_apiDocument2587 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_declaration_in_apiDocument2613 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_documentedDeclaration_in_apiDocument2642 = frozenset([6, 7, 8, 12, 14, 15, 16, 17, 18, 26, 30, 31])
    FOLLOW_EOF_in_apiDocument2669 = frozenset([1])



def main(argv, stdin=sys.stdin, stdout=sys.stdout, stderr=sys.stderr):
    from antlr3.main import ParserMain
    main = ParserMain("interfaceLexer", interfaceParser)

    main.stdin = stdin
    main.stdout = stdout
    main.stderr = stderr
    main.execute(argv)



def ParseCode(apiFile, searchPath=[], ifaceName=None):
    if os.path.isabs(apiFile) or os.path.isfile(apiFile):
        apiPath = apiFile
    else:
        for path in searchPath:
            apiPath = os.path.join(path, apiFile)
            if os.path.isfile(apiPath):
                break
        else:
            # File not found in search path.  Try as a relative path.
            # This will usually fail since the current directory should be in the search
            # path but at least will raise a reasonable exception
            apiPath = apiFile

    fileStream = ANTLRFileStream(apiPath, 'utf-8')
    lexer = interfaceLexer(fileStream)
    tokens = CommonTokenStream(lexer)
    parser = interfaceParser(tokens)
    parser.searchPath=searchPath
    baseName = os.path.splitext(os.path.basename(apiPath))[0]
    if ifaceName == None:
        parser.iface.name = baseName
    else:
        parser.iface.name = ifaceName
    parser.iface.baseName = baseName
    parser.iface.path = apiPath

    iface = parser.apiDocument()

    if (parser.getNumberOfSyntaxErrors() > 0 or
        parser.compileErrors > 0):
        return None

    if tokens.getTokens():
        iface.text= ''.join([ token.text
                            for token in tokens.getTokens()
                            if token.type not in frozenset([ C_COMMENT,
                                                           CPP_COMMENT,
                                                           DOC_PRE_COMMENT,
                                                           DOC_POST_COMMENT ]) ])

    return iface


if __name__ == '__main__':
    main(sys.argv)
