/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*

  Author(s):  Anton Deguet
  Created on: 2010-09-06

  (C) Copyright 2010-2013 Johns Hopkins University (JHU), All Rights Reserved.

  --- begin cisst license - do not edit ---

  This software is provided "as is" under an open source license, with
  no warranty.  The complete license can be found in license.txt and
  http://www.cisst.org/cisst/license.txt.

  --- end cisst license ---

*/

#include "cdgClass.h"
#include "cdgInline.h"

CMN_IMPLEMENT_SERVICES(cdgClass);


cdgClass::cdgClass(size_t lineNumber):
    cdgScope("class", lineNumber)
{
    CMN_ASSERT(this->AddField("name", "", true, "name of the generated C++ class"));
    CMN_ASSERT(this->AddField("attribute", "", false, "string placed between 'class' and the class name (e.g. CISST_EXPORT)"));

    cdgField * field;
    field = this->AddField("ctor-all-members", "false", false, "adds a constructor requiring an initial value for each member");
    CMN_ASSERT(field);
    field->AddPossibleValue("true");
    field->AddPossibleValue("false");

    field = this->AddField("generate-human-readable", "true", false, "generate the code for std::string _type.HumanReadable(void), set this to false to provide own implementation");
    CMN_ASSERT(field);
    field->AddPossibleValue("true");
    field->AddPossibleValue("false");

    CMN_ASSERT(this->AddField("namespace", "", false, "namespace for the class"));

    field = this->AddField("mts-proxy", "true", false, "generate the code to create a cisstMultiTask proxy, set this to false to avoid proxy generation or \"declaration-only\" to manually instantiate the proxy in a different source file (.cpp)");
    CMN_ASSERT(field);
    field->AddPossibleValue("true");
    field->AddPossibleValue("declaration-only");
    field->AddPossibleValue("false");

    this->AddKnownScope(*this);

    cdgBaseClass newBaseClass(0);
    this->AddSubScope(newBaseClass);

    cdgTypedef newTypedef(0);
    this->AddSubScope(newTypedef);

    cdgMember newMember(0);
    this->AddSubScope(newMember);

    cdgEnum newEnum(0);
    this->AddSubScope(newEnum);

    cdgInline newInline(0, cdgInline::CDG_INLINE_HEADER);
    this->AddSubScope(newInline);

    cdgInline newCode(0, cdgInline::CDG_INLINE_CODE);
    this->AddSubScope(newCode);
}


cdgScope::Type cdgClass::GetScope(void) const
{
    return cdgScope::CDG_CLASS;
}


cdgScope * cdgClass::Create(size_t lineNumber) const
{
    return new cdgClass(lineNumber);
}


bool cdgClass::Validate(std::string & CMN_UNUSED(errorMessage))
{
    cdgMember * memberPtr;
    cdgBaseClass * baseClassPtr;
    cdgTypedef * typedefPtr;
    cdgEnum * enumPtr;
    const ScopesContainer::iterator end = Scopes.end();
    ScopesContainer::iterator iter;
    for (iter = Scopes.begin();
         iter != end;
         ++iter) {
        memberPtr = dynamic_cast<cdgMember *>(*iter);
        if (memberPtr) {
            Members.push_back(memberPtr);
        } else {
            baseClassPtr = dynamic_cast<cdgBaseClass *>(*iter);
            if (baseClassPtr) {
                BaseClasses.push_back(baseClassPtr);
            } else {
                typedefPtr = dynamic_cast<cdgTypedef *>(*iter);
                if (typedefPtr) {
                    Typedefs.push_back(typedefPtr);
                } else {
                    enumPtr = dynamic_cast<cdgEnum *>(*iter);
                    if (enumPtr) {
                        Enums.push_back(enumPtr);
                    }
                }
            }
        }
    }
    for (size_t index = 0; index < Members.size(); index++) {
        Members[index]->ClassName = this->ClassWithNamespace(); // this->GetFieldValue("name");
    }

    return true;
}


void cdgClass::GenerateIncludes(std::ostream & outputStream) const
{
    GenerateLineComment(outputStream);
    const std::string mtsProxy = this->GetFieldValue("mts-proxy");
    // includes for mts proxy
    if (mtsProxy != "false") {
        outputStream << std::endl
                     << "// mts-proxy set to " << mtsProxy << std::endl
                     << "#include <cisstCommon/cmnClassServices.h>" << std::endl
                     << "#include <cisstCommon/cmnClassRegisterMacros.h>" << std::endl
                     << "#include <cisstMultiTask/mtsGenericObjectProxy.h>" << std::endl
                     << std::endl;
    }
}


