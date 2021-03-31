#pragma once
#include <cstdio>

#include <rapidjson/document.h>

#include "Common.h"
#include "StoreVisitor.h"

class ExportSL : public StoreVisitor
{
public:
  ~ExportSL();

  bool open(const char* path_obj);

  void init(class Store& store) override;

  //bool done() override;

  //void beginFile(struct Group* group) override;

  //void endFile() override;

  void beginModel(struct Group* group);

  void endModel() override;

  void beginGroup(struct Group* group) override;

  //void doneGroupContents(struct Group* group) {}

  void EndGroup() override;

  //void beginChildren(struct Group* container) override;

  //void endChildren() override;

  //void beginAttributes(struct Group* container) override;

  //void attribute(const char* key, const char* val) override;

  //void endAttributes(struct Group* container) override;

  void beginGeometries(struct Group* container) override;

  void geometry(struct Geometry* geometry) override;

  void endGeometries() override;

private:
  FILE* out = nullptr;
  Store* store = nullptr;
  rapidjson::Document* jDoc = nullptr;

  void writeAttributes(struct Group* group);
};