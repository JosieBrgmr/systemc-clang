#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include "FindClassDataMembers.h"
#include "clang/AST/DeclCXX.h"
#include <map>
#include <string>

namespace scpar {

using namespace clang;
using namespace std;

class Sockets {

public:
  Sockets();
  Sockets(const string &, const string &, vector<string>);

  Sockets(const Sockets &);

  ~Sockets();

  string getName();
  string getSocketType();
  vector<string> getRegisterCallbackMethods();

  void dump(raw_ostream &, int);

private:
  string _name;
  string _socketType;
  vector<string> _registerCallbackMethods;
};
} // namespace scpar
#endif
