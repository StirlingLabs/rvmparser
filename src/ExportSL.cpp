#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

#include "ExportSL.h"
#include "Store.h"

namespace rj = rapidjson;

ExportSL::~ExportSL()
{
  if (out) {
    fclose(out);
  }
  if (jDoc) {
    delete jDoc;
  }
}

bool ExportSL::open(const char* path_obj)
{
  return open_w(&out, path_obj));
}

void ExportSL::init(class Store& store) {
  assert(out);
  this->store = store;
  this->jDoc = new rj::Document;
}

void ExportSL::beginModel(Group* group)
{  
  // assert(asmStack.empty());
  // assert(currentPart == nullptr);
  // { /* Begin JsonRoot */
  // writeAttributes(group);
  // 
  // assembly = { /* Begin root assembly */
  // asmStack.push(asm);
}

void ExportSL::endModel()
{
  // assert parent is assembly
  // } /* End root assembly */
  // asmStack.pop();
  // assert(asmStack.empty());
  // } /* End JsonRoot */
}

void ExportSL::beginGroup(Group* group)
{
  // assert(!asmStack.empty());
  // assert(currentPart == nullptr);
  //  { /* Begin assembly */
}

void ExportSL::EndGroup()
{
  // End assembly
}

void ExportSL::beginGeometries(struct Group* container)
{
  // assert(!asmStack.empty());
  // assert(currentPart == nullptr);
  // TODO: assert parent does not have child assemblies?
  // "parts" = { /* Begin part */
  // writeAttributes(jDoc, container);
  // "shapeLods" = [ [ [
}

void ExportSL::geometry(struct Group* container)
{
  // { /* Begin ShapeSet */
  // /* End ShapeSet */
}

void ExportSL::endGeometries()
{
  // assert(!asmStack.empty());
  // assert(currentPart != nullptr);
  // End JsonPart
}

void ExportSL::writeAttributes(struct Group* group)
{
  // transformsTransposed = false
  // rotationsTransposed = false
  // root = 
  // Begin root assembly
  // typeIdName = string
  // name = string
  // version = int
  // nodeId = int
  // units = string
  // 
  // attributes = JsonAttributes
  // 
  // publicProperties = attributes
  // privateProperties = {}
  //
  // components = ?
}
