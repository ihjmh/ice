// **********************************************************************
//
// Copyright (c) 2001
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#include <IceUtil/Functional.h>
#include <Slice/CPlusPlusUtil.h>
#include <Gen.h>
#include <limits>

using namespace std;
using namespace Slice;

Slice::Gen::Gen(const string& name, const string& base,	const string& include, const vector<string>& includePaths,
		const string& dllExport, const string& dir) :
    _base(base),
    _include(include),
    _includePaths(includePaths),
    _dllExport(dllExport)
{
    for (vector<string>::iterator p = _includePaths.begin(); p != _includePaths.end(); ++p)
    {
	if (p->length() && (*p)[p->length() - 1] != '/')
	{
	    *p += '/';
	}
    }

    string::size_type pos = _base.rfind('/');
    if (pos != string::npos)
    {
	_base.erase(0, pos + 1);
    }

    string fileH = _base + ".h";
    string fileC = _base + ".cpp";
    if (!dir.empty())
    {
	fileH = dir + '/' + fileH;
	fileC = dir + '/' + fileC;
    }

    H.open(fileH.c_str());
    if (!H)
    {
	cerr << name << ": can't open `" << fileH << "' for writing: " << strerror(errno) << endl;
	return;
    }
    
    C.open(fileC.c_str());
    if (!C)
    {
	cerr << name << ": can't open `" << fileC << "' for writing: " << strerror(errno) << endl;
	return;
    }

    printHeader(H);
    printHeader(C);
    H << "\n// Generated from file `" << changeInclude(_base, _includePaths) << ".ice'\n";
    C << "\n// Generated from file `" << changeInclude(_base, _includePaths) << ".ice'\n";

    string s = fileH;
    if (_include.size())
    {
	s = _include + '/' + s;
    }
    transform(s.begin(), s.end(), s.begin(), ToIfdef());
    H << "\n#ifndef __" << s << "__";
    H << "\n#define __" << s << "__";
    H << '\n';
}

Slice::Gen::~Gen()
{
    H << "\n\n#endif\n";
    C << '\n';
}

bool
Slice::Gen::operator!() const
{
    return !H || !C;
}

void
Slice::Gen::generate(const UnitPtr& unit)
{
    C << "\n#include <";
    if (_include.size())
    {
	C << _include << '/';
    }
    C << _base << ".h>";

    H << "\n#include <Ice/ProxyF.h>";
    H << "\n#include <Ice/ObjectF.h>";
    H << "\n#include <Ice/LocalObjectF.h>";
    H << "\n#include <Ice/Exception.h>";
    if (unit->hasProxies())
    {
	H << "\n#include <Ice/Proxy.h>";
	H << "\n#include <Ice/Object.h>";
	H << "\n#include <Ice/Outgoing.h>";
	H << "\n#include <Ice/Incoming.h>";
	H << "\n#include <Ice/Direct.h>";
    }
    else
    {
	H << "\n#include <Ice/LocalObject.h>";
	C << "\n#include <Ice/BasicStream.h>";
	C << "\n#include <Ice/Proxy.h>";
	C << "\n#include <Ice/Object.h>";
    }

    StringList includes = unit->includeFiles();
    for (StringList::const_iterator q = includes.begin(); q != includes.end(); ++q)
    {
	H << "\n#include <" << changeInclude(*q, _includePaths) << ".h>";
    }

    printVersionCheck(H);
    printVersionCheck(C);

    printDllExportStuff(H, _dllExport);
    if (_dllExport.size())
    {
	_dllExport += " ";
    }

    ProxyDeclVisitor proxyDeclVisitor(H, C, _dllExport);
    unit->visit(&proxyDeclVisitor);

    ObjectDeclVisitor objectDeclVisitor(H, C, _dllExport);
    unit->visit(&objectDeclVisitor);

    IceVisitor iceVisitor(H, C, _dllExport);
    unit->visit(&iceVisitor);

    HandleVisitor handleVisitor(H, C, _dllExport);
    unit->visit(&handleVisitor);

    TypesVisitor typesVisitor(H, C, _dllExport);
    unit->visit(&typesVisitor);

    ProxyVisitor proxyVisitor(H, C, _dllExport);
    unit->visit(&proxyVisitor);

    DelegateVisitor delegateVisitor(H, C, _dllExport);
    unit->visit(&delegateVisitor);

    DelegateMVisitor delegateMVisitor(H, C, _dllExport);
    unit->visit(&delegateMVisitor);

    DelegateDVisitor delegateDVisitor(H, C, _dllExport);
    unit->visit(&delegateDVisitor);

    ObjectVisitor objectVisitor(H, C, _dllExport);
    unit->visit(&objectVisitor);
}

Slice::Gen::TypesVisitor::TypesVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::TypesVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasOtherConstructedOrExceptions())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::TypesVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::TypesVisitor::visitClassDefStart(const ClassDefPtr&)
{
    return false;
}

bool
Slice::Gen::TypesVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    string name = p->name();
    string scoped = p->scoped();
    ExceptionPtr base = p->base();

    H << sp << nl << "class " << name << " : ";
    H.useCurrentPosAsIndent();
    if (!base)
    {
	if (p->isLocal())
	{
	    H << "public ::Ice::LocalException";
	}
	else
	{
	    H << "public ::Ice::UserException";
	}
    }
    else
    {
	H << "public " << base->scoped();
    }
    H.restoreIndent();
    H << sb;

    H.dec();
    H << nl << "public: ";
    H.inc();

    H << sp;
    if (p->isLocal())
    {
	H << nl << _dllExport << name << "(const char*, int);";
	C << sp << nl << scoped.substr(2) << "::" << name << "(const char* file, int line) : ";
	C.inc();
	if (!base)
	{
	    C << nl << "::Ice::LocalException(file, line)";
	}
	else
	{
	    C << nl << base->scoped() << "(file, line)";
	}
	C.dec();
	C << sb;
	C << eb;
    }

    H << nl << _dllExport << "virtual ::std::string ice_name() const;";
    C << sp << nl << "::std::string" << nl << scoped.substr(2) << "::ice_name() const";
    C << sb;
    C << nl << "static const ::std::string name(\"" << scoped.substr(2) << "\");";
    C << nl << "return name;";
    C << eb;
    
    if (p->isLocal())
    {
	H << nl << _dllExport << "virtual void ice_print(::std::ostream&) const;";
    }

    H << nl << _dllExport << "virtual ::Ice::Exception* ice_clone() const;";
    C << sp << nl << "::Ice::Exception*" << nl << scoped.substr(2) << "::ice_clone() const";
    C << sb;
    C << nl << "return new " << name << "(*this);";
    C << eb;

    H << nl << _dllExport << "virtual void ice_throw() const;";
    C << sp << nl << "void" << nl << scoped.substr(2) << "::ice_throw() const";
    C << sb;
    C << nl << "throw *this;";
    C << eb;
    
    if (!p->isLocal())
    {
	ExceptionList allBases = p->allBases();
	StringList exceptionIds;
	transform(allBases.begin(), allBases.end(), back_inserter(exceptionIds),
		  ::IceUtil::memFun(&Exception::scoped));
	exceptionIds.push_front(scoped);
	exceptionIds.push_back("::Ice::UserException");
	
	StringList::const_iterator q;
	
	H << sp;
	H << nl << _dllExport << "static ::std::string __exceptionIds[" << exceptionIds.size() << "];";
	H << nl << _dllExport << "virtual ::std::string* __getExceptionIds() const;";
	C << sp << nl << "::std::string " << scoped.substr(2) << "::__exceptionIds[" << exceptionIds.size() << "] =";
	C << sb;
	q = exceptionIds.begin();
	while (q != exceptionIds.end())
	{
	    C << nl << '"' << *q << '"';
	    if (++q != exceptionIds.end())
	    {
		C << ',';
	    }
	}
	C << eb << ';';
	C << sp << nl << "::std::string*" << nl << scoped.substr(2) << "::__getExceptionIds() const";
	C << sb;
	C << nl << "return __exceptionIds;";
	C << eb;
    }

    return true;
}

