#include "f4db.h"
#include "skyrimdb.h"
#include <iostream>
#include <CLI/CLI.hpp>
#include <CLI/Error.hpp>
int main(int argc, char *argv[])
{
    CLI::App app("XSE Address Library Dumper");
    std::string filename = "version-1-10-163-0.bin";
    bool skyrim = false;
    bool fallout4 = false;
    std::string output_filename = "";
    bool use_base = false;
    app.add_option("database.bin", filename, "The database to dump")->required(true);
    app.add_option("output,-o,--output", output_filename, "Output file (defaults to <database>.txt)");
    app.add_flag("-s,--skyrim", skyrim, "Set for a Skyrim database");
    app.add_flag("-f,--fallout4", fallout4, "Set for a Fallout 4 database (one of the two required)");
    app.add_flag("-b,--base", use_base, "Add executable base address to addresses");
    try {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e){
        std::cerr << e.what() << "\n";
        std::cerr << app.help() << std::flush;
        return -1;
    }
    if (!(skyrim || fallout4))
    {
        std::cerr << "Must set either --skyrim or --fallout4" << "\n";
        std::cerr << app.help() << std::flush;
        return -1;
    }
    try {
        if (output_filename.empty()) {
            output_filename = filename;
            output_filename = output_filename.replace(output_filename.size() - 3, 3, "txt");
        }
        std::cout << "Dumping " << filename << " to " << output_filename << "...\n\n";

        if (fallout4) {
            IDDatabase db = IDDatabase(filename);
            db.Dump(output_filename, use_base);
        } else {
            VersionDb db = VersionDb();
            db.Load(filename);
            db.Dump(output_filename, use_base);
        }
    } catch (const std::exception& exc){
        
        std::cerr << exc.what();
        return -1;
    }
    return 0;
    
}
