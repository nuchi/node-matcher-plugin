# NodeMatcherPlugin: clang-query in plugin form

This provides the functionality of clang-query in the form of a clang plugin, so you don't need to rewrite your clang-query scripts in C++. For example, given:

```cpp
// test.cpp
int foo() {
    int badName = 1;
    return badName;
}

int main() { return foo(); }
```

```
# test.query
let Diagnostic "found a call to a function with a terribly named variable: '%1'"
let DiagnosticArgs "call var"

m callExpr(
  callee(
    decl(
      hasDescendant(
        varDecl(
          hasName("badName")
        ).bind("var")
      )
    )
  )
).bind("call")
```

Instead of
```
% clang-query -f test.query test.cpp

Match #1:

/Users/haggai/clang-query-plugin/test.cpp:6:21: note: "call" binds here
    6 | int main() { return foo(); }
      |                     ^~~~~
/Users/haggai/clang-query-plugin/test.cpp:6:21: note: "root" binds here
    6 | int main() { return foo(); }
      |                     ^~~~~
/Users/haggai/clang-query-plugin/test.cpp:2:5: note: "var" binds here
    2 |     int badName = 1;
      |     ^~~~~~~~~~~~~~~
1 match.
```

you can do

```
% clang++ test.cpp \
    -fplugin=./NodeMatcherPlugin.dylib \
    -fplugin-arg-NodeMatcherPlugin-./test.query
test.cpp:6:21: warning: found a call to a function with a terribly named variable: 'int badName = 1'
    6 | int main() { return foo(); }
      |                     ^~~~~
1 warning generated.
```

## Building

Just `make` at the repo root will work. It uses cmake so you can include it in other cmake-based projects.

## Usage

You can use your existing clang-query scripts with some small modification: add two `let` statements before each match, with the names `Diagnostic` and `DiagnosticArgs`. The former is the warning message, and the latter is a space-separated list of bound nodes which will be interpolated into the warning.
```
let Diagnostic "found a call to a function with a terribly named variable: '%1'"
let DiagnosticArgs "call var"
```
The zeroth bound node (in this example, the one bound with "call") will be the node whose source range is shown.

## Why?

Suppose you've just written a clang-query script, and now you want to add it in production as a warning. Without this plugin, you'd have to rewrite it as a C++ plugin using the clang C++ AST matcher api, and then build that plugin alongside your codebase, and then load that plugin. But now, you can just build this plugin once. You don't have to write any C++ to translate your clang-query scripts, you can just use them as is.