void
Slice::Gen::TypesVisitor::visitExceptionEnd(const ExceptionPtr& p)
{
    if (!p->isLocal())
    {
	string name = p->name();
	string scoped = p->scoped();
	
	ExceptionPtr base = p->base();
    
	H << sp << nl << _dllExport << "virtual void __write(::IceInternal::BasicStream*) const;";
	H << nl << _dllExport << "virtual void __read(::IceInternal::BasicStream*);";
	TypeStringList memberList;
	DataMemberList dataMembers = p->dataMembers();
	for (DataMemberList::const_iterator q = dataMembers.begin(); q != dataMembers.end(); ++q)
	{
	    memberList.push_back(make_pair((*q)->type(), (*q)->name()));
	}
	C << sp << nl << "void" << nl << scoped.substr(2) << "::__write(::IceInternal::BasicStream* __os) const";
	C << sb;
	writeMarshalCode(C, memberList, 0);
	if (base)
	{
	    C.zeroIndent();
	    C << nl << "#ifdef WIN32"; // COMPILERBUG
	    C.restoreIndent();
	    C << nl << base->name() << "::__write(__os);";
	    C.zeroIndent();
	    C << nl << "#else";
	    C.restoreIndent();
	    C << nl << base->scoped() << "::__write(__os);";
	    C.zeroIndent();
	    C << nl << "#endif";
	    C.restoreIndent();
	}
	C << eb;
	C << sp << nl << "void" << nl << scoped.substr(2) << "::__read(::IceInternal::BasicStream* __is)";
	C << sb;
	writeUnmarshalCode(C, memberList, 0);
	if (base)
	{
	    C.zeroIndent();
	    C << nl << "#ifdef WIN32"; // COMPILERBUG
	    C.restoreIndent();
	    C << nl << base->name() << "::__read(__is);";
	    C.zeroIndent();
	    C << nl << "#else";
	    C.restoreIndent();
	    C << nl << base->scoped() << "::__read(__is);";
	    C.zeroIndent();
	    C << nl << "#endif";
	    C.restoreIndent();
	}
	C << eb;
    }
    H << eb << ';';
}

bool
Slice::Gen::TypesVisitor::visitStructStart(const StructPtr& p)
{
    string name = p->name();

    H << sp << nl << "struct " << name;
    H << sb;

    return true;
}

void
Slice::Gen::TypesVisitor::visitStructEnd(const StructPtr& p)
{
    string name = p->name();
    string scoped = p->scoped();

    H << sp << nl << _dllExport << "void __write(::IceInternal::BasicStream*) const;"; // NOT virtual!
    H << nl << _dllExport << "void __read(::IceInternal::BasicStream*);"; // NOT virtual!
    H << nl << _dllExport << "bool operator==(const " << name << "&) const;";
    H << nl << _dllExport << "bool operator!=(const " << name << "&) const;";
    H << nl << _dllExport << "bool operator<(const " << name << "&) const;";
    H << eb << ';';
    
    TypeStringList memberList;
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList::const_iterator q;
    for (q = dataMembers.begin(); q != dataMembers.end(); ++q)
    {
	memberList.push_back(make_pair((*q)->type(), (*q)->name()));
    }
    C << sp << nl << "void" << nl << scoped.substr(2) << "::__write(::IceInternal::BasicStream* __os) const";
    C << sb;
    writeMarshalCode(C, memberList, 0);
    C << eb;
    C << sp << nl << "void" << nl << scoped.substr(2) << "::__read(::IceInternal::BasicStream* __is)";
    C << sb;
    writeUnmarshalCode(C, memberList, 0);
    C << eb;
    C << sp << nl << "bool" << nl << scoped.substr(2) << "::operator==(const " << name << "& __rhs) const";
    C << sb;
    C << nl << "return !operator!=(__rhs);";
    C << eb;
    C << sp << nl << "bool" << nl << scoped.substr(2) << "::operator!=(const " << name << "& __rhs) const";
    C << sb;
    C << nl << "if (this == &__rhs)";
    C << sb;
    C << nl << "return false;";
    C << eb;
    for (q = dataMembers.begin(); q != dataMembers.end(); ++q)
    {
	C << nl << "if (" << (*q)->name() << " != __rhs." << (*q)->name() << ')';
	C << sb;
	C << nl << "return true;";
	C << eb;
    }
    C << nl << "return false;";
    C << eb;
    C << sp << nl << "bool" << nl << scoped.substr(2) << "::operator<(const " << name << "& __rhs) const";
    C << sb;
    C << nl << "if (this == &__rhs)";
    C << sb;
    C << nl << "return false;";
    C << eb;
    for (q = dataMembers.begin(); q != dataMembers.end(); ++q)
    {
	C << nl << "if (" << (*q)->name() << " < __rhs." << (*q)->name() << ')';
	C << sb;
	C << nl << "return true;";
	C << eb;
	C << nl << "else if (__rhs." << (*q)->name() << " < " << (*q)->name() << ')';
	C << sb;
	C << nl << "return false;";
	C << eb;
    }
    C << nl << "return false;";
    C << eb;
}

void
Slice::Gen::TypesVisitor::visitDataMember(const DataMemberPtr& p)
{
    string name = p->name();
    string s = typeToString(p->type());
    H << sp << nl << s << ' ' << name << ';';
}

