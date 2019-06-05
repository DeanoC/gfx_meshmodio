#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_os/filesystem.h"
#include "al2o3_vfile/vfile.hpp"
#include "al2o3_catch2/catch2.hpp"
#include "gfx_meshmod/mesh.h"
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
	MeshMod_MeshDestroy(mesh);
}