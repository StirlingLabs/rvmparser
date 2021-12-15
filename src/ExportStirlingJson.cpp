#include <cstdio>

#include "ExportStirlingJson.h"
#include "Store.h"
#include "LinAlgOps.h"

namespace rj = rapidjson;

namespace {

    bool open_w(FILE** f, const char* path)
    {
        fprintf(stderr, "Opening %s for writing.\n", path);
#ifdef _WIN32
        auto err = fopen_s(f, path, "w");
        if (err == 0) return true;

        char buf[1024];
        if (strerror_s(buf, sizeof(buf), err) != 0) {
            buf[0] = '\0';
        }
        fprintf(stderr, "Failed to open %s for writing: %s.\n", path, buf);
#else
        * f = fopen(path, "w");
        if (*f != nullptr) return true;

        fprintf(stderr, "Failed to open %s for writing.\n", path);
#endif
        return false;
    }

}

ExportStirlingJson::~ExportStirlingJson()
{
    if (out) {
        fclose(out);
    }
    if (myBuf) {
        delete myBuf;
    }
}


ExportStirlingJson::ExportStirlingJson(Store* store, Logger logger, const char* path_obj) : store(store), logger(logger)
{
    fileIsOpen = open_w(&out, path_obj);
    myBuf = new char[2 << 16];
    ExportSL slExporter(logger, out, myBuf, sizeof(myBuf));
    store->apply(&slExporter);
    success = slExporter.success;
}
