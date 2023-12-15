import re
import os
import glob

# sorry for this monstrosity

# TODO
# 1. remove hard-coded includes and add them automatically
# 2. redesign

class Generator:

    def __init__(self, IR, sinkPath, blueprintDir):
        self.IR = IR
        self.sinkPath = sinkPath
        self.blueprintDir = blueprintDir
        self.messageHeaderBlueprint = self._getMessageBlueprint(True)
        self.messageSourceBlueprint = self._getMessageBlueprint()
        self.serverHeaderBlueprint = self._getServerHeaderBlueprint()
        self.serverSourceBlueprint = self._getServerSourceBlueprint()

    def generate(self):
        self.clearGenDir()
        self._traverseProtoDescriptor()

    def clearGenDir(self):
        files = glob.glob(self.sinkPath + "/*")
        for f in files:
            os.remove(f)

    def _getMessageBlueprint(self, isHeader=False):
        headerPath = ""
        if isHeader:
            headerPath = self.blueprintDir + "/message_blueprint_h.txt"
        else:
            headerPath = self.blueprintDir + "/message_blueprint_cpp.txt"

        with open(headerPath, "r") as blueprint:
            data = blueprint.read()
            return str(sorted(re.findall(r'(namespace\s*[^$]*\s*\})', data), key=len, reverse=True)[0])

    def _getServerHeaderBlueprint(self):
        headerPath = self.blueprintDir + "/server_blueprint_h.txt"
        with open(headerPath, "r") as blueprint:
            data = blueprint.read()
            return sorted(re.findall(r'(class\s*[^$]*\s*\;)', data), key=len, reverse=True)[0]

    def _getServerSourceBlueprint(self):
        headerPath = self.blueprintDir + "/server_blueprint_cpp.txt"
        with open(headerPath, "r") as blueprint:
            data = blueprint.read()
            return sorted(re.findall(r'(net::Packet\s*[^$]*\s*\})', data), key=len, reverse=True)[0]

    def _traverseProtoDescriptor(self):
        d = {}
        for handlerDescriptor in list(self.IR):
            new = dict(d, **handlerDescriptor)
            d = new
            self._traverseHandlerDescriptor(d)
        assert self._appendToCppServerFiles(d)

    def _isMessageInFile(self, messageName):
        path = self.sinkPath + "/gen_messages.h"
        if not os.path.isfile(path):
            return False
        with open(path, "r") as reader:
            data = reader.read()
            if messageName in data:
                return True
            return False

    def _traverseHandlerDescriptor(self, handlerDescriptor):

        def getFirstKeyOfDict(d):
            for key, _ in d.items():
                return key

        for handlerName, handlerSynopsis in handlerDescriptor.items():
            if len(handlerName) == 0:
                raise Exception("HandlerDescriptor must contain a name")
            if "request" not in handlerSynopsis or "response" not in handlerSynopsis:
                raise Exception("HandlerDescriptor must contain request and response description")

            requestDict = handlerSynopsis["request"]
            responseDict = handlerSynopsis["response"]

            if not self._isMessageInFile(getFirstKeyOfDict(requestDict)):
                assert self._appendToCppMessageFiles(requestDict)

            if not self._isMessageInFile(getFirstKeyOfDict(responseDict)):
                assert self._appendToCppMessageFiles(responseDict)

    def _appendToCppMessageFiles(self, message):
        if len(message.items()) != 1:
            raise Exception("MessageDescriptor must contain exactly one descriptor")

        headerGenFile = self.sinkPath + "/gen_messages.h"
        sourceGenFile = self.sinkPath + "/gen_messages.cpp"

        for messageName, messageSynopsis in message.items():
            headerStatus = self._appendToCppMessageHeader(messageName, messageSynopsis, headerGenFile)
            sourceStatus = self._appendToCppMessageSource(messageName, messageSynopsis, sourceGenFile)
            return headerStatus and sourceStatus

    def _appendToCppServerFiles(self, handlerDescriptor):
        items = handlerDescriptor.items()
        if len(items) < 1:
            raise Exception("Handler must contain at least one descriptor")

        headerGenFile = self.sinkPath + "/gen_server_stub.h"
        sourceGenFile = self.sinkPath + "/gen_server_stub.cpp"

        pairs = []
        for handlerName, handlerSynopsis in items:
            pairs.append({handlerName: handlerSynopsis})

        headerStatus = self._appendToCppServerHeader(pairs, headerGenFile)
        sourceStatus = self._appendToCppServerSource(pairs, sourceGenFile)
        return headerStatus and sourceStatus

    def _appendToCppServerHeader(self, pairs, headerGenFile):
        handlerBlueprint = self._getServerHeaderBlueprint()
        section = ""
        for handlerDescriptor in pairs:
            for handlerName, _ in handlerDescriptor.items():
                section += f"virtual net::ResponsePtr {handlerName}(const net::RequestPtr& requestPtr) = 0;\n"

        filler = {
            "SERVER_NAME": 'AbstractServer',
            "HEADER_ABSTRACT_HANDLER_SECTION": section
        }

        with open(headerGenFile, "a+") as header:
            header.seek(0)
            if section in header.read():
                return True
            if os.stat(headerGenFile).st_size == 0:
                header.write("\n#pragma once \n #include <vector> \n #include \"../abstract/sstub.h\"\n")
            header.write(handlerBlueprint.format(**filler))
        return True

    def _appendToCppServerSource(self, pairs, sourceGenFile):
        handlerBlueprint = self._getServerSourceBlueprint()
        section = ""
        for handlerDescriptor in pairs:
            for handlerName, handlerSynopsis in handlerDescriptor.items():
                request = handlerSynopsis["request"]
                reqItems = request.items()
                if len(reqItems) != 1:
                    return False
                for requestName, _ in reqItems:
                    section += f"""
                case net::stub::hash("{handlerName}"):
                    {{
                        auto packet = net::deserialize<std::shared_ptr<net::{requestName}::{requestName}>>(frame.frame);
                        responsePtr = {handlerName}(packet.body);
                        break;
                    }}
                """

        filler = {
            "SERVER_NAME": 'AbstractServer',
            "ROUTING_SECTION": section
        }

        with open(sourceGenFile, "a+") as source:
            source.seek(0)
            if section in source.read():
                return True
            if os.stat(sourceGenFile).st_size == 0:
                source.write(
                    "\n#include \"gen_server_stub.h\"\n#include \"gen_messages.h\"\n#include \"../common/buf_utils.h\"\n\n")
            source.write(handlerBlueprint.format(**filler))
        return True

    def _findBetween(self, src, first, last):
        try:
            start = src.index(first) + len(first)
            end = src.index(last, start)
            return src[start:end]
        except ValueError:
            return ""

    def _castArrayTypeToCpp(self, arrayType):
        arrayType = "".join(arrayType.split())
        cppNestedType = self._castProtoTypeToCpp(self._findBetween(arrayType, '[', ']'))
        return f"std::vector<{cppNestedType}>"

    def _castProtoTypeToCpp(self, protoType):
        if protoType == "int32":
            return "int"
        elif protoType == "int64":
            return "int64_t"
        elif protoType == "uint64":
            return "uint64_t"
        elif protoType == "bool":
            return "bool"
        elif protoType == "string":
            return "std::string"
        elif "array" in protoType:
            return self._castArrayTypeToCpp(protoType)
        else:
            raise ValueError("Invalid data type")

    def _constCoreMemberSection(self, messageSynopsis):
        core = ""
        for fieldName, fieldType in messageSynopsis.items():
            core += self._castProtoTypeToCpp(fieldType) + f" {fieldName}" + ";\n"
        return core

    def _constSignatureGetterSection(self, messageSynopsis):
        getterSignatures = ""
        for fieldName, fieldType in messageSynopsis.items():
            getterSignatures += self._castProtoTypeToCpp(fieldType) + "& get" + fieldName.capitalize() + "();\n"
        return getterSignatures

    def _constSerializerSection(self, messageSynopsis):
        section = "json = Json {"
        for name, _ in messageSynopsis.items():
            section += str('{' + f"\"{name}\", core.{name}" + '},')
        section = section[:-1]
        section += '};'
        return section

    def _constDeserializerSection(self, messageSynopsis):
        section = ""
        for name, _ in messageSynopsis.items():
            section += f"json.at(\"{name}\").get_to(core.{name});"
        return section

    def _constSourseGetterSection(self, messageName, messageSynopsis):
        section = ""
        for name, t in messageSynopsis.items():
            section += str(f"\n\t{self._castProtoTypeToCpp(t)}& {messageName}::get{name.capitalize()}()" +
                           f"{{return m_data.{name};}}")
        return section

    def _appendToCppMessageHeader(self, messageName, messageSynopsis, headerGenFile):
        messageBlueprint = self._getMessageBlueprint(isHeader=True)

        filler = {"MESSAGE_NAME": messageName,
                  "CORE_MEMBER_SECTION": self._constCoreMemberSection(messageSynopsis),
                  "HEADER_GETTER_SECTION": self._constSignatureGetterSection(messageSynopsis)
                  }

        with open(headerGenFile, "a+") as header:
            header.seek(0)
            if messageName in header.read():
                return True
            if os.stat(headerGenFile).st_size == 0:
                header.write("\n #pragma once\n#include <vector> \n #include \"../abstract/message.h\"\n")
            header.write(messageBlueprint.format(**filler))
        return True

    def _appendToCppMessageSource(self, messageName, messageSynopsis, sourceGenFile):
        messageBlueprint = self._getMessageBlueprint(isHeader=False)

        filler = {
            "MESSAGE_NAME": messageName,
            "SERIALIZE_SECTION": self._constSerializerSection(messageSynopsis),
            "DESERIALIZE_SECTION": self._constDeserializerSection(messageSynopsis),
            "SOURCE_GETTER_SECTION": self._constSourseGetterSection(messageName, messageSynopsis)
        }
        with open(sourceGenFile, "a+") as source:
            source.seek(0)
            if messageName in source.read():
                return True
            if os.stat(sourceGenFile).st_size == 0:
                source.write("\n#include \"gen_messages.h\"\n")
            source.write(messageBlueprint.format(**filler))
        return True
