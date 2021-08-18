# This is Scanner++ Library

The idea of this project is to have (Java) Scanner functionalities on C++.

This project was ported from SourceForge: https://sourceforge.net/projects/scannerpp/

Current version: 0.98-alpha

## How to use

Example 1 (default separators):
```c++
#include "Scanner.h"

using namespace scannerpp;

int main() {
    Scanner scanner("Hello World");
    scanner.next(); // returns "Hello"
    scanner.next(); // returns "World"
    return 0;
}
```

Example 2 (other separators):
```c++
#include "Scanner.h"

using namespace scannerpp;

int main() {
    Scanner scanner("Hello;World");
    scanner.useSeparators(" ;\n\t"); // using semi-colon as separator too
    scanner.next(); // returns "Hello"
    scanner.next(); // returns "World"
    return 0;
}
```


LGPLv3 License

Copyleft, OptFrame group

2008-2019
