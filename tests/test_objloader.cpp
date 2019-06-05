#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_os/filesystem.h"
#include "al2o3_vfile/vfile.hpp"
#include "al2o3_catch2/catch2.hpp"
#include "gfx_meshmod/mesh.h"
#include "gfx_meshmod/vertex/position.h"
#include "gfx_meshmod/edge/halfedge.h"
#include "gfx_meshmod/polygon/tribrep.h"
#include "gfx_meshmodio/io.h"


static const char* gBasePath = "test_data/models/";
#define SET_PATH()   char existCurDir[1024]; \
	Os_GetCurrentDir(existCurDir, sizeof(existCurDir)); \
	char path[2048]; \
	strcpy(path, existCurDir); \
	strcat(path, gBasePath); \
	Os_SetCurrentDir(path)
#define RESTORE_PATH()   Os_SetCurrentDir(existCurDir)

static MeshMod_MeshHandle TestLoadObj(const char* filename) {
	SET_PATH();

	LOGINFOF("Loading %s", filename);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_Read);
	if (!file) {
		RESTORE_PATH();
		return nullptr;
	}
	MeshMod_MeshHandle mesh =  MeshModIO_LoadObjWithDefaultRegistry(file);
	RESTORE_PATH();

	return mesh;
}


TEST_CASE("cube", "[MeshModIO ObjLoader]") {
	MeshMod_MeshHandle mesh =  TestLoadObj("cube.obj");
	REQUIRE(mesh);

	auto vertices = MeshMod_MeshGetVertices(mesh);
	auto edges = MeshMod_MeshGetEdges(mesh);
	auto polygons = MeshMod_MeshGetPolygons(mesh);
	REQUIRE(MeshMod_DataContainerSize(vertices) == 36);
	REQUIRE(MeshMod_DataContainerSize(edges) == 36);
	REQUIRE(MeshMod_DataContainerSize(polygons) == 12);

	CADT_VectorHandle triBRepVector = MeshMod_DataContainerVectorLookup(polygons, MeshMod_PolygonTriBRepTag);
	auto triBRepData = (MeshMod_PolygonTriBRep*)CADT_VectorAt(triBRepVector, 0);
	REQUIRE(triBRepData[0].edge[0] == 0);
	REQUIRE(triBRepData[0].edge[1] == 1);
	REQUIRE(triBRepData[0].edge[2] == 2);
	MeshMod_MeshDestroy(mesh);
}