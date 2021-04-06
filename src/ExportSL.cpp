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

  //writeBuffer = new char[65536];
  //rj::FileWriteStream os(out, writeBuffer, 65536);
  //writer = new rj::Writer<rj::FileWriteStream>(os);
  jDoc = rj::Document(rapidjson::kObjectType);
  allocator = &jDoc.GetAllocator();
}

void ExportSL::beginModel(Group* group)
{  
  assert(asmVec.empty());
  //assert(currentPart == nullptr);

  //writer->StartObject();

  // Write root properties
  //writer->String("transformsTransposed");
  //writer->Bool(false);
  //writer->Key("rotationsTransposed");
  //writer->Bool(false);
  jDoc.AddMember("transformsTransposed", rapidjson::Value(false), *allocator);
  jDoc.AddMember("rotationsTransposed", rapidjson::Value(false), *allocator);

  // Begin root assembly
  //writer->Key("root");
  //writer->StartObject();
  //writeAttributes(writer, group);

  //asmStack.push(rootAsm);
  rj::Value rootAsm(rj::kObjectType);
  rootAsm.AddMember("typeIdName", "JtkAssembly", *allocator);
  writeAttributes(rootAsm, group);
  asmVec.push_back(std::move(rootAsm));
}

void ExportSL::endModel()
{
  // } /* End root assembly */
  //writer->EndObject();
  
  jDoc.AddMember("root", std::move(asmVec[(asmVec.size() - 1)]), *allocator);
  asmVec.pop_back();
  assert(asmVec.empty());
  
  // } /* End JsonRoot */
  //writer->EndObject();

  char writeBuffer[8192];
  rj::FileWriteStream os(out, writeBuffer, sizeof(writeBuffer));
  rj::Writer<rj::FileWriteStream> writer(os);
  jDoc.Accept(writer);
  writer.Flush();
}

void ExportSL::beginGroup(Group* group)
{
  // assert(!asmStack.empty());
  // assert(currentPart == nullptr);
  //  { /* Begin assembly */
  //writer->StartObject();
  rj::Value my_asm(rj::kObjectType);
  my_asm.AddMember("typeIdName", "JtkAssembly", *allocator);
  writeAttributes(my_asm, group);
  my_asm.AddMember("assemblies", rj::Value(rj::kArrayType), *allocator);
  my_asm.AddMember("parts", rj::Value(rj::kArrayType), *allocator);

  asmVec.push_back(std::move(my_asm));
}

void ExportSL::EndGroup()
{
  // End assembly
  //writer->EndObject();
  assert(asmVec.size() >= 2);
  //rj::Value& val1 = asmVec[(asmVec.size() - 1)];
  rj::Value& val2 = asmVec[(asmVec.size() - 2)];
  assert(asmVec.back().IsObject());
  assert(val2.IsObject());
  //assert(val2.HasMember("assemblies"));
  if (!val2.HasMember("assemblies")) {
    val2.AddMember("assemblies", rj::Value(rj::kArrayType), *allocator);
  }
  val2.FindMember("assemblies")->value.PushBack(std::move(asmVec.back()), *allocator);
  asmVec.pop_back();
}

void ExportSL::beginGeometries(struct Group* container)
{
  // assert(!asmStack.empty());
  // assert(currentPart == nullptr);
  // TODO: assert parent does not have child assemblies?
  // "parts" = { /* Begin part */
  //writer->Key("parts");
  //writer->StartObject();
  rj::Value part(rj::kObjectType);
  part.AddMember("typeIdName", "JtkPart", *allocator);
  rj::Value arr(rj::kArrayType);
  //part.AddMember("shapeLods", )
  //writeAttributes(writer, container);
  // "shapeLods" = [ [ [
  //writer->StartArray();
  //writer->StartArray();
  //writer->StartArray();
  asmVec.push_back(std::move(part));
  asmVec.push_back(std::move(arr));
}