void cdgClass::GenerateHeader(std::ostream & outputStream) const
{
    GenerateLineComment(outputStream);
    const std::string className = this->GetFieldValue("name");
    const std::string classNamespace = this->GetFieldValue("namespace");
    const std::string mtsProxy = this->GetFieldValue("mts-proxy");

    size_t index;

    // class definition
    if (classNamespace != "") {
        outputStream << "namespace " << classNamespace << " {" << std::endl;
    }
    outputStream << "class " << this->GetFieldValue("attribute") << " " << className;

    if (BaseClasses.size() != 0) {
        outputStream << ": ";
    }
    for (index = 0; index < BaseClasses.size(); index++) {
        BaseClasses[index]->GenerateHeaderInheritance(outputStream);
        if (index != (BaseClasses.size() - 1)) {
            outputStream << ", ";
        }
    }
    outputStream << std::endl
                 << "{" << std::endl;

    // constructors and destructor
    outputStream << " /* default constructors and destructors. */" << std::endl
                 << " public:" << std::endl
                 << "    " << className << "(void);" << std::endl
                 << "    " << className << "(const " << this->GetFieldValue("name") << " & other);" << std::endl
                 << "    ~" << className << "();" << std::endl << std::endl;

    // generate code for all scopes
    for (index = 0; index < Scopes.size(); index++) {
        Scopes[index]->GenerateHeader(outputStream);
    }

    // create a constructor that requires all data members to be
    // initialized.  Must appear after all scope code since this might
    // require some user defined enum and/or typedef
    if (this->GetFieldValue("ctor-all-members") == "true" && Members.size() > 0) {
        outputStream << std::endl
                     << " public:" << std::endl
                     << "    /* ctor-all-member is set to: true */" << std::endl
                     << "    " << className << "(";
        std::string memberName, memberType;
        for (index = 0; index < Members.size(); index++) {
            memberName = Members[index]->GetFieldValue("name");
            memberType = Members[index]->GetFieldValue("type");
            outputStream << "const " << memberType << " & new" << memberName;
            if (index != (Members.size() - 1)) {
                outputStream << ", ";
            }
        }
        outputStream << ");" << std::endl;
    }

    // methods declaration
    GenerateStandardMethodsHeader(outputStream);
    GenerateDataMethodsHeader(outputStream);

    outputStream << "}; // " << className << std::endl;

    if (classNamespace != "") {
        outputStream << "}; // end of namespace " << classNamespace << std::endl;
    }

    // make sure mts proxy is registered, use namespace if any
    if (mtsProxy != "false") {
        outputStream << std::endl
                     << "// mts-proxy set to " << mtsProxy << std::endl;
        if (classNamespace != "") {
            outputStream << "typedef mtsGenericObjectProxy<" << classNamespace << "::" << className << "> " << classNamespace << "_" << className << "Proxy;" << std::endl
                         << "CMN_DECLARE_SERVICES_INSTANTIATION(" << classNamespace << "_" << className << "Proxy);" << std::endl;
        } else {
            outputStream << "typedef mtsGenericObjectProxy<" << className << "> " << className << "Proxy;" << std::endl
                         << "CMN_DECLARE_SERVICES_INSTANTIATION(" << className << "Proxy);" << std::endl;
        }
        outputStream << std::endl;
    }

    // declaration of all global functions
    GenerateStandardFunctionsHeader(outputStream);
    GenerateDataFunctionsHeader(outputStream);

    // global functions for enums have to be declared after the enum is defined
    const std::string classWithNamespace = this->ClassWithNamespace();
    for (index = 0; index < Enums.size(); ++index) {
        Enums[index]->GenerateDataFunctionsHeader(outputStream, classWithNamespace, this->GetFieldValue("attribute"));
    }

}


void cdgClass::GenerateStandardMethodsHeader(std::ostream & outputStream) const
{
    outputStream << "    /* default methods */" << std::endl
                 << " public:" << std::endl
                 << "    void SerializeRaw(std::ostream & outputStream) const;" << std::endl
                 << "    void DeSerializeRaw(std::istream & inputStream);" << std::endl
                 << "    void ToStream(std::ostream & outputStream) const;" << std::endl
                 << "    void ToStreamRaw(std::ostream & outputStream, const char delimiter = ' '," << std::endl
                 << "                     bool headerOnly = false, const std::string & headerPrefix = \"\") const;" << std::endl;
}


