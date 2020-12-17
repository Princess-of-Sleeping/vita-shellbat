/*
 * taihen_extra.c
 * Copyright (C) 2020, Princess of Sleeping
 */

#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/clib.h>
#include <taihen.h>
#include "taihen_extra.h"

typedef struct SceModuleImport1 {
	uint16_t size;               // 0x34
	uint16_t version;
	uint16_t flags;
	uint16_t entry_num_function;
	uint16_t entry_num_variable;
	uint16_t entry_num_tls;
	uint32_t rsvd1;
	uint32_t libnid;
	const char *libname;
	uint32_t rsvd2;
	uint32_t *table_func_nid;
	void    **table_function;
	uint32_t *table_vars_nid;
	void    **table_variable;
	uint32_t *table_tls_nid;
	void    **table_tls;
} SceModuleImport1;

typedef struct SceModuleImport2 {
	uint16_t size; // 0x24
	uint16_t version;
	uint16_t flags;
	uint16_t entry_num_function;
	uint16_t entry_num_variable;
	uint16_t data_0x0A; // unused?
	uint32_t libnid;
	const char *libname;
	uint32_t *table_func_nid;
	void    **table_function;
	uint32_t *table_vars_nid;
	void    **table_variable;
} SceModuleImport2;

typedef union SceModuleImport {
	uint16_t size;
	SceModuleImport1 type1;
	SceModuleImport2 type2;
} SceModuleImport;

int searchModuleImportFunc(const tai_module_info_t *info, const char *libname, uintptr_t func, uint32_t *dst){

	SceModuleImport *pImports = (SceModuleImport *)info->imports_start;
	SceSize size = info->imports_end - info->imports_start;
	while(size != 0){

		uint16_t entry_num_function = 0;
		const char *_libname;
		void **_table_function;

		if(pImports->size == sizeof(SceModuleImport1)){
			entry_num_function = pImports->type1.entry_num_function;
			_libname           = pImports->type1.libname;
			_table_function    = pImports->type1.table_function;
		}else if(pImports->size == sizeof(SceModuleImport2)){
			entry_num_function = pImports->type2.entry_num_function;
			_libname           = pImports->type2.libname;
			_table_function    = pImports->type2.table_function;
		}else{
			return -1;
		}

		if(sceClibStrcmp(libname, _libname) == 0){
			for(int i=0;i<entry_num_function;i++){

				uint32_t movw = *(uint32_t *)(_table_function[i]);
				uint32_t movt = *(uint32_t *)(_table_function[i] + 4);

				movw = ((movw & 0xF0000) >> 4) | (movw & 0xFFF);
				movt = ((movt & 0xF0000) >> 4) | (movt & 0xFFF);

				if(((movt << 16) | movw) == func){
					*dst = (uint32_t)(_table_function[i]);
					return 0;
				}
			}
		}

		size -= pImports->size;
		pImports = (SceModuleImport *)((uintptr_t)pImports + pImports->size);
	}

	return -1;
}

int taiGetModuleImportFuncForNonEnso(
	const char *target_module, const char *source_module,
	const char *libname, uint32_t libnid, uint32_t funcnid,
	tai_hook_ref_t *ref, void *func
){
	int res;
	tai_module_info_t info;
	info.size = sizeof(info);

	res = taiGetModuleInfo(target_module, &info);
	if(res < 0)
		return res;

	uint32_t import_func = 0;
	uintptr_t paf_ptr = 0;

	SceKernelModuleInfo sce_info;
	res = sceKernelGetModuleInfo(info.modid, &sce_info);
	if(res < 0)
		return res;

	res = taiGetModuleExportFunc(source_module, libnid, funcnid, &paf_ptr);
	if(res < 0)
		return res;

	res = searchModuleImportFunc(&info, libname, paf_ptr, &import_func);
	if(res < 0)
		return res;

	return taiHookFunctionOffset(ref, info.modid, 0, import_func - (uint32_t)sce_info.segments[0].vaddr, 0, func);
}
