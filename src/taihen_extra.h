/*
 * taihen_extra.h
 * Copyright (C) 2020, Princess of Sleeping
 */

#ifndef _MY_TAIHEN_EXTRA_H_
#define _MY_TAIHEN_EXTRA_H_

#include <taihen.h>

#define HookImportNonEnso(target_module, source_module, libname, libnid, funcnid, func_name) \
	taiGetModuleImportFuncForNonEnso(target_module, source_module, libname, libnid, funcnid, &func_name ## _ref, func_name ## _patch)

int taiGetModuleImportFuncForNonEnso(
	const char *target_module, const char *source_module,
	const char *libname, uint32_t libnid, uint32_t funcnid,
	tai_hook_ref_t *ref, void *func
);

#endif /* _MY_TAIHEN_EXTRA_H_ */