void cdgClass::GenerateDataMethodsHeader(std::ostream & outputStream) const
{
    std::string name = this->GetFieldValue("name");
    outputStream << "    /* default data methods */" << std::endl
                 << " public:" << std::endl
                 << "    void Copy(const " << name << " & source);" << std::endl
                 << "    void SerializeBinary(std::ostream & outputStream) const throw (std::runtime_error);" << std::endl
                 << "    void DeSerializeBinary(std::istream & inputStream, const cmnDataFormat & localFormat, const cmnDataFormat & remoteFormat) throw (std::runtime_error);" << std::endl
                 << "    void SerializeText(std::ostream & outputStream, const char delimiter = ',') const throw (std::runtime_error);" << std::endl
                 << "    std::string SerializeDescription(const char delimiter = ',', const std::string & userDescription = \"\") const;" << std::endl
                 << "    void DeSerializeText(std::istream & inputStream, const char delimiter = ',') throw (std::runtime_error);" << std::endl
                 << "    std::string HumanReadable(void) const;" << std::endl
                 << "    bool ScalarNumberIsFixed(void) const;" << std::endl
                 << "    size_t ScalarNumber(void) const;" << std::endl
                 << "    double Scalar(const size_t index) const throw (std::out_of_range);" << std::endl
                 << "    std::string ScalarDescription(const size_t index, const std::string & userDescription = \"\") const throw (std::out_of_range);" << std::endl
                 << "#if CISST_HAS_JSON" << std::endl
                 << "    void SerializeTextJSON(Json::Value & jsonValue) const;" << std::endl
                 << "    void DeSerializeTextJSON(const Json::Value & jsonValue) throw (std::runtime_error);" << std::endl
                 << "#endif // CISST_HAS_JSON" << std::endl
                 << std::endl;
}


void cdgClass::GenerateCode(std::ostream & outputStream) const
{
    GenerateLineComment(outputStream);

    // if we need instantiation for mts proxy
    const std::string className = this->GetFieldValue("name");
    const std::string classNamespace = this->GetFieldValue("namespace");
    const std::string mtsProxy = this->GetFieldValue("mts-proxy");

    if (mtsProxy == "true") {
        outputStream << std::endl
                     << "// mts-proxy set to " << mtsProxy << std::endl;
        if (classNamespace != "") {
            outputStream << "CMN_IMPLEMENT_SERVICES_TEMPLATED(" << classNamespace << "_" << className << "Proxy);" << std::endl;
        } else {
            outputStream << "CMN_IMPLEMENT_SERVICES_TEMPLATED(" << className << "Proxy);" << std::endl;
        }
        outputStream << std::endl;
    }

    size_t index;
    GenerateConstructorsCode(outputStream);
    GenerateMethodSerializeRawCode(outputStream);
    GenerateMethodDeSerializeRawCode(outputStream);
    GenerateMethodToStreamCode(outputStream);
    GenerateMethodToStreamRawCode(outputStream);

    for (index = 0; index < Members.size(); index++) {
        Members[index]->GenerateCode(outputStream);
    }

    GenerateStandardFunctionsCode(outputStream);
    GenerateDataFunctionsCode(outputStream);
}


