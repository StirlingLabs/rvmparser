#pragma once
#include <cstdio>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

#include "Common.h"
#include "StoreVisitor.h"
#include "ExportSL.h"

class ExportStirlingJson
{
public:
	~ExportStirlingJson();

	ExportStirlingJson(Store* store, Logger logger, const char* path_obj);

	bool success = false;


private:
	bool fileIsOpen = false;
	FILE* out = nullptr;
	Store* store = nullptr;
	Logger logger = nullptr;
	char* myBuf;
};

