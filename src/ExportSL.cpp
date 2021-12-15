#include <cstdio>
#include <sstream>

#include "ExportSL.h"
#include "Store.h"
#include "LinAlgOps.h"


namespace rj = rapidjson;

ExportSL::~ExportSL()
{
}

ExportSL::ExportSL(Logger logger, std::FILE* filePointer, char* buffer, size_t bufferSize) : logger(logger), os(rj::FileWriteStream(filePointer, buffer, bufferSize))
{
}

void ExportSL::init(class Store& store) {
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
    writer.EndArray();
    writer.EndObject();
    writer.EndObject();
    success = writer.IsComplete();
    assert(success);
    // Write to file
    writer.Flush();
}

void ExportSL::beginGroup(Group* group)
{
  //  { /* Begin assembly */
  writer.StartObject();
  writer.Key("typeIdName");
  writer.String("JtkAssembly");
  writer.Key("name");
  if (group->kind == Group::Kind::Model 
      && group->model.name != std::string("")) {
      writer.String(group->model.name);
  }
  else if (group->kind == Group::Kind::Group 
           && group->group.name != std::string("")) {
      writer.String(group->group.name);
  }
  else if (group->group.id != 0) {
      std::ostringstream ss;
      ss << "id " << group->group.id;
      writer.String(ss.str().c_str());
  }
  else {
      writer.String("Unnamed");
  }

  if (group->kind == Group::Kind::Group) {
      writer.Key("nodeId");
      int id = (int)group->group.id; //FIXME: ID should be a string or at least 64-bit
      if (id < 0) {
          id = 0;
      }
      writer.Int(id); 
  }
  if (group->kind == Group::Kind::Model) {
      logger(0, "Group Model");
      Color *c = group->model.colors.first;
      
      for (Color *p = group->model.colors.first; p <= group->model.colors.last; p++) {
          Color c = *p;
          logger(0, "Color %lu idx %lu (%u,%u,%u)", (unsigned long)c.colorIndex, (unsigned long)c.colorKind, c.rgb[0], c.rgb[1], c.rgb[2]);
      }
  }
  
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
}

void ExportSL::geometry(struct Geometry* geometry)
{
  if (geometry->kind == Geometry::Kind::Line)
  {
    logger(0, "Skipping line %u.", geometry->id);
    return;
  }

  // begin Part
  writer.StartObject();
  writer.Key("typeIdName");
  writer.String("JtkPart");
  writer.Key("name");
  std::ostringstream ss;
  ss << "id " << geometry->id;
  writer.String(ss.str().c_str());
  writer.Key("nodeId");
  writer.Uint(geometry->id);
  writer.Key("shapeLods");
  writer.StartArray();
  writer.StartArray();
  writer.StartArray();
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
  // "shapeLods": [ [ [ ... ] ] ]
  writer.EndArray();
  writer.EndArray();
  writer.EndArray();

  writer.Key("attributes");
  writer.StartObject();

  //material
  std::string colorName = "unknown";
  if (geometry->colorName != nullptr) {
      colorName = geometry->colorName;
  }
  double red = ((geometry->color >> 16) & 0xFF) / 255.0;
  double green = ((geometry->color >> 8) & 0xFF) / 255.0;
  double blue = ((geometry->color) & 0xFF) / 255.0;
  double alpha = geometry->transparency / 100.0;
  if (red == 0.0 && green == 0.0 && blue == 0.0 && alpha == 0.0
      && colorName != "Black" && colorName != "black") {
      logger(0, "%s: %lx (%f,%f,%f)", colorName.c_str(), geometry->color, red, green, blue);
      red = 0.5;
      green = 0.5;
      blue = 0.5;
      alpha = 1.0;
  } else {
      red = (red * 0.6) + 0.2;     //FIXME: colour grading should occur in Pipeline
      green = (green * 0.6) + 0.2; //FIXME: colour grading should occur in Pipeline
      blue = (blue * 0.6) + 0.2;   //FIXME: colour grading should occur in Pipeline
  }
  writer.Key("material");
  writer.StartObject();

  writer.Key("ambient");
  writer.StartArray();
  writer.Double(red);  
  writer.Double(green);
  writer.Double(blue);
  writer.Double(alpha);
  writer.EndArray();

  writer.Key("diffuse");
  writer.StartArray();
  writer.Double(red);
  writer.Double(green);
  writer.Double(blue);
  writer.Double(alpha);
  writer.EndArray();

  writer.Key("specular");
  writer.StartArray();
  writer.Double(red);
  writer.Double(green);
  writer.Double(blue);
  writer.Double(alpha);
  writer.EndArray();

  writer.Key("emission");
  writer.StartArray();
  writer.Double(red);
  writer.Double(green);
  writer.Double(blue);
  writer.Double(alpha);
  writer.EndArray();

  writer.EndObject(); // material
  writer.EndObject(); // attributes

  writer.Key("components");
  writer.StartObject();
  writer.Key("boundingBoxComponent");
  writer.StartObject();
  writer.Key("minCorner");
  writer.StartArray();
  writer.Double(geometry->bboxWorld.min[0]);
  writer.Double(geometry->bboxWorld.min[1]);
  writer.Double(geometry->bboxWorld.min[2]);
  writer.EndArray();
  writer.Key("maxCorner");
  writer.StartArray();
  writer.Double(geometry->bboxWorld.max[0]);
  writer.Double(geometry->bboxWorld.max[1]);
  writer.Double(geometry->bboxWorld.max[2]);
  writer.EndArray();
  writer.EndObject(); // boundingBoxComponent
  writer.EndObject(); // components

  // publicProperties = {}
  // privateProperties = {}

  writer.EndObject(); // part
}

void ExportSL::endGeometries()
{
    // "parts": [ { ... } ]
    writer.EndArray();
}
