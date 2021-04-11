#pragma once
#include <cstdio>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

#include "Common.h"
#include "StoreVisitor.h"

class ExportSL final : public StoreVisitor
{
public:
  ~ExportSL();

  bool open(const char* path_obj);

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
  FILE* out = nullptr;
  Store* store = nullptr;

  std::vector<rapidjson::Pointer::Token> tokenStack;

  rapidjson::Document::AllocatorType* allocator = nullptr;
  rapidjson::Document jDoc = nullptr;

  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> writer;

  unsigned off_v = 1;
  unsigned off_n = 1;
  unsigned off_t = 1;

  void writeAttributes(rapidjson::Value& value, struct Group* group);
};