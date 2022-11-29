# xse-address-lib-dumper
Dumps binary SKSE/F4SE address libraries to text

## Usage:
```bash
xse-address-lib-dumper.exe [OPTIONS] database.bin [output]

Positionals:
  database.bin TEXT REQUIRED  The database to dump
  output TEXT                 Output file (defaults to <database>.txt)

Options:
  -h,--help                   Print this help message and exit
  -o,--output TEXT            Output file (defaults to <database>.txt)
  -s,--skyrim                 Set for a Skyrim database
  -f,--fallout4               Set for a Fallout 4 database (one of the two required)
  -b,--base                   Add executable base address to addresses
```

## Compiling:

Use Visual Studio 2022.