#include <cstdio>

#include "ExportSL.h"
#include "Store.h"
#include "LinAlgOps.h"

namespace rj = rapidjson;

namespace {

  bool open_w(FILE** f, const char* path)
  {
#ifdef _WIN32
    auto err = fopen_s(f, path, "w");
    if (err == 0) return true;

    char buf[1024];
    if (strerror_s(buf, sizeof(buf), err) != 0) {
      buf[0] = '\0';
    }
    fprintf(stderr, "Failed to open %s for writing: %s", path, buf);
#else
    * f = fopen(path, "w");
    if (*f != nullptr) return true;

    fprintf(stderr, "Failed to open %s for writing.", path);
#endif
    return false;
  }

}

ExportSL::~ExportSL()
{
  if (out) {
    fclose(out);
  }
  if (myBuf) {
    delete myBuf;
  }
}

ExportSL::ExportSL(const char* path_obj) : fileIsOpen(open_w(&out, path_obj)), myBuf(new char[2 << 16]), os(rj::FileWriteStream(out, myBuf, 2 << 16))
{
}

bool ExportSL::open(const char* path_obj)
{
  //return (fileIsOpen = open_w(&out, path_obj));
  return fileIsOpen;
}

void ExportSL::init(class Store& store) {
  assert(fileIsOpen);

  this->store = &store;
  // Currently assuming there is only one file per export
  assert(!store.getFirstRoot()->next);

  writer.Reset(this->os);
}

void ExportSL::beginModel(Group* group)
{
  // Begin Root Object
  writer.StartObject();
  writer.Key("transformsTransposed");
  writer.Bool(false);
  writer.Key("rotationsTransposed");
  writer.Bool(false);

  // Begin root assembly
  writer.Key("root");
  writer.StartObject();
  writer.Key("typeIdName");
  writer.String("JtkAssembly");
  writer.Key("assemblies");
  writer.StartArray();
}

void ExportSL::endModel()
{
  // Write to file
  writer.EndArray();
  writer.EndObject();
  writer.EndObject();
  assert(writer.IsComplete());
  writer.Flush();
}

void ExportSL::beginGroup(Group* group)
{
  //  { /* Begin assembly */
  writer.StartObject();
  writer.Key("typeIdName");
  writer.String("JtkAssembly");
}

void ExportSL::EndGroup()
{
  //  } /* End assembly */
  writer.EndObject();
}

void ExportSL::beginChildren(Group* container)
{
  // "assemblies": [
  writer.Key("assemblies");
  writer.StartArray();
}

void ExportSL::endChildren()
{
  writer.EndArray();
}

void ExportSL::beginGeometries(struct Group* container)
{
  writer.Key("parts");
  writer.StartArray();
  writer.StartObject();
  writer.Key("typeIdName");
  writer.String("JtkPart");
  writer.Key("shapeLods");
  writer.StartArray();
  writer.StartArray();
  writer.StartArray();
}

void ExportSL::geometry(struct Geometry* geometry)
{
  if (geometry->kind == Geometry::Kind::Line)
    return;

  // { /* Begin ShapeSet */
  writer.StartObject();
  writer.Key("typeIdName");
  writer.String("IndexedTrianglesSet");

  auto scale = 1.f;

  assert(geometry->triangulation);
  auto* tri = geometry->triangulation;

  if (tri->indices != 0) {

    // Indices
    writer.Key("indices");
    writer.StartArray();
    for (size_t i = 0; i < 3 * tri->triangles_n; i++) {
      writer.Uint(tri->indices[i]);
    }
    writer.EndArray();

    // Vertices
    writer.Key("vertices");
    writer.StartArray();
    for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {

      auto p = scale * mul(geometry->M_3x4, Vec3f(tri->vertices + i));

      //fprintf(out, "v %f %f %f\n", p.x, p.y, p.z);
      writer.Double(p.x);
      writer.Double(p.y);
      writer.Double(p.z);
    }
    writer.EndArray();

    // Normals
    writer.Key("normals");
    writer.StartArray();
    for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {
      auto n = normalize(mul(Mat3f(geometry->M_3x4.data), Vec3f(tri->normals + i)));

      //fprintf(out, "vn %f %f %f\n", n.x, n.y, n.z);
          
      writer.Double(std::isnormal(n.x) ? n.x : 0);
      writer.Double(std::isnormal(n.x) ? n.y : 0);
      writer.Double(std::isnormal(n.x) ? n.z : 0);
    }
    writer.EndArray();

    if (tri->texCoords) {
      writer.Key("texCoord0s");
      writer.StartArray();
      for (size_t i = 0; i < tri->vertices_n; i++) {
        const Vec2f vt(tri->texCoords + 2 * i);
        //fprintf(out, "vt %f %f\n", vt.x, vt.y);
        writer.Double(vt.x);
        writer.Double(vt.y);
      }
      writer.EndArray();
    }
  }
  // /* End ShapeSet */
  writer.EndObject();
}

void ExportSL::endGeometries()
{
  // "shapeLods": [ [ [ ... ] ] ]
  writer.EndArray();
  writer.EndArray();
  writer.EndArray();
  // "parts": [ { ... } ]
  writer.EndObject();
  writer.EndArray();
}

inline void ExportSL::writeAttributes(rapidjson::Value& value, Group* group)
{
  /*
  // typeIdName = string // not this one
  // name = string
  value.AddMember("name", "UNNAMED", *allocator);
  // version = int
  value.AddMember("version", int(0), *allocator);
  // nodeId = int
  value.AddMember("nodeId", int(0), *allocator);
  // units = string
  value.AddMember("units", "mm", *allocator);

  // attributes = JsonAttributes
  rj::Value attributes = rj::Value(rj::kObjectType);
  {
    rj::Value material = rj::Value(rj::kObjectType);

    rj::Value color = rj::Value(rj::kArrayType);
    color.PushBack(int(0), *allocator);
    color.PushBack(int(0), *allocator);
    color.PushBack(int(0), *allocator);
    color.PushBack(int(0), *allocator);

    material.AddMember("ambient", color, *allocator);
    material.AddMember("diffuse", color, *allocator);
    material.AddMember("specular", color, *allocator);
    material.AddMember("emission", color, *allocator);
    material.AddMember("shininess", int(0), *allocator);
    material.AddMember("bumpScale", int(0), *allocator);
    material.AddMember("depthOffset", int(0), *allocator);

    attributes.AddMember("material", std::move(material), *allocator);
  }
   
  // publicProperties = {}
  // privateProperties = {}
  attributes.AddMember("publicProperties", rj::Value(rj::kObjectType), *allocator);
  attributes.AddMember("privateProperties", rj::Value(rj::kObjectType), *allocator);

  // components = ?
  attributes.AddMember("components", rj::Value(rj::kObjectType), *allocator);
  */
}
