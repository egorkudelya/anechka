import re

# Example of the resulting IR:

# fileDescriptor =
# [
#     {
#         RequestMatrixProcessing :
#         {
#             request: {MatrixRequest : {rows : int32, cols : int32, data : array[int32]}},
#             response: {BasicBooleanResponse : {value : bool}}
#         }
#     },
#     {
#         ...
#     }
# ]

# This parser takes heavy inspiration from the following article:
# https://www.booleanworld.com/building-recursive-descent-parsers-definitive-guide/

def readFile(filepath):
    with open(filepath) as f:
        return f.read()

def stripComments(message):
    return re.sub("#.*?\n", '', message, flags=re.DOTALL)

def tokenize(message):
    return message.split()

class ParserError(Exception):
    def __init__(self, i, msg):
        self.i = i
        self.msg = msg

    def __str__(self):
        return f"{self.msg} at position {self.i}"

class BaseParser:

    def __init__(self):
        self.fileDescriptor = []
        self.messageDescriptors = {}
        self.filepath = ""
        self.mergedTokens = ""
        self.cache = {}
        self.i = -1
        self.len = 0

    def parse(self, filepath):
        assert not self.isEnd()
        self.filepath = filepath
        tokens = tokenize(stripComments(readFile(filepath)))
        self.mergedTokens = " ".join(tokens)
        self.len = len(self.mergedTokens) - 1
        while self.i != self.len:
            self.parseNextEntity()
        return self.fileDescriptor

    def isEnd(self):
        return self.i == self.len

    def ignoreWhitespace(self):
        if self.i < self.len and self.mergedTokens[self.i + 1] == " ":
            self.i += 1

    def isInRange(self, i):
        return i < self.len

    def getRangeList(self, mergedRanges):
        if mergedRanges in self.cache:
            return self.cache[mergedRanges]
        curr = 0
        l = len(mergedRanges)
        res = []
        while curr < l:
            if curr + 2 < l and mergedRanges[curr + 1] == "-":
                if mergedRanges[curr] >= mergedRanges[curr + 2]:
                    raise ValueError(f"Bad character range. The character before {'-'} "
                                     f"must define the lower boundary, "
                                     f"while the character after must define the upper boundary")
                res.append(mergedRanges[curr:curr + 3])
                curr += 3
            else:
                res.append(mergedRanges[curr])
                curr += 1
        self.cache[mergedRanges] = res
        return res

    def char(self, chars=None):
        if not self.isInRange(self.i):
            raise ParserError(
                self.i + 1,
                "Expected a char bot got end of string"
            )
        nextChar = self.mergedTokens[self.i + 1]
        if not chars:
            self.i += 1
            return nextChar

        for charRange in self.getRangeList(chars):
            if len(charRange) == 1:
                if nextChar == charRange:
                    self.i += 1
                    return nextChar
            elif charRange[0] <= nextChar <= charRange[2]:
                self.i += 1
                return nextChar

        raise ParserError(
            self.i + 1,
            f"Char error"
        )

    def keyword(self, *keywords):
        self.ignoreWhitespace()
        if not self.isInRange(self.i):
            raise ParserError(
                self.i + 1,
                f"Expected keywords {','.join(keywords)} bot got end of string"
            )
        for keyword in keywords:
            first = self.i + 1
            last = first + len(keyword)
            if self.mergedTokens[first:last] == keyword:
                self.i += len(keyword)
                self.ignoreWhitespace()
                return keyword

        raise ParserError(
            self.i + 1,
            f"Did not find any matching keywords"
        )

    def match(self, *rules):
        self.ignoreWhitespace()
        lastErrorIdx = -1
        lastException = None
        faultyRules = []
        for rule in rules:
            start = self.i
            try:
                res = getattr(self, rule)()
                self.ignoreWhitespace()
                return res
            except ParserError as err:
                self.i = start
                if err.i > lastErrorIdx:
                    lastErrorIdx = err.i
                    lastException = err
                    faultyRules = [rule]
                elif err.i == lastErrorIdx:
                    faultyRules.append(rule)

        if len(faultyRules) == 1:
            raise lastException
        else:
            raise ParserError(
                self.i + 1,
                f"Expected {''.join(faultyRules)} but got {self.mergedTokens[lastErrorIdx]}"
            )

    def tryChar(self, chars=None):
        try:
            return self.char(chars)
        except ParserError:
            return None

    def tryKeyword(self, *keywords):
        try:
            return self.keyword(*keywords)
        except ParserError:
            return None

    def tryMatch(self, *rules):
        try:
            return self.match(*rules)
        except ParserError:
            return None


class ProtoParser(BaseParser):

    def parseNextEntity(self):
        return self.match("anyType")

    def anyType(self):
        return self.match("complexType", "primitiveType")

    def primitiveType(self):
        return self.match("string")

    def complexType(self):
        return self.match("method", "message")

    def string(self):
        validRanges = "0-9A-Za-z[]"
        chars = [self.char(validRanges)]
        while True:
            char = self.tryChar(validRanges)
            if char is None:
                break
            chars.append(char)

        return ''.join(chars).rstrip(' \t')

    def pair(self):
        key = self.match("string")
        if type(key) is not str:
            raise ParserError(
                self.i + 1,
                f"Expected a string, but got {type(key)}"
            )
        self.keyword(':')
        value = self.match("string")
        return key, value

    def message(self):
        self.keyword("message")
        self.ignoreWhitespace()
        messageName = self.string()
        self.ignoreWhitespace()
        messageBody = self.messageBody()
        self.messageDescriptors[messageName] = messageBody
        return {messageName: messageBody}

    def messageBody(self):
        res = {}
        self.keyword("{")
        while True:
            pair = self.tryMatch("pair")
            if not pair:
                break
            res[pair[0]] = pair[1]
            if not self.tryKeyword(','):
                break
        self.keyword('}')
        return res

    def method(self):
        self.keyword("method")
        methodName = self.match("string")
        self.keyword('(')
        requestName = self.match("string")
        if requestName not in self.messageDescriptors:
            raise ParserError(
                self.i,
                f"Request message {requestName} not defined"
            )
        request = self.messageDescriptors[requestName]
        self.keyword(')')
        self.keyword('->')
        responseName = self.match("string")
        if responseName not in self.messageDescriptors:
            raise ParserError(
                self.i,
                f"Response message {responseName} not defined"
            )
        response = self.messageDescriptors[responseName]
        self.keyword(';')
        methodDescriptor = {methodName: {"request": {requestName: request}, "response": {responseName: response}}}
        self.fileDescriptor.append(methodDescriptor)
        return methodDescriptor
