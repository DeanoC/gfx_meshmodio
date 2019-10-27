	#pragma once
#ifndef GFX_MESHMODIO_IO_H
#define GFX_MESHMODIO_IO_H

#include "al2o3_platform/platform.h"
#include "render_meshmod/mesh.h"
#include "al2o3_vfile/vfile.h"

AL2O3_EXTERN_C MeshMod_MeshHandle MeshModIO_LoadObj(MeshMod_RegistryHandle registry, VFile_Handle handle);


// try to figure out which format the file is in and load it
AL2O3_EXTERN_C MeshMod_MeshHandle MeshModIO_Load(VFile_Handle handle);

#endif //GFX_MESHMODIO_IO_H