void
Slice::Gen::TypesVisitor::visitSequence(const SequencePtr& p)
{
    string name = p->name();
    TypePtr type = p->type();
    string s = typeToString(type);
    if (s[0] == ':')
    {
	s.insert(0, " ");
    }
    H << sp << nl << "typedef ::std::vector<" << s << "> " << name << ';';

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    if (!builtin)
    {
	string scoped = p->scoped();
	string scope = p->scope();

	H << sp << nl << "class __U__" << name << " { };";
	H << nl << _dllExport << "void __write(::IceInternal::BasicStream*, const " << name << "&, __U__" << name
	  << ");";
	H << nl << _dllExport << "void __read(::IceInternal::BasicStream*, " << name << "&, __U__" << name << ");";
	C << sp << nl << "void" << nl << scope.substr(2) << "__write(::IceInternal::BasicStream* __os, const "
	  << scoped << "& v, " << scope << "__U__" << name << ")";
	C << sb;
	C << nl << "__os->write(::Ice::Int(v.size()));";
	C << nl << scoped << "::const_iterator p;";
	C << nl << "for (p = v.begin(); p != v.end(); ++p)";
	C << sb;
	writeMarshalUnmarshalCode(C, type, "(*p)", true);
	C << eb;
	C << eb;
	C << sp << nl << "void" << nl << scope.substr(2) << "__read(::IceInternal::BasicStream* __is, " << scoped
	  << "& v, " << scope << "__U__" << name << ')';
	C << sb;
	C << nl << "::Ice::Int sz;";
	C << nl << "__is->read(sz);";
	// Don't use v.resize(sz) or v.reserve(sz) here, as it
	// cannot be checked whether sz is a reasonable value
	C << nl << "while (sz--)";
	C << sb;
	C.zeroIndent();
	C << nl << "#ifdef WIN32"; // STLBUG
	C.restoreIndent();
	C << nl << "v.push_back(" << typeToString(type) << "());";
	C.zeroIndent();
	C << nl << "#else";
	C.restoreIndent();
	C << nl << "v.push_back();";
	C.zeroIndent();
	C << nl << "#endif";
	C.restoreIndent();
	writeMarshalUnmarshalCode(C, type, "v.back()", false);
	C << eb;
	C << eb;
    }
}

void
Slice::Gen::TypesVisitor::visitDictionary(const DictionaryPtr& p)
{
    string name = p->name();
    TypePtr keyType = p->keyType();
    TypePtr valueType = p->valueType();
    string ks = typeToString(keyType);
    if (ks[0] == ':')
    {
	ks.insert(0, " ");
    }
    string vs = typeToString(valueType);
    H << sp << nl << "typedef ::std::map<" << ks << ", " << vs << "> " << name << ';';

    string scoped = p->scoped();
    string scope = p->scope();
    
    H << sp << nl << "class __U__" << name << " { };";
    H << nl << _dllExport << "void __write(::IceInternal::BasicStream*, const " << name << "&, __U__" << name << ");";
    H << nl << _dllExport << "void __read(::IceInternal::BasicStream*, " << name << "&, __U__" << name << ");";
    C << sp << nl << "void" << nl << scope.substr(2) << "__write(::IceInternal::BasicStream* __os, const " << scoped
      << "& v, " << scope << "__U__" << name << ")";
    C << sb;
    C << nl << "__os->write(::Ice::Int(v.size()));";
    C << nl << scoped << "::const_iterator p;";
    C << nl << "for (p = v.begin(); p != v.end(); ++p)";
    C << sb;
    writeMarshalUnmarshalCode(C, keyType, "p->first", true);
    writeMarshalUnmarshalCode(C, valueType, "p->second", true);
    C << eb;
    C << eb;
    C << sp << nl << "void" << nl << scope.substr(2) << "__read(::IceInternal::BasicStream* __is, " << scoped
      << "& v, " << scope << "__U__" << name << ')';
    C << sb;
    C << nl << "::Ice::Int sz;";
    C << nl << "__is->read(sz);";
    C << nl << "while (sz--)";
    C << sb;
    C << nl << "::std::pair<" << ks << ", " << vs << "> pair;";
    writeMarshalUnmarshalCode(C, keyType, "pair.first", false);
    writeMarshalUnmarshalCode(C, valueType, "pair.second", false);
    C << nl << "v.insert(v.end(), pair);";
    C << eb;
    C << eb;
}

void
Slice::Gen::TypesVisitor::visitEnum(const EnumPtr& p)
{
    string name = p->name();
    EnumeratorList enumerators = p->getEnumerators();
    H << sp << nl << "enum " << name;
    H << sb;
    EnumeratorList::const_iterator en = enumerators.begin();
    while (en != enumerators.end())
    {
	H << nl << (*en)->name();
	if (++en != enumerators.end())
	{
	    H << ',';
	}
    }
    H << eb << ';';

    string scoped = p->scoped();
    string scope = p->scope();

    int sz = enumerators.size();
    
    H << sp << nl << _dllExport << "void __write(::IceInternal::BasicStream*, " << name << ");";
    H << nl << _dllExport << "void __read(::IceInternal::BasicStream*, " << name << "&);";
    C << sp << nl << "void" << nl << scope.substr(2) << "__write(::IceInternal::BasicStream* __os, " << scoped
      << " v)";
    C << sb;
    if (sz <= 0x7f)
    {
	C << nl << "__os->write(static_cast< ::Ice::Byte>(v));";
    }
    else if (sz <= 0x7fff)
    {
	C << nl << "__os->write(static_cast< ::Ice::Short>(v));";
    }
    else if (sz <= 0x7fffffff)
    {
	C << nl << "__os->write(static_cast< ::Ice::Int>(v));";
    }
    else
    {
	C << nl << "__os->write(static_cast< ::Ice::Long>(v));";
    }
    C << eb;
    C << sp << nl << "void" << nl << scope.substr(2) << "__read(::IceInternal::BasicStream* __is, " << scoped
      << "& v)";
    C << sb;
    if (sz <= 0x7f)
    {
	C << nl << "::Ice::Byte val;";
	C << nl << "__is->read(val);";
	C << nl << "v = static_cast< " << scoped << ">(val);";
    }
    else if (sz <= 0x7fff)
    {
	C << nl << "::Ice::Short val;";
	C << nl << "__is->read(val);";
	C << nl << "v = static_cast< " << scoped << ">(val);";
    }
    else if (sz <= 0x7fffffff)
    {
	C << nl << "::Ice::Int val;";
	C << nl << "__is->read(val);";
	C << nl << "v = static_cast< " << scoped << ">(val);";
    }
    else
    {
	C << nl << "::Ice::Long val;";
	C << nl << "__is->read(val);";
	C << nl << "v = static_cast< " << scoped << ">(val);";
    }
    C << eb;
}

Slice::Gen::ProxyDeclVisitor::ProxyDeclVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::ProxyDeclVisitor::visitUnitStart(const UnitPtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    H << sp << nl << "namespace IceProxy" << nl << '{';

    return true;
}

void
Slice::Gen::ProxyDeclVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}
    
bool
Slice::Gen::ProxyDeclVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ProxyDeclVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

void
Slice::Gen::ProxyDeclVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    if (p->isLocal())
    {
	return;
    }

    string name = p->name();

    H << sp << nl << "class " << name << ';';
}

Slice::Gen::ProxyVisitor::ProxyVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::ProxyVisitor::visitUnitStart(const UnitPtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    H << sp << nl << "namespace IceProxy" << nl << '{';

    return true;
}

void
Slice::Gen::ProxyVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}
    