void cdgClass::GenerateConstructorsCode(std::ostream & outputStream) const
{
    size_t index;

    std::stringstream defaultConstructor;
    std::stringstream copyConstructor;
    std::stringstream membersConstructor;

    // default and copy constructors
    const std::string name = this->GetFieldValue("name");
    const std::string classWithNamespace = this->ClassWithNamespace();
    bool commaNeeded = false;

    defaultConstructor << classWithNamespace << "::" << name << "(void)";
    copyConstructor    << classWithNamespace << "::" << name << "(const " << name << " & other)";
    membersConstructor << classWithNamespace << "::" << name << "(";
    std::string memberName, memberType;
    for (index = 0; index < Members.size(); index++) {
        if (commaNeeded) {
            membersConstructor << ", ";
        }
        memberName = Members[index]->GetFieldValue("name");
        memberType = Members[index]->GetFieldValue("type");
        membersConstructor << "const " << memberType << " & new" << memberName;
        commaNeeded = true;
    }
    membersConstructor << ")";

    if ((BaseClasses.size() + Members.size())!= 0) {
        defaultConstructor << ":";
        copyConstructor    << ":";
        membersConstructor << ":";
    }
    defaultConstructor << std::endl;
    copyConstructor    << std::endl;
    membersConstructor << std::endl;
    commaNeeded = false;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (commaNeeded) {
            defaultConstructor << "    , ";
            copyConstructor    << "    , ";
            membersConstructor << "    , ";
        } else {
            defaultConstructor << "      ";
            copyConstructor    << "      ";
            membersConstructor << "      ";
        }
        defaultConstructor << BaseClasses[index]->GetFieldValue("type") << "()" << std::endl;
        copyConstructor    << BaseClasses[index]->GetFieldValue("type") << "(other)" << std::endl;
        membersConstructor << BaseClasses[index]->GetFieldValue("type") << "()" << std::endl;
        commaNeeded = true;
    }
    for (index = 0; index < Members.size(); index++) {
        // can use member initialization except for arrays
        if (!Members[index]->IsCArray) {
            if (commaNeeded) {
                defaultConstructor << "    , ";
                copyConstructor    << "    , ";
                membersConstructor << "    , ";
            } else {
                defaultConstructor << "      ";
                copyConstructor    << "      ";
                membersConstructor << "      ";
            }
            memberName = Members[index]->MemberName;
            defaultConstructor << memberName << "(" << Members[index]->GetFieldValue("default") << ")" << std::endl;
            copyConstructor    << memberName << "(other." << memberName << ")" << std::endl;
            membersConstructor << memberName << "(new" << Members[index]->GetFieldValue("name") << ")" << std::endl;
            commaNeeded = true;
        }
    }

    defaultConstructor << "{}" << std::endl << std::endl;
    copyConstructor    << "{}" << std::endl << std::endl;
    membersConstructor << "{}" << std::endl << std::endl;

    outputStream << defaultConstructor.str();
    outputStream << copyConstructor.str();
    if (this->GetFieldValue("ctor-all-members") == "true") {
        outputStream << membersConstructor.str();
    }

    // destructor
    outputStream << classWithNamespace << "::~" << name << "(void)"
                 << "{}" << std::endl << std::endl;
}


void cdgClass::GenerateMethodSerializeRawCode(std::ostream & outputStream) const
{
    size_t index;
    const std::string name = this->ClassWithNamespace();
    outputStream << std::endl
                 << "void " << name << "::SerializeRaw(std::ostream & outputStream) const" << std::endl
                 << "{" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            outputStream << "    " << BaseClasses[index]->GetFieldValue("type") << "::SerializeRaw(outputStream);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            if (Members[index]->GetFieldValue("is-size_t") == "true") {
                outputStream << "    cmnSerializeSizeRaw(outputStream, this->" << Members[index]->MemberName << ");" << std::endl;
            } else {
                outputStream << "    cmnSerializeRaw(outputStream, this->" << Members[index]->MemberName << ");" << std::endl;
            }
        }
    }
    outputStream << "}" << std::endl
                 << std::endl;
}


void cdgClass::GenerateMethodDeSerializeRawCode(std::ostream & outputStream) const
{
    size_t index;
    const std::string name = this->ClassWithNamespace();
    outputStream << std::endl
                 << "void " << name << "::DeSerializeRaw(std::istream & inputStream)" << std::endl
                 << "{" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            outputStream << "    " << BaseClasses[index]->GetFieldValue("type") << "::DeSerializeRaw(inputStream);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            if (Members[index]->GetFieldValue("is-size_t") == "true") {
                outputStream << "    cmnDeSerializeSizeRaw(inputStream, this->" << Members[index]->MemberName << ");" << std::endl;
            } else {
                outputStream << "    cmnDeSerializeRaw(inputStream, this->" << Members[index]->MemberName << ");" << std::endl;
            }
        }
    }
    outputStream << "}" << std::endl
                 << std::endl;
}


void cdgClass::GenerateMethodToStreamCode(std::ostream & outputStream) const
{
    const std::string name = this->ClassWithNamespace();
    outputStream << std::endl
                 << "void " << name << "::ToStream(std::ostream & outputStream) const" << std::endl
                 << "{" << std::endl
                 << "    outputStream << this->HumanReadable();" << std::endl
                 << "}" << std::endl;
}


void cdgClass::GenerateMethodToStreamRawCode(std::ostream & outputStream) const
{
    const std::string name = this->ClassWithNamespace();
    outputStream << std::endl
                 << "void " << name << "::ToStreamRaw(std::ostream & outputStream, const char delimiter, bool headerOnly, const std::string & headerPrefix) const" << std::endl
                 << "{" << std::endl
                 << "    if (headerOnly) {" << std::endl
                 << "        outputStream << cmnData<" << name << " >::SerializeDescription(*this, delimiter, headerPrefix);" << std::endl
                 << "    } else {" << std::endl
                 << "        cmnData<" << name << " >::SerializeText(*this, outputStream, delimiter);" << std::endl
                 << "    }" << std::endl
                 << "}" << std::endl;
}


void cdgClass::GenerateStandardFunctionsHeader(std::ostream & outputStream) const
{
    const std::string name = this->ClassWithNamespace();
    const std::string attribute = this->GetFieldValue("attribute");
    outputStream << "/* default functions */" << std::endl
                 << "void " << attribute << " cmnSerializeRaw(std::ostream & outputStream, const " << name << " & object);" << std::endl
                 << "void " << attribute << " cmnDeSerializeRaw(std::istream & inputStream, " << name << " & placeHolder);" << std::endl;
}


void cdgClass::GenerateDataFunctionsHeader(std::ostream & outputStream) const
{
    const std::string name = this->ClassWithNamespace();
    const std::string attribute = this->GetFieldValue("attribute");
    outputStream << "/* data functions */" << std::endl
                 << "template <> class cmnData<" << name << " > {" << std::endl
                 << "public: " << std::endl
                 << "    enum {IS_SPECIALIZED = 1};" << std::endl
                 << "    typedef " << name << " DataType;" << std::endl
                 << "    static void Copy(DataType & data, const DataType & source) {" << std::endl
                 << "        data.Copy(source);" << std::endl
                 << "    }" << std::endl
                 << "    static std::string SerializeDescription(const DataType & data, const char delimiter, const std::string & userDescription) {" << std::endl
                 << "        return data.SerializeDescription(delimiter, userDescription);" << std::endl
                 << "    }" << std::endl
                 << "    static void SerializeBinary(const DataType & data, std::ostream & outputStream) throw (std::runtime_error) {" << std::endl
                 << "        data.SerializeBinary(outputStream);" << std::endl
                 << "    }" << std::endl
                 << "    static void DeSerializeBinary(DataType & data, std::istream & inputStream, const cmnDataFormat & localFormat, const cmnDataFormat & remoteFormat) throw (std::runtime_error) {" << std::endl
                 << "        data.DeSerializeBinary(inputStream, localFormat, remoteFormat);" << std::endl
                 << "    }" << std::endl
                 << "    static void SerializeText(const DataType & data, std::ostream & outputStream, const char delimiter = ',') throw (std::runtime_error) {" << std::endl
                 << "        data.SerializeText(outputStream, delimiter);" << std::endl
                 << "    }" << std::endl
                 << "    static void DeSerializeText(DataType & data, std::istream & inputStream, const char delimiter = ',') throw (std::runtime_error) {" << std::endl
                 << "        data.DeSerializeText(inputStream, delimiter);" << std::endl
                 << "    }" << std::endl
                 << "    static std::string HumanReadable(const DataType & data) {" << std::endl
                 << "        return data.HumanReadable();" << std::endl
                 << "    }" << std::endl
                 << "    static bool ScalarNumberIsFixed(const DataType & data) {" << std::endl
                 << "        return data.ScalarNumberIsFixed();" << std::endl
                 << "    }" << std::endl
                 << "    static size_t ScalarNumber(const DataType & data) {" << std::endl
                 << "        return data.ScalarNumber();" << std::endl
                 << "    }" << std::endl
                 << "    static std::string ScalarDescription(const DataType & data, const size_t index, const std::string & userDescription = \"\") throw (std::out_of_range) {" << std::endl
                 << "        return data.ScalarDescription(index, userDescription);" << std::endl
                 << "    }" << std::endl
                 << "    static double Scalar(const DataType & data, const size_t index) throw (std::out_of_range) {" << std::endl
                 << "        return data.Scalar(index);" << std::endl
                 << "    }" << std::endl
                 << "};" << std::endl
                 << "inline std::ostream & operator << (std::ostream & outputStream, const " << name << " & data) {" << std::endl
                 << "    outputStream << cmnData<" << name << " >::HumanReadable(data);" << std::endl
                 << "    return outputStream;" << std::endl
                 << "}" << std::endl
                 << "#if CISST_HAS_JSON" << std::endl
                 << "template <> void " << attribute << " cmnDataJSON<" << name << " >::SerializeText(const " << name << " & data, Json::Value & jsonValue);" << std::endl
                 << "template <> void " << attribute << " cmnDataJSON<" << name << " >::DeSerializeText(" << name << " & data, const Json::Value & jsonValue) throw (std::runtime_error);" << std::endl
                 << "#endif // CISST_HAS_JSON" << std::endl;
}


void cdgClass::GenerateStandardFunctionsCode(std::ostream & outputStream) const
{
    const std::string name = this->ClassWithNamespace();
    outputStream << "/* default functions */" << std::endl
                 << "void cmnSerializeRaw(std::ostream & outputStream, const " << name << " & object)" << std::endl
                 << "{" << std::endl
                 << "    object.SerializeRaw(outputStream);" << std::endl
                 << "}" << std::endl
                 << "void cmnDeSerializeRaw(std::istream & inputStream, " << name << " & placeHolder)" << std::endl
                 << "{" << std::endl
                 << "    placeHolder.DeSerializeRaw(inputStream);" << std::endl
                 << "}" << std::endl;
}


void cdgClass::GenerateDataFunctionsCode(std::ostream & outputStream) const
{
    size_t index;
    std::string name, type;
    const std::string className = this->ClassWithNamespace();

    outputStream << "/* data functions */" << std::endl;

    outputStream << "void " << className << "::Copy(const " << className << " & source) {" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    cmnData<" << type << " >::Copy(*this, source);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    cmnData<" << type << " >::Copy" << "(this->" << name << ", source." << name << ");" << std::endl;
        }
    }
    outputStream << "}" << std::endl;



    outputStream << "void " << className << "::SerializeBinary(std::ostream & outputStream) const throw (std::runtime_error) {" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    cmnData<" << type << " >::SerializeBinary(*this, outputStream);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    cmnData<" << type << " >::SerializeBinary" << "(this->" << name << ", outputStream);" << std::endl;
        }
    }
    outputStream << "}" << std::endl;



    outputStream << "void " << className << "::DeSerializeBinary(std::istream & inputStream," << std::endl
                 << "                                            const cmnDataFormat & localFormat, const cmnDataFormat & remoteFormat) throw (std::runtime_error) {"<< std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    cmnData<" << type << " >::DeSerializeBinary(*this, inputStream, localFormat, remoteFormat);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            if (Members[index]->GetFieldValue("is-size_t") == "true") {
                outputStream << "    cmnDataDeSerializeBinary_size_t" << "(this->" << name << ", inputStream, localFormat, remoteFormat);" << std::endl;
            } else {
                outputStream << "    cmnData<" << type << " >::DeSerializeBinary" << "(this->" << name << ", inputStream, localFormat, remoteFormat);" << std::endl;
            }
        }
    }
    outputStream << "}" << std::endl;



    outputStream << "void " << className << "::SerializeText(std::ostream & outputStream, const char delimiter) const throw (std::runtime_error) {" << std::endl
                 << "    bool someData = false;" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    if (someData) {" << std::endl
                         << "        outputStream << delimiter;" << std::endl
                         << "    }" << std::endl
                         << "    someData = true;" << std::endl
                         << "    cmnData<" << type << " >::SerializeText(*this, outputStream, delimiter);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    if (someData) {" << std::endl
                         << "        outputStream << delimiter;" << std::endl
                         << "    }" << std::endl
                         << "    someData = true;" << std::endl
                         << "    " << "cmnData<" << type << " >::SerializeText(this->" << name << ", outputStream, delimiter);" << std::endl;
        }
    }
    outputStream << "}" << std::endl;

    outputStream << "std::string " << className << "::SerializeDescription(const char delimiter, const std::string & userDescription) const {" << std::endl
                 << "    bool someData = false;" << std::endl
                 << "    const std::string prefix = (userDescription == \"\") ? \"\" : (userDescription + \".\");" << std::endl
                 << "    std::stringstream description;" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    if (someData) {" << std::endl
                         << "        description << delimiter;" << std::endl
                         << "    }" << std::endl
                         << "    someData = true;" << std::endl
                         << "    description << cmnData<" << type << " >::SerializeDescription(*this, delimiter, userDescription);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    if (someData) {" << std::endl
                         << "        description << delimiter;" << std::endl
                         << "    }" << std::endl
                         << "    someData = true;" << std::endl
                         << "    description << cmnData<" << type << " >::SerializeDescription(this->" << name << ", delimiter, prefix + \""
                         << Members[index]->GetFieldValue("name") << "\");" << std::endl;
        }
    }
    outputStream << "    return description.str();" << std::endl
                 << "}" << std::endl;

    outputStream << "void " << className << "::DeSerializeText(std::istream & inputStream," << std::endl
                 << "                                          const char delimiter) throw (std::runtime_error) {"<< std::endl
                 << "    bool someData = false;" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    if (someData) {" << std::endl
                         << "        cmnDataDeSerializeTextDelimiter(inputStream, delimiter, \"" << className << "\");" << std::endl
                         << "    }" << std::endl
                         << "    someData = true;" << std::endl
                         << "    cmnData<" << type << " >::DeSerializeText(*this, inputStream, delimiter);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    if (someData) {" << std::endl
                         << "        cmnDataDeSerializeTextDelimiter(inputStream, delimiter, \"" << className << "\");" << std::endl
                         << "    }" << std::endl
                         << "    someData = true;" << std::endl
                         << "    cmnData<" << type << " >::DeSerializeText(this->" << name << ", inputStream, delimiter);" << std::endl;
        }
    }
    outputStream << "}" << std::endl;


    if (this->GetFieldValue("generate-human-readable") == "true") {
        outputStream << "std::string " << className << "::HumanReadable(void) const {" << std::endl
                     << "    std::stringstream description;" << std::endl
                     << "    description << \"" << className << "\" << std::endl;" << std::endl;
        for (index = 0; index < BaseClasses.size(); index++) {
            if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
                type = BaseClasses[index]->GetFieldValue("type");
                outputStream << "    description << cmnData< " << type << " >::HumanReadable(*this) << std::endl;" << std::endl;
            }
        }
        for (index = 0; index < Members.size(); index++) {
            if (Members[index]->GetFieldValue("is-data") == "true") {
                type = Members[index]->GetFieldValue("type");
                outputStream << "    description << \"  " << Members[index]->GetFieldValue("description")
                             << ":\" << cmnData<" << type << " >::HumanReadable(this->"
                             << Members[index]->MemberName << ");" << std::endl;
            }
        }
        outputStream << "    return description.str();" << std::endl
                     << "}" << std::endl;
    }


    outputStream << "bool " << className << "::ScalarNumberIsFixed(void) const {" << std::endl
                 << "    return true" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "           && cmnData<" << type << " >::ScalarNumberIsFixed(*this)" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "           && cmnData<" << type << " >::ScalarNumberIsFixed(this->" << name << ")" << std::endl;
        }
    }
    outputStream << "    ;" << std::endl
                 << "}" << std::endl;


    outputStream << "size_t " << className << "::ScalarNumber(void) const {" << std::endl
                 << "    return 0" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "           + cmnData< " << type << " >::ScalarNumber(*this)" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "           + cmnData<" << type << " >::ScalarNumber(this->" << name << ")" << std::endl;
        }
    }
    outputStream << "    ;" << std::endl
                 << "}" << std::endl;



    outputStream << "std::string " << className << "::ScalarDescription(const size_t index, const std::string & userDescription) const throw (std::out_of_range) {" << std::endl
                 << "    std::string prefix = (userDescription == \"\") ? \"\" : (userDescription + \".\");" << std::endl
                 << "    size_t baseIndex = 0;" << std::endl
                 << "    size_t currentMaxIndex = 0;" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    currentMaxIndex += cmnData<" << type << " >::ScalarNumber(*this);" << std::endl
                         << "    if (index < currentMaxIndex) {" << std::endl
                         << "        return cmnData<" << type << " >::ScalarDescription(*this, index - baseIndex, prefix);" << std::endl
                         << "    }" << std::endl
                         << "    baseIndex = currentMaxIndex;" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    currentMaxIndex += cmnData<" << type << " >::ScalarNumber(this->" << name << ");" << std::endl
                         << "    if (index < currentMaxIndex) {" << std::endl
                         << "        return cmnData<" << type << " >::ScalarDescription(this->" << name << ", index - baseIndex, prefix + \""
                         << Members[index]->GetFieldValue("name") << "\");" << std::endl
                         << "    }" << std::endl
                         << "    baseIndex = currentMaxIndex;" << std::endl;
        }
    }
    outputStream << "    cmnThrow(std::out_of_range(\"cmnDataScalarDescription: " << className << " index out of range\"));" << std::endl
                 << "    return \"\";" << std::endl
                 << "}" << std::endl;



    outputStream << "double " << className << "::Scalar(const size_t index) const throw (std::out_of_range) {" << std::endl
                 << "    size_t baseIndex = 0;" << std::endl
                 << "    size_t currentMaxIndex = 0;" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    currentMaxIndex += cmnData<" << type << " >::ScalarNumber(*this);" << std::endl
                         << "    if (index < currentMaxIndex) {" << std::endl
                         << "        return cmnData<" << type << " >::Scalar(*this, index - baseIndex);" << std::endl
                         << "    }" << std::endl
                         << "    baseIndex = currentMaxIndex;" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    currentMaxIndex += cmnData<" << type << " >::ScalarNumber(this->" << name << ");" << std::endl
                         << "    if (index < currentMaxIndex) {" << std::endl
                         << "        return cmnData<" << type << " >::Scalar(this->" << name << ", index - baseIndex);" << std::endl
                         << "    }" << std::endl
                         << "    baseIndex = currentMaxIndex;" << std::endl;
        }
    }
    outputStream << "    cmnThrow(std::out_of_range(\"cmnDataScalar: " << className << " index out of range\"));" << std::endl
                 << "    return 1.2345;" << std::endl
                 << "}" << std::endl;



    outputStream << "#if CISST_HAS_JSON" << std::endl
                 << "template <>" << std::endl
                 << "void cmnDataJSON<" << className << " >::SerializeText(const " << className << " & data, Json::Value & jsonValue) {" << std::endl
                 << "    data.SerializeTextJSON(jsonValue);" << std::endl
                 << "}" << std::endl
                 << "void " << className << "::SerializeTextJSON(Json::Value & jsonValue) const {" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    cmnDataJSON<" << type << " >::SerializeText(*(dynamic_cast<const " << type << "*>(this)), jsonValue);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    cmnDataJSON<" << type << " >::SerializeText(this->" << name << ", jsonValue[\""
                         << Members[index]->GetFieldValue("name") << "\"]);" << std::endl;
        }
    }
    outputStream << "}" << std::endl
                 << "template<>" << std::endl
                 << "void cmnDataJSON<" << className << " >::DeSerializeText(" << className << " & data, const Json::Value & jsonValue) throw (std::runtime_error) {" << std::endl
                 << "    data.DeSerializeTextJSON(jsonValue);" << std::endl
                 << "}" << std::endl
                 << "void " << className << "::DeSerializeTextJSON(const Json::Value & jsonValue) throw (std::runtime_error) {" << std::endl;
    for (index = 0; index < BaseClasses.size(); index++) {
        if (BaseClasses[index]->GetFieldValue("is-data") == "true") {
            type = BaseClasses[index]->GetFieldValue("type");
            outputStream << "    cmnDataJSON<" << type << " >::DeSerializeText(*(dynamic_cast<" << type << "*>(this)), jsonValue);" << std::endl;
        }
    }
    for (index = 0; index < Members.size(); index++) {
        if (Members[index]->GetFieldValue("is-data") == "true") {
            type = Members[index]->GetFieldValue("type");
            name = Members[index]->MemberName;
            outputStream << "    cmnDataJSON<" << type << " >::DeSerializeText(this->" << name << ", jsonValue[\""
                         << Members[index]->GetFieldValue("name") << "\"]);" << std::endl;
        }
    }
    outputStream << "}" << std::endl
                 << "#endif // CISST_HAS_JSON" << std::endl;

    for (index = 0; index < Enums.size(); ++index) {
        Enums[index]->GenerateDataFunctionsCode(outputStream, className);
    }
}


std::string cdgClass::ClassWithNamespace(void) const
{
    if (this->GetFieldValue("namespace") != "") {
        return this->GetFieldValue("namespace") + "::" + this->GetFieldValue("name");
    } else {
        return this->GetFieldValue("name");
    }
}
