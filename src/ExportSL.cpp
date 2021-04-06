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
}

bool ExportSL::open(const char* path_obj)
{
  return open_w(&out, path_obj);
  return true;
}

void ExportSL::init(class Store& store) {
  assert(out);

  this->store = &store;

  jDoc = rj::Document(rj::kObjectType);
  allocator = &jDoc.GetAllocator();
}

void ExportSL::beginModel(Group* group)
{  
  // Begin Root Object
  //jDoc.StartObject();
  jDoc.AddMember("transformsTransposed", false, *allocator);
  //jDoc.Key("transformsTransposed", sizeof("transformsTransposed"), true);
  //jDoc.Bool(false);
  jDoc.AddMember("rotationsTransposed", false, *allocator);
  //jDoc.Key("rotationsTransposed", sizeof("rotationsTransposed"), true);
  //jDoc.Bool(false);

  // Begin root assembly
  //jDoc.Key("root", sizeof("root"), true);
  //jDoc.StartObject();
  //jDoc.AddMember("root", rj::Value(rj::kObjectType), *allocator);
  asmStack.emplace_back(rj::kObjectType);
  //jDoc.Key("typeIdName", sizeof("typeIdName"), true);
  //jDoc.String("JtkAssembly", sizeof("JtkAssembly"), true);
  asmStack.back().AddMember("typeIdName", "JtkAssembly", *allocator);
  //jDoc.Key("assemblies", sizeof("assemblies"), true);
  //jDoc.StartArray();
  asmStack.emplace_back(rj::kArrayType);
}

void ExportSL::endModel()
{
  //assert(jDoc.IsArray());
  rj::Value& assemblies = asmStack.back();
  assert(assemblies.IsArray());
  // ] /* End root assembly array */
  //jDoc.EndArray(1);
  asmStack[asmStack.size() - 2].AddMember("assemblies", assemblies, *allocator);
  asmStack.pop_back();
  // } /* End root assembly */
  rj::Value& rootAsm = asmStack.back();
  assert(rootAsm.IsObject());
  jDoc.AddMember("root", rootAsm, *allocator);
  asmStack.pop_back();
  // } /* End JsonRoot */
  //jDoc.EndObject(2);
  assert(asmStack.empty());
  
  // Write to file
  char writeBuffer[8192];
  rj::FileWriteStream os(out, writeBuffer, sizeof(writeBuffer));
  rj::Writer<rj::FileWriteStream> writer(os);
  jDoc.Accept(writer);
  writer.Flush();
}

void ExportSL::beginGroup(Group* group)
{
  assert(asmStack.back().IsArray());
  //  { /* Begin assembly */
  //jDoc.StartObject();
  asmStack.emplace_back(rj::kObjectType);
  //jDoc.Key("typeIdName", sizeof("typeIdName"), true);
  //jDoc.String("JtkAssembly", sizeof("JtkAssembly"), true);
  asmStack.back().AddMember("typeIdName", "JtkAssembly", *allocator);
}

void ExportSL::EndGroup()
{
  rj::Value& my_asm = asmStack.back();
  assert(my_asm.IsObject());
  rj::Value& assemblies = asmStack[asmStack.size() - 2];
  assert(assemblies.IsArray());
  assemblies.PushBack(my_asm, *allocator);
  asmStack.pop_back();
}

void ExportSL::beginChildren(Group* container)
{
  assert(asmStack.back().IsObject());
  // "assemblies": [
  //jDoc.Key("assemblies", sizeof("assemblies"), true);
  //jDoc.StartArray();
  asmStack.emplace_back(rj::kArrayType);
}

void ExportSL::endChildren()
{
  rj::Value& assemblies = asmStack.back();
  rj::Value& my_asm = asmStack[asmStack.size() - 2];
  assert(assemblies.IsArray());
  assert(my_asm.IsObject());
  my_asm.AddMember("assemblies", assemblies, *allocator);
  asmStack.pop_back();
}

void ExportSL::beginGeometries(struct Group* container)
{
  assert(asmStack.back().IsObject());
  // "parts" = [ /* Begin parts array */
  //jDoc.Key("parts", sizeof("parts"), true);
  //jDoc.StartArray();
  asmStack.emplace_back(rj::kArrayType);

  // Begin the first part
  assert(container->kind == Group::Kind::Group && container->group.geometries.first != nullptr);
  //jDoc.StartObject();
  asmStack.emplace_back(rj::kObjectType);
  //jDoc.Key("typeIdName", sizeof("typeIdName"), true);
  //jDoc.String("JtkPart", sizeof("JtkPart"), true);
  asmStack.back().AddMember("typeIdName", "JtkPart", *allocator);
  //jDoc.Key("shapeLods", sizeof("shapeLods"), true);
  //jDoc.StartArray();
  asmStack.emplace_back(rj::kArrayType);
  //jDoc.StartArray();
  asmStack.emplace_back(rj::kArrayType);
  //jDoc.StartArray();
  asmStack.emplace_back(rj::kArrayType);
}

void ExportSL::geometry(struct Geometry* geometry)
{
  assert(asmStack.back().IsArray());
  if (geometry->kind == Geometry::Kind::Line)
    return;

  // { /* Begin ShapeSet */
  rj::Value shapeLod(rj::kObjectType);
  shapeLod.AddMember("typeIdName", "IndexedTrianglesSet", *allocator);

  auto scale = 1.f;

  if (geometry->kind == Geometry::Kind::Line) {}
  else {
    assert(geometry->triangulation);
    auto* tri = geometry->triangulation;

    if (tri->indices != 0) {
      //fprintf(out, "g\n");
      if (geometry->triangulation->error != 0.f) {
        //fprintf(out, "# error=%f\n", geometry->triangulation->error);
      }

      // Vertices
      {
        rj::Value vertices(rj::kArrayType);
        vertices.Reserve(tri->vertices_n, *allocator);
        for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {

          auto p = scale * mul(geometry->M_3x4, Vec3f(tri->vertices + i));

          //fprintf(out, "v %f %f %f\n", p.x, p.y, p.z);
          vertices.PushBack(p.x, *allocator);
          vertices.PushBack(p.y, *allocator);
          vertices.PushBack(p.z, *allocator);
        }
        shapeLod.AddMember("vertices", std::move(vertices), *allocator);
      }

      // Normals
      {
        rj::Value normals(rj::kArrayType);
        normals.Reserve(tri->vertices_n, *allocator);
        for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {

          auto n = normalize(mul(Mat3f(geometry->M_3x4.data), Vec3f(tri->normals + i)));

          //fprintf(out, "vn %f %f %f\n", n.x, n.y, n.z);
          normals.PushBack(n.x, *allocator);
          normals.PushBack(n.y, *allocator);
          normals.PushBack(n.z, *allocator);
        }
        shapeLod.AddMember("normals", std::move(normals), *allocator);
      }

      if (tri->texCoords) {
        rj::Value texCoords(rj::kArrayType);
        texCoords.Reserve(tri->vertices_n, *allocator);
        for (size_t i = 0; i < tri->vertices_n; i++) {
          const Vec2f vt(tri->texCoords + 2 * i);
          //fprintf(out, "vt %f %f\n", vt.x, vt.y);
          texCoords.PushBack(vt.x, *allocator);
          texCoords.PushBack(vt.y, *allocator);
        }
        shapeLod.AddMember("texCoord0s", std::move(texCoords), *allocator);
      }
      else {
        for (size_t i = 0; i < tri->vertices_n; i++) {
          auto p = scale * mul(geometry->M_3x4, Vec3f(tri->vertices + 3 * i));
          //fprintf(out, "vt %f %f\n", 0 * p.x, 0 * p.y);
        }

        rj::Value indices(rj::kArrayType);
        indices.Reserve(3 * tri->triangles_n, *allocator);
        for (size_t i = 0; i < 3 * tri->triangles_n; i += 3) {
          auto a = tri->indices[i + 0];
          auto b = tri->indices[i + 1];
          auto c = tri->indices[i + 2];
          //fprintf(out, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
          //  a + off_v, a + off_t, a + off_n,
          //  b + off_v, b + off_t, b + off_n,
          //  c + off_v, c + off_t, c + off_n);
          indices.PushBack(a + off_v, *allocator);
          indices.PushBack(a + off_t, *allocator);
          indices.PushBack(a + off_n, *allocator);
          indices.PushBack(b + off_v, *allocator);
          indices.PushBack(b + off_t, *allocator);
          indices.PushBack(b + off_n, *allocator);
          indices.PushBack(c + off_v, *allocator);
          indices.PushBack(c + off_t, *allocator);
          indices.PushBack(c + off_n, *allocator);
        }
        shapeLod.AddMember("indices", std::move(indices), *allocator);
      }

      off_v += tri->vertices_n;
      off_n += tri->vertices_n;
      off_t += tri->vertices_n;
    }
  }
  // /* End ShapeSet */
  asmStack.back().PushBack(std::move(shapeLod), *allocator);
}

void ExportSL::endGeometries()
{
  //jDoc.EndArray(3);
  //jDoc.EndObject(1);
  rj::Value& lods = asmStack[asmStack.size() - 1];
  rj::Value& lodsList = asmStack[asmStack.size() - 2];
  rj::Value& lodsListList = asmStack[asmStack.size() - 3];
  rj::Value& part = asmStack[asmStack.size() - 4];
  rj::Value& parts = asmStack[asmStack.size() - 5];
  rj::Value& my_asm = asmStack[asmStack.size() - 6];
  assert(lods.IsArray());
  assert(lodsList.IsArray());
  assert(lodsListList.IsArray());
  assert(part.IsObject());
  assert(parts.IsArray());
  assert(my_asm.IsObject());
  lodsList.PushBack(lods, *allocator);
  lodsListList.PushBack(lodsList, *allocator);
  part.AddMember("shapeLods", lodsListList, *allocator);
  parts.PushBack(part, *allocator);
  my_asm.AddMember("parts", parts, *allocator);
  asmStack.pop_back();
  asmStack.pop_back();
  asmStack.pop_back();
  asmStack.pop_back();
  asmStack.pop_back();
}

inline void ExportSL::writeAttributes(rapidjson::Value& value, Group* group)
{
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
}