bool
Slice::Gen::ProxyVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ProxyVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::ProxyVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if (p->isLocal())
    {
	return false;
    }

    string name = p->name();
    string scoped = p->scoped();
    ClassList bases = p->bases();

    H << sp << nl << "class " << _dllExport << name << " : ";
    if (bases.empty())
    {
	H << "virtual public ::IceProxy::Ice::Object";
    }
    else
    {
	H.useCurrentPosAsIndent();
	ClassList::const_iterator q = bases.begin();
	while (q != bases.end())
	{
	    H << "virtual public ::IceProxy" << (*q)->scoped();
	    if (++q != bases.end())
	    {
		H << ',' << nl;
	    }
	}
	H.restoreIndent();
    }

    H << sb;
    H.dec();
    H << nl << "public: ";
    H.inc();

    return true;
}

void
Slice::Gen::ProxyVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    string scoped = p->scoped();
    
    H.dec();
    H << sp << nl << "private: ";
    H.inc();
    H << sp << nl << "virtual ::IceInternal::Handle< ::IceDelegateM::Ice::Object> __createDelegateM();";
    H << nl << "virtual ::IceInternal::Handle< ::IceDelegateD::Ice::Object> __createDelegateD();";
    H << eb << ';';
    C << sp << nl << "::IceInternal::Handle< ::IceDelegateM::Ice::Object>";
    C << nl << "IceProxy" << scoped << "::__createDelegateM()";
    C << sb;
    C << nl << "return ::IceInternal::Handle< ::IceDelegateM::Ice::Object>(new ::IceDelegateM" << scoped << ");";
    C << eb;
    C << sp << nl << "::IceInternal::Handle< ::IceDelegateD::Ice::Object>";
    C << nl << "IceProxy" << scoped << "::__createDelegateD()";
    C << sb;
    C << nl << "return ::IceInternal::Handle< ::IceDelegateD::Ice::Object>(new ::IceDelegateD" << scoped << ");";
    C << eb;
}

void
Slice::Gen::ProxyVisitor::visitOperation(const OperationPtr& p)
{
    string name = p->name();
    string scoped = p->scoped();
    string scope = p->scope();

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret);

    TypeStringList inParams = p->inputParameters();
    TypeStringList outParams = p->outputParameters();
    TypeStringList::const_iterator q;

    string params = "(";
    string paramsDecl = "("; // With declarators
    string args = "(";

    for (q = inParams.begin(); q != inParams.end(); ++q)
    {
	string typeString = inputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	args += q->second;
	params += ", ";
	paramsDecl += ", ";
	args += ", ";
    }

    for (q = outParams.begin(); q != outParams.end(); ++q)
    {
	string typeString = outputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	args += q->second;
	params += ", ";
	paramsDecl += ", ";
	args += ", ";
    }

    params += "const ::Ice::Context& = ::Ice::Context())";
    paramsDecl += "const ::Ice::Context& __context)";
    args += "__context)";

    H << sp << nl << retS << ' ' << name << params << ';';
    C << sp << nl << retS << nl << "IceProxy" << scoped << paramsDecl;
    C << sb;
    C << nl << "int __cnt = 0;";
    C << nl << "while (true)";
    C << sb;
    C << nl << "::IceInternal::Handle< ::IceDelegate::Ice::Object> __delBase = __getDelegate();";
    C << nl << "::IceDelegate" << scope.substr(0, scope.size() - 2) << "* __del = dynamic_cast< ::IceDelegate"
      << scope.substr(0, scope.size() - 2) << "*>(__delBase.get());";
    C << nl << "try";
    C << sb;
    C << nl;
    if (ret)
    {
	C << "return ";
    }
    C << "__del->" << name << args << ";";
    if (!ret)
    {
	C << nl << "return;";
    }
    C << eb;
    C << nl << "catch (const ::Ice::LocationForward& __ex)";
    C << sb;
    C << nl << "__locationForward(__ex);";
    C << eb;
    C << nl << "catch (const ::IceInternal::NonRepeatable& __ex)";
    C << sb;
    if (p->nonmutating())
    {
	C << nl << "__handleException(*__ex.get(), __cnt);";
    }
    else
    {
	C << nl << "__rethrowException(*__ex.get());";
    }
    C << eb;
    C << nl << "catch (const ::Ice::LocalException& __ex)";
    C << sb;
    C << nl << "__handleException(__ex, __cnt);";
    C << eb;
    C << eb;
    C << eb;
}

Slice::Gen::DelegateVisitor::DelegateVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::DelegateVisitor::visitUnitStart(const UnitPtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    H << sp << nl << "namespace IceDelegate" << nl << '{';

    return true;
}

void
Slice::Gen::DelegateVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}
    
bool
Slice::Gen::DelegateVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::DelegateVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::DelegateVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if (p->isLocal())
    {
	return false;
    }

    string name = p->name();
    ClassList bases = p->bases();

    H << sp << nl << "class " << _dllExport << name << " : ";
    if (bases.empty())
    {
	H << "virtual public ::IceDelegate::Ice::Object";
    }
    else
    {
	H.useCurrentPosAsIndent();
	ClassList::const_iterator q = bases.begin();
	while (q != bases.end())
	{
	    H << "virtual public ::IceDelegate" << (*q)->scoped();
	    if (++q != bases.end())
	    {
		H << ',' << nl;
	    }
	}
	H.restoreIndent();
    }
    H << sb;
    H.dec();
    H << nl << "public: ";
    H.inc();

    return true;
}

void
Slice::Gen::DelegateVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    H << eb << ';';
}

void
Slice::Gen::DelegateVisitor::visitOperation(const OperationPtr& p)
{
    string name = p->name();

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret);

    TypeStringList inParams = p->inputParameters();
    TypeStringList outParams = p->outputParameters();
    TypeStringList::const_iterator q;

    string params = "(";

    for (q = inParams.begin(); q != inParams.end(); ++q)
    {
	string typeString = inputTypeToString(q->first);
	params += typeString;
	params += ", ";
    }

    for (q = outParams.begin(); q != outParams.end(); ++q)
    {
	string typeString = outputTypeToString(q->first);
	params += typeString;
	params += ", ";
    }

    params += "const ::Ice::Context&)";
    
    H << sp << nl << "virtual " << retS << ' ' << name << params << " = 0;";
}

Slice::Gen::DelegateMVisitor::DelegateMVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::DelegateMVisitor::visitUnitStart(const UnitPtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    H << sp << nl << "namespace IceDelegateM" << nl << '{';

    return true;
}

void
Slice::Gen::DelegateMVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}
    
bool
Slice::Gen::DelegateMVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::DelegateMVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::DelegateMVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if (p->isLocal())
    {
	return false;
    }

    string name = p->name();
    string scoped = p->scoped();
    ClassList bases = p->bases();

    H << sp << nl << "class " << _dllExport << name << " : ";
    H.useCurrentPosAsIndent();
    H << "virtual public ::IceDelegate" << scoped << ',';
    if (bases.empty())
    {
	H << nl << "virtual public ::IceDelegateM::Ice::Object";
    }
    else
    {
	ClassList::const_iterator q = bases.begin();
	while (q != bases.end())
	{
	    H << nl << "virtual public ::IceDelegateM" << (*q)->scoped();
	    if (++q != bases.end())
	    {
		H << ',';
	    }
	}
    }
    H.restoreIndent();
    H << sb;
    H.dec();
    H << nl << "public: ";
    H.inc();

    return true;
}

void
Slice::Gen::DelegateMVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    H << eb << ';';
}

void
Slice::Gen::DelegateMVisitor::visitOperation(const OperationPtr& p)
{
    string name = p->name();
    string scoped = p->scoped();

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret);

    TypeStringList inParams = p->inputParameters();
    TypeStringList outParams = p->outputParameters();
    TypeStringList::const_iterator q;

    string params = "(";
    string paramsDecl = "("; // With declarators

    for (q = inParams.begin(); q != inParams.end(); ++q)
    {
	string typeString = inputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	params += ", ";
	paramsDecl += ", ";
    }

    for (q = outParams.begin(); q != outParams.end(); ++q)
    {
	string typeString = outputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	params += ", ";
	paramsDecl += ", ";
    }

    params += "const ::Ice::Context&)";
    paramsDecl += "const ::Ice::Context& __context)";
    
    ExceptionList throws = p->throws();
    throws.sort();
    throws.unique();

    H << sp << nl << "virtual " << retS << ' ' << name << params << ';';
    C << sp << nl << retS << nl << "IceDelegateM" << scoped << paramsDecl;
    C << sb;
    C << nl << "bool __sendProxy = false;";
    C << nl << "while (true)";
    C << sb;
    C << nl << "try";
    C << sb;
    C << nl << "static const ::std::string __operation(\"" << name << "\");";
    C << nl << "::IceInternal::Outgoing __out(__emitter, __reference, __sendProxy, __operation, "
      << (p->nonmutating() ? "true" : "false") << ", __context);";
    if (ret || !outParams.empty() || !throws.empty())
    {
	C << nl << "::IceInternal::BasicStream* __is = __out.is();";
    }
    if (!inParams.empty())
    {
	C << nl << "::IceInternal::BasicStream* __os = __out.os();";
    }
    writeMarshalCode(C, inParams, 0);
    C << nl << "if (!__out.invoke())";
    C << sb;
    if (!throws.empty())
    {
	ExceptionList::const_iterator r;
	C << nl << "static ::std::string __throws[] =";
	C << sb;
	for (r = throws.begin(); r != throws.end(); ++r)
	{
	    C << nl << '"' << (*r)->scoped() << '"';
	    if (r != throws.end())
	    {
		C << ',';
	    }
	}
	C << eb << ';';
	C << nl << "switch (__is->throwException(__throws, __throws + " << throws.size() << "))";
	C << sb;
	int cnt = 0;
	for (r = throws.begin(); r != throws.end(); ++r)
	{
	    C << nl << "case " << cnt++ << ':';
	    C << sb;
	    C << nl << (*r)->scoped() << " __ex;";
	    C << nl << "__ex.__read(__is);";
	    C << nl << "throw __ex;";
	    C << eb;
	}
	C << eb;
	C << nl << "throw ::Ice::UnknownUserException(__FILE__, __LINE__);";
    }
    else
    {
	C << nl << "throw ::Ice::UnknownUserException(__FILE__, __LINE__);";
    }
    C << eb;
    writeAllocateCode(C, TypeStringList(), ret);
    writeUnmarshalCode(C, outParams, ret);
    if (ret)
    {
	C << nl << "return __ret;";
    }
    else
    {
	C << nl << "return;";
    }
    C << eb;
    C << nl << "catch (const ::Ice::ProxyRequested&)";
    C << sb;
    C << nl << "__sendProxy = true;";
    C << eb;
    C << eb;
    C << eb;
}

Slice::Gen::DelegateDVisitor::DelegateDVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::DelegateDVisitor::visitUnitStart(const UnitPtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    H << sp << nl << "namespace IceDelegateD" << nl << '{';

    return true;
}

void
Slice::Gen::DelegateDVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}
    
bool
Slice::Gen::DelegateDVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasProxies())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::DelegateDVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::DelegateDVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if (p->isLocal())
    {
	return false;
    }

    string name = p->name();
    string scoped = p->scoped();
    ClassList bases = p->bases();

    H << sp << nl << "class " << _dllExport << name << " : ";
    H.useCurrentPosAsIndent();
    H << "virtual public ::IceDelegate" << scoped << ',';
    if (bases.empty())
    {
	H << nl << "virtual public ::IceDelegateD::Ice::Object";
    }
    else
    {
	ClassList::const_iterator q = bases.begin();
	while (q != bases.end())
	{
	    H << nl << "virtual public ::IceDelegateD" << (*q)->scoped();
	    if (++q != bases.end())
	    {
		H << ',';
	    }
	}
    }
    H.restoreIndent();
    H << sb;
    H.dec();
    H << nl << "public: ";
    H.inc();

    return true;
}

void
Slice::Gen::DelegateDVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    H << eb << ';';
}

void
Slice::Gen::DelegateDVisitor::visitOperation(const OperationPtr& p)
{
    ContainerPtr container = p->container();
    ClassDefPtr cl = ClassDefPtr::dynamicCast(container);
    
    string name = p->name();
    string scoped = p->scoped();

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret);

    TypeStringList inParams = p->inputParameters();
    TypeStringList outParams = p->outputParameters();
    TypeStringList::const_iterator q;

    string params = "(";
    string paramsDecl = "("; // With declarators
    string args = "(";

    for (q = inParams.begin(); q != inParams.end(); ++q)
    {
	string typeString = inputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	args += q->second;
	params += ", ";
	paramsDecl += ", ";
	args += ", ";
    }

    for (q = outParams.begin(); q != outParams.end(); ++q)
    {
	string typeString = outputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	args += q->second;
	params += ", ";
	paramsDecl += ", ";
	args += ", ";
    }

    params += "const ::Ice::Context& = ::Ice::Context())";
    paramsDecl += "const ::Ice::Context& __context)";
    args += "__current)";
    
    H << sp << nl << "virtual " << retS << ' ' << name << params << ';';
    C << sp << nl << retS << nl << "IceDelegateD" << scoped << paramsDecl;
    C << sb;
    C << nl << "::Ice::Current __current;";
    C << nl << "__initCurrent(__current, \"" << name << "\", " << (p->nonmutating() ? "true" : "false")
      << ", __context);";
    C << nl << "while (true)";
    C << sb;
    C << nl << "::IceInternal::Direct __direct(__adapter, __current);";
    C << nl << cl->scoped() << "* __servant = dynamic_cast< " << cl->scoped() << "*>(__direct.facetServant().get());";
    C << nl << "if (!__servant)";
    C << sb;
    C << nl << "throw ::Ice::OperationNotExistException(__FILE__, __LINE__);";
    C << eb;
    C << nl << "try";
    C << sb;
    C << nl;
    if (ret)
    {
	C << "return ";
    }
    C << "__servant->" << name << args << ';';
    if (!ret)
    {
	C << nl << "return;";
    }
    C << eb;
    ExceptionList throws = p->throws();
    throws.sort();
    throws.unique();
    ExceptionList::const_iterator r;
    for (r = throws.begin(); r != throws.end(); ++r)
    {
	C << nl << "catch (const " << (*r)->scoped() << "&)";
	C << sb;
	C << nl << "throw;";
	C << eb;
    }
    C << nl << "catch (const ::Ice::ProxyRequested&)";
    C << sb;
    C << nl << "__initCurrentProxy(__current);";
    C << eb;
    C << nl << "catch (const ::Ice::LocalException&)";
    C << sb;
    C << nl << "throw ::Ice::UnknownLocalException(__FILE__, __LINE__);";
    C << eb;
    C << nl << "catch (const ::Ice::UserException&)";
    C << sb;
    C << nl << "throw ::Ice::UnknownUserException(__FILE__, __LINE__);";
    C << eb;
    C << nl << "catch (...)";
    C << sb;
    C << nl << "throw ::Ice::UnknownException(__FILE__, __LINE__);";
    C << eb;
    C << eb;
    C << eb;
}

Slice::Gen::ObjectDeclVisitor::ObjectDeclVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::ObjectDeclVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasClassDecls())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ObjectDeclVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

void
Slice::Gen::ObjectDeclVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    string name = p->name();
    
    H << sp << nl << "class " << name << ';';
}

Slice::Gen::ObjectVisitor::ObjectVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::ObjectVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasClassDefs())
    {
	return false;
    }

    string name = p->name();
    
    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ObjectVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp;
    H << nl << '}';
}

bool
Slice::Gen::ObjectVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string name = p->name();
    string scoped = p->scoped();
    ClassList bases = p->bases();
    
    string exp1;
    string exp2;
    if (_dllExport.size())
    {
	if (p->hasDataMembers())
	{
	    exp2 = _dllExport;
	}
	else
	{
	    exp1 = _dllExport;
	}
    }

    H << sp;
    H << nl << "class " << exp1 << name << " : ";
    H.useCurrentPosAsIndent();
    if (bases.empty())
    {
	if (p->isLocal())
	{
	    H << "virtual public ::Ice::LocalObject";
	}
	else
	{
	    H << "virtual public ::Ice::Object";
	}
    }
    else
    {
	ClassList::const_iterator q = bases.begin();
	while (q != bases.end())
	{
	    H << "virtual public " << (*q)->scoped();
	    if (++q != bases.end())
	    {
		H << ',' << nl;
	    }
	}
    }
    H.restoreIndent();
    H << sb;
    H.dec();
    H << nl << "public: ";
    H.inc();

    if (!p->isLocal())
    {
	ClassList allBases = p->allBases();
	StringList ids;
	transform(allBases.begin(), allBases.end(), back_inserter(ids), ::IceUtil::memFun(&ClassDef::scoped));
	StringList other;
	other.push_back(scoped);
	other.push_back("::Ice::Object");
	other.sort();
	ids.merge(other);
	ids.unique();
	
	ClassList allBaseClasses;
	ClassDefPtr cl;
	if (!bases.empty())
	{
	    cl = bases.front();
	}
	else
	{
	    cl = 0;
	}
	while (cl && !cl->isInterface())
	{
	    allBaseClasses.push_back(cl);
	    ClassList baseBases = cl->bases();
	    if (!baseBases.empty())
	    {
		cl = baseBases.front();
	    }
	    else
	    {
		cl = 0;
	    }
	}
	StringList classIds;
	transform(allBaseClasses.begin(), allBaseClasses.end(), back_inserter(classIds),
		  ::IceUtil::memFun(&ClassDef::scoped));
	if (!p->isInterface())
	{
	    classIds.push_front(scoped);
	}
	classIds.push_back("::Ice::Object");

	StringList::const_iterator q;

	H << sp;
	H << nl << exp2 << "static ::std::string __ids[" << ids.size() << "];";
	H << nl << exp2 << "static ::std::string __classIds[" << classIds.size() << "];";
	H << nl << exp2 << "virtual bool ice_isA(const ::std::string&, const ::Ice::Current& = ::Ice::Current());";
	H << nl << exp2 << "virtual ::std::string* __getClassIds();";
	C << sp;
	C << nl << "::std::string " << scoped.substr(2) << "::__ids[" << ids.size() << "] =";
	C << sb;
	q = ids.begin();
	while (q != ids.end())
	{
	    C << nl << '"' << *q << '"';
	    if (++q != ids.end())
	    {
		C << ',';
	    }
	}
	C << eb << ';';
	C << sp;
	C << nl << "::std::string " << scoped.substr(2) << "::__classIds[" << classIds.size() << "] =";
	C << sb;
	q = classIds.begin();
	while (q != classIds.end())
	{
	    C << nl << '"' << *q << '"';
	    if (++q != classIds.end())
	    {
		C << ',';
	    }
	}
	C << eb << ';';
	C << sp;
	C << nl << "bool" << nl << scoped.substr(2) << "::ice_isA(const ::std::string& s, const ::Ice::Current&)";
	C << sb;
	C << nl << "::std::string* b = __ids;";
	C << nl << "::std::string* e = __ids + " << ids.size() << ';';
	C << nl << "return ::std::binary_search(b, e, s);";
	C << eb;
	C << sp;
	C << nl << "::std::string*" << nl << scoped.substr(2) << "::__getClassIds()";
	C << sb;
	C << nl << "return __classIds;";
	C << eb;
    }

    return true;
}
    
void
Slice::Gen::ObjectVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    if (!p->isLocal())
    {
	string name = p->name();
	string scoped = p->scoped();
	
	ClassList bases = p->bases();
	ClassDefPtr base;
	if (!bases.empty() && !bases.front()->isInterface())
	{
	    base = bases.front();
	}
    
	string exp2;
	if (_dllExport.size())
	{
	    if (p->hasDataMembers())
	    {
		exp2 = _dllExport;
	    }
	}

	OperationList allOps = p->allOperations();
	if (!allOps.empty())
	{
	    StringList allOpNames;
	    transform(allOps.begin(), allOps.end(), back_inserter(allOpNames), ::IceUtil::memFun(&Operation::name));
	    allOpNames.push_back("ice_isA");
	    allOpNames.push_back("ice_ping");
	    allOpNames.sort();
	    allOpNames.unique();

	    StringList::const_iterator q;
	    
	    H << sp;
	    H << nl << exp2 << "static ::std::string __all[" << allOpNames.size() << "];";
	    H << nl << exp2
	      << "virtual ::IceInternal::DispatchStatus __dispatch(::IceInternal::Incoming&, const ::Ice::Current&);";
	    C << sp;
	    C << nl << "::std::string " << scoped.substr(2) << "::__all[] =";
	    C << sb;
	    q = allOpNames.begin();
	    while (q != allOpNames.end())
	    {
		C << nl << '"' << *q << '"';
		if (++q != allOpNames.end())
		{
		    C << ',';
		}
	    }
	    C << eb << ';';
	    C << sp;
	    C << nl << "::IceInternal::DispatchStatus" << nl << scoped.substr(2)
	      << "::__dispatch(::IceInternal::Incoming& in, const ::Ice::Current& current)";
	    C << sb;
	    C << nl << "::std::pair< ::std::string*, ::std::string*> r = "
	      << "::std::equal_range(__all, __all + " << allOpNames.size() << ", current.operation);";
	    C << nl << "if (r.first == r.second)";
	    C << sb;
	    C << nl << "return ::IceInternal::DispatchOperationNotExist;";
	    C << eb;
	    C << sp;
	    C << nl << "switch (r.first - __all)";
	    C << sb;
	    int i = 0;
	    for (q = allOpNames.begin(); q != allOpNames.end(); ++q)
	    {
		C << nl << "case " << i++ << ':';
		C << sb;
		C << nl << "return ___" << *q << "(in, current);";
		C << eb;
	    }
	    C << eb;
	    C << sp;
	    C << nl << "assert(false);";
	    C << nl << "return ::IceInternal::DispatchOperationNotExist;";
	    C << eb;
	}
	H << sp;
	H << nl << exp2 << "virtual void __write(::IceInternal::BasicStream*) const;";
	H << nl << exp2 << "virtual void __read(::IceInternal::BasicStream*);";
	TypeStringList memberList;
	DataMemberList dataMembers = p->dataMembers();
	for (DataMemberList::const_iterator q = dataMembers.begin(); q != dataMembers.end(); ++q)
	{
	    memberList.push_back(make_pair((*q)->type(), (*q)->name()));
	}
	C << sp;
	C << nl << "void" << nl << scoped.substr(2) << "::__write(::IceInternal::BasicStream* __os) const";
	C << sb;
	writeMarshalCode(C, memberList, 0);
	if (base)
	{
	    C.zeroIndent();
	    C << nl << "#ifdef WIN32"; // COMPILERBUG
	    C.restoreIndent();
	    C << nl << base->name() << "::__write(__os);";
	    C.zeroIndent();
	    C << nl << "#else";
	    C.restoreIndent();
	    C << nl << base->scoped() << "::__write(__os);";
	    C.zeroIndent();
	    C << nl << "#endif";
	    C.restoreIndent();
	}
	else
	{
	    C.zeroIndent();
	    C << nl << "#ifdef WIN32"; // COMPILERBUG
	    C.restoreIndent();
	    C << nl << "Object::__write(__os);";
	    C.zeroIndent();
	    C << nl << "#else";
	    C.restoreIndent();
	    C << nl << "::Ice::Object::__write(__os);";
	    C.zeroIndent();
	    C << nl << "#endif";
	    C.restoreIndent();
	}
	C << eb;
	C << sp;
	C << nl << "void" << nl << scoped.substr(2) << "::__read(::IceInternal::BasicStream* __is)";
	C << sb;
	writeUnmarshalCode(C, memberList, 0);
	if (base)
	{
	    C.zeroIndent();
	    C << nl << "#ifdef WIN32"; // COMPILERBUG
	    C.restoreIndent();
	    C << nl << base->name() << "::__read(__is);";
	    C.zeroIndent();
	    C << nl << "#else";
	    C.restoreIndent();
	    C << nl << base->scoped() << "::__read(__is);";
	    C.zeroIndent();
	    C << nl << "#endif";
	    C.restoreIndent();
	}
	else
	{
	    C.zeroIndent();
	    C << nl << "#ifdef WIN32"; // COMPILERBUG
	    C.restoreIndent();
	    C << nl << "Object::__read(__is);";
	    C.zeroIndent();
	    C << nl << "#else";
	    C.restoreIndent();
	    C << nl << "::Ice::Object::__read(__is);";
	    C.zeroIndent();
	    C << nl << "#endif";
	    C.restoreIndent();
	}
	C << eb;
    }
    H << eb << ';';
}

bool
Slice::Gen::ObjectVisitor::visitExceptionStart(const ExceptionPtr&)
{
    return false;
}

bool
Slice::Gen::ObjectVisitor::visitStructStart(const StructPtr&)
{
    return false;
}

void
Slice::Gen::ObjectVisitor::visitOperation(const OperationPtr& p)
{
    ContainerPtr container = p->container();
    ClassDefPtr cl = ClassDefPtr::dynamicCast(container);
    string name = p->name();
    string scoped = p->scoped();
    string scope = p->scope();

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret);

    TypeStringList inParams = p->inputParameters();
    TypeStringList outParams = p->outputParameters();
    TypeStringList::const_iterator q;

    string params = "(";
    string paramsDecl = "("; // With declarators
    string args = "(";

    for (q = inParams.begin(); q != inParams.end(); ++q)
    {
	if (q != inParams.begin())
	{
	    params += ", ";
	    paramsDecl += ", ";
	    args += ", ";
	}

	string typeString = inputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	args += q->second;
    }

    for (q = outParams.begin(); q != outParams.end(); ++q)
    {
	if (q != outParams.begin() || !inParams.empty())
	{
	    params += ", ";
	    paramsDecl += ", ";
	    args += ", ";
	}

	string typeString = outputTypeToString(q->first);
	params += typeString;
	paramsDecl += typeString;
	paramsDecl += ' ';
	paramsDecl += q->second;
	args += q->second;
    }

    if (!cl->isLocal())
    {
	if (!inParams.empty() || !outParams.empty())
	{
	    params += ", ";
	    paramsDecl += ", ";
	    args += ", ";
	}
	params += "const ::Ice::Current& = ::Ice::Current())";
	paramsDecl += "const ::Ice::Current& __current)";
	args += "__current)";
    }
    else
    {
	params += ')';
	paramsDecl += ')';
	args += ')';
    }
    
    string exp2;
    if (_dllExport.size())
    {
	if (cl->hasDataMembers())
	{
	    exp2 = _dllExport;
	}
    }

    H << sp;
    H << nl << exp2 << "virtual " << retS << ' ' << name << params << " = 0;";

    if (!cl->isLocal())
    {
	ExceptionList throws = p->throws();
	throws.sort();
	throws.unique();

	H << nl << exp2 << "::IceInternal::DispatchStatus ___" << name
	  << "(::IceInternal::Incoming&, const ::Ice::Current&);";
	C << sp;
	C << nl << "::IceInternal::DispatchStatus" << nl << scope.substr(2) << "___" << name
	  << "(::IceInternal::Incoming& __in, const ::Ice::Current& __current)";
	C << sb;
	if (!inParams.empty())
	{
	    C << nl << "::IceInternal::BasicStream* __is = __in.is();";
	}
	if (ret || !outParams.empty() || !throws.empty())
	{
	    C << nl << "::IceInternal::BasicStream* __os = __in.os();";
	}
	writeAllocateCode(C, inParams, 0);
	writeUnmarshalCode(C, inParams, 0);
	writeAllocateCode(C, outParams, 0);
	if (!throws.empty())
	{
	    C << nl << "try";
	    C << sb;
	}
	C << nl;
	if (ret)
	{
	    C << retS << " __ret = ";
	}
	C << name << args << ';';
	writeMarshalCode(C, outParams, ret);
	if (!throws.empty())
	{
	    C << eb;
	    ExceptionList::const_iterator r;
	    for (r = throws.begin(); r != throws.end(); ++r)
	    {
		C << nl << "catch (const " << (*r)->scoped() << "& __ex)";
		C << sb;
		C << nl << "__os->write(__ex);";
		C << nl << "return ::IceInternal::DispatchUserException;";
		C << eb;
	    }
	}
	C << nl << "return ::IceInternal::DispatchOK;";
	C << eb;
    }	
}

void
Slice::Gen::ObjectVisitor::visitDataMember(const DataMemberPtr& p)
{
    string name = p->name();
    string s = typeToString(p->type());
    H << sp;
    H << nl << s << ' ' << name << ';';
}

Slice::Gen::IceVisitor::IceVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::IceVisitor::visitUnitStart(const UnitPtr& p)
{
    if (!p->hasClassDecls())
    {
	return false;
    }

    H << sp;
    H << nl << "namespace IceInternal" << nl << '{';

    return true;
}

void
Slice::Gen::IceVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp;
    H << nl << '}';
}
    
void
Slice::Gen::IceVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    string scoped = p->scoped();
    
    H << sp;
    H << nl << _dllExport << "void incRef(" << scoped << "*);";
    H << nl << _dllExport << "void decRef(" << scoped << "*);";
    if (!p->isLocal())
    {
	H << sp;
	H << nl << _dllExport << "void incRef(::IceProxy" << scoped << "*);";
	H << nl << _dllExport << "void decRef(::IceProxy" << scoped << "*);";
	H << sp;
	H << nl << _dllExport << "void checkedCast(const ::Ice::ObjectPrx&, const ::std::string&, "
	  << "ProxyHandle< ::IceProxy" << scoped << ">&);";
	H << nl << _dllExport << "void uncheckedCast(const ::Ice::ObjectPrx&, const ::std::string&, "
	  << "ProxyHandle< ::IceProxy" << scoped << ">&);";
    }
}

bool
Slice::Gen::IceVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string scoped = p->scoped();
    
    C << sp;
    C << nl << "void" << nl << "IceInternal::incRef(" << scoped << "* p)";
    C << sb;
    C << nl << "p->__incRef();";
    C << eb;
    C << nl << "void" << nl << "IceInternal::decRef(" << scoped << "* p)";
    C << sb;
    C << nl << "p->__decRef();";
    C << eb;

    if (!p->isLocal())
    {
	C << sp;
	C << nl << "void" << nl << "IceInternal::incRef(::IceProxy" << scoped << "* p)";
	C << sb;
	C << nl << "p->__incRef();";
	C << eb;
	C << nl << "void" << nl << "IceInternal::decRef(::IceProxy" << scoped << "* p)";
	C << sb;
	C << nl << "p->__decRef();";
	C << eb;
	C << sp;
	C << nl << "void" << nl << "IceInternal::checkedCast(const ::Ice::ObjectPrx& b, const ::std::string& f, "
	  << scoped << "Prx& d)";
	C << sb;
	C << nl << "d = 0;";
	C << nl << "if (b)";
	C << sb;
	C << nl << "if (f == b->ice_getFacet())";
	C << sb;
	C << nl << "d = dynamic_cast< ::IceProxy" << scoped << "*>(b.get());";
	C << nl << "if (!d && b->ice_isA(\"" << scoped << "\"))";
	C << sb;
	C << nl << "d = new ::IceProxy" << scoped << ";";
	C << nl << "d->__copyFrom(b);";
	C << eb;
	C << eb;
	C << nl << "else";
	C << sb;
	C << nl << "::Ice::ObjectPrx bb = b->ice_newFacet(f);";
	C << nl << "try";
	C << sb;
	C << nl << "if (bb->ice_isA(\"" << scoped << "\"))";
	C << sb;
	C << nl << "d = new ::IceProxy" << scoped << ";";
	C << nl << "d->__copyFrom(bb);";
	C << eb;
	C << eb;
	C << nl << "catch (const ::Ice::FacetNotExistException&)";
	C << sb;
	C << eb;
	C << eb;
	C << eb;
	C << eb;
	C << sp;
	C << nl << "void" << nl << "IceInternal::uncheckedCast(const ::Ice::ObjectPrx& b, const ::std::string& f, "
	  << scoped << "Prx& d)";
	C << sb;
	C << nl << "d = 0;";
	C << nl << "if (b)";
	C << sb;
	C << nl << "if (f == b->ice_getFacet())";
	C << sb;
	C << nl << "d = dynamic_cast< ::IceProxy" << scoped << "*>(b.get());";
	C << nl << "if (!d)";
	C << sb;
	C << nl << "d = new ::IceProxy" << scoped << ";";
	C << nl << "d->__copyFrom(b);";
	C << eb;
	C << eb;
	C << nl << "else";
	C << sb;
	C << nl << "::Ice::ObjectPrx bb = b->ice_newFacet(f);";
	C << nl << "d = new ::IceProxy" << scoped << ";";
	C << nl << "d->__copyFrom(bb);";
	C << eb;
	C << eb;
	C << eb;
    }

    return true;
}

Slice::Gen::HandleVisitor::HandleVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::HandleVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasClassDecls())
    {
	return false;
    }

    string name = p->name();
    
    H << sp;
    H << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::HandleVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp;
    H << nl << '}';
}

void
Slice::Gen::HandleVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    string name = p->name();
    string scoped = p->scoped();
    
    H << sp;
    H << nl << "typedef ::IceInternal::Handle< " << scoped << "> " << name << "Ptr;";
    if (!p->isLocal())
    {
	H << nl << "typedef ::IceInternal::ProxyHandle< ::IceProxy" << scoped << "> " << name << "Prx;";
	H << sp;
	H << nl << _dllExport << "void __write(::IceInternal::BasicStream*, const " << name << "Prx&);";
	H << nl << _dllExport << "void __read(::IceInternal::BasicStream*, " << name << "Prx&);";
    }
}

bool
Slice::Gen::HandleVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if (p->isLocal())
    {
	return false;
    }

    string scoped = p->scoped();
    string scope = p->scope();

    C << sp;
    C << nl << "void" << nl << scope.substr(2) << "__write(::IceInternal::BasicStream* __os, const " << scoped
      << "Prx& v)";
    C << sb;
    C << nl << "__os->write(::Ice::ObjectPrx(v));";
    C << eb;
    C << sp;
    C << nl << "void" << nl << scope.substr(2) << "__read(::IceInternal::BasicStream* __is, " << scoped << "Prx& v)";
    C << sb;
    C << nl << "::Ice::ObjectPrx proxy;";
    C << nl << "__is->read(proxy);";
    C << nl << "if (!proxy)";
    C << sb;
    C << nl << "v = 0;";
    C << eb;
    C << nl << "else";
    C << sb;
    C << nl << "v = new ::IceProxy" << scoped << ';';
    C << nl << "v->__copyFrom(proxy);";
    C << eb;
    C << eb;

    return true;
}