void ExportSL::geometry(struct Geometry* geometry)
{
  if (geometry->kind == Geometry::Kind::Line)
    return;
  // { /* Begin ShapeSet */
  //writer->StartObject();
  rj::Value shapeLod(rj::kObjectType);

  //writer->Key("typeIdName");
  //writer->String("IndexedTrianglesSet");
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
        //writer->Key("vertices");
        //writer->StartArray();
        rj::Value vertices(rj::kArrayType);
        //vertices.Reserve(tri->vertices_n, *allocator);
        for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {

          auto p = scale * mul(geometry->M_3x4, Vec3f(tri->vertices + i));

          //fprintf(out, "v %f %f %f\n", p.x, p.y, p.z);
          //writer->Double(p.x);
          //writer->Double(p.y);
          //writer->Double(p.z);
          vertices.PushBack(p.x, *allocator);
          vertices.PushBack(p.y, *allocator);
          vertices.PushBack(p.z, *allocator);
        }
        //writer->EndArray();
        shapeLod.AddMember("vertices", std::move(vertices), *allocator);
      }

      // Normals
      {
        //writer->Key("normals");
        //writer->StartArray();
        rj::Value normals(rj::kArrayType);
        //normals.Reserve(tri->vertices_n, *allocator);
        for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {

          auto n = normalize(mul(Mat3f(geometry->M_3x4.data), Vec3f(tri->normals + i)));

          //fprintf(out, "vn %f %f %f\n", n.x, n.y, n.z);
          //writer->Double(n.x);
          //writer->Double(n.y);
          //writer->Double(n.z);
          normals.PushBack(n.x, *allocator);
          normals.PushBack(n.y, *allocator);
          normals.PushBack(n.z, *allocator);
        }
        //writer->EndArray();
        shapeLod.AddMember("normals", std::move(normals), *allocator);
      }

      if (tri->texCoords) {
        //writer->Key("texCoord0s");
        //writer->StartArray();
        rj::Value texCoords(rj::kArrayType);
        //texCoords.Reserve(tri->vertices_n, *allocator);
        for (size_t i = 0; i < tri->vertices_n; i++) {
          const Vec2f vt(tri->texCoords + 2 * i);
          //fprintf(out, "vt %f %f\n", vt.x, vt.y);
          //writer->Double(vt.x);
          //writer->Double(vt.y);
          texCoords.PushBack(vt.x, *allocator);
          texCoords.PushBack(vt.y, *allocator);
        }
        //writer->EndArray();
        shapeLod.AddMember("texCoord0s", std::move(texCoords), *allocator);
      }
      else {
        for (size_t i = 0; i < tri->vertices_n; i++) {
          auto p = scale * mul(geometry->M_3x4, Vec3f(tri->vertices + 3 * i));
          //fprintf(out, "vt %f %f\n", 0 * p.x, 0 * p.y);
        }

        //writer->Key("indices");
        //writer->StartArray();
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
          //writer->Uint(a + off_v);
          //writer->Uint(a + off_t);
          //writer->Uint(a + off_n);
          //writer->Uint(b + off_v);
          //writer->Uint(b + off_t);
          //writer->Uint(b + off_n);
          //writer->Uint(c + off_v);
          //writer->Uint(c + off_t);
          //writer->Uint(c + off_n);
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
        //writer->EndArray();
        shapeLod.AddMember("indices", std::move(indices), *allocator);
      }

      off_v += tri->vertices_n;
      off_n += tri->vertices_n;
      off_t += tri->vertices_n;
    }
  }
  // /* End ShapeSet */
  //writer->EndObject();
  asmVec.back().PushBack(std::move(shapeLod), *allocator);
}

void ExportSL::endGeometries()
{
  // assert(!asmStack.empty());
  // assert(currentPart != nullptr);
  // End JsonPart
  //writer->EndArray();
  //writer->EndArray();
  //writer->EndArray();
  //writer->EndObject();
  {
    rj::Value arr0(rj::kArrayType);
    {
      rj::Value arr1(rj::kArrayType);

      assert(asmVec.back().IsArray());
      arr1.PushBack(std::move(asmVec[asmVec.size() - 1]), *allocator);
      asmVec.pop_back();
      arr0.PushBack(std::move(arr1), *allocator);
    }
    assert(asmVec.back().IsObject());
    asmVec.back().AddMember("shapeLods", std::move(arr0), *allocator);
  }
  rj::Value& my_asm = asmVec[asmVec.size() - 2];
  assert(my_asm.HasMember("parts"));
  my_asm.FindMember("parts")->value.PushBack(std::move(asmVec[asmVec.size() - 1]), *allocator);
  asmVec.pop_back();
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
