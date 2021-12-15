#pragma once
#include <cstdio>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <rapidjson/writer.h>

#include "Common.h"
#include "StoreVisitor.h"

class ExportSL final : public StoreVisitor
{
public:
  ~ExportSL();

  ExportSL(Logger logger, std::FILE* filePointer, char* buffer, size_t bufferSize);

  bool success = false;

  //bool open(const char* path_obj);

  void init(class Store& store) override;

  //bool done() override;

  //void beginFile(struct Group* group) override;

  //void endFile() override;

  void beginModel(struct Group* group) override;

  void endModel() override;

  void beginGroup(struct Group* group) override;

  //void doneGroupContents(struct Group* group) override;

  void EndGroup() override;

  void beginChildren(struct Group* container) override;

  void endChildren() override;

  //void beginAttributes(struct Group* container) override;

  //void attribute(const char* key, const char* val) override;

  //void endAttributes(struct Group* container) override;

  void beginGeometries(struct Group* container) override;

  void geometry(struct Geometry* geometry) override;

  void endGeometries() override;

private:
  bool fileIsOpen = false;
  FILE* out = nullptr;
  Store* store = nullptr;
  Logger logger = nullptr;

  char* myBuf;
  rapidjson::FileWriteStream os;
  rapidjson::Writer<rapidjson::FileWriteStream> writer;

  void writeAttributes(rapidjson::Value& value, struct Group* group);
};