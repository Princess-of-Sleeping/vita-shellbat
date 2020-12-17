/*
 *  ShellBat plugin
 *  Copyright (c) 2017 David Rosca
 *  Copyright (c) 2020 Princess of Sleeping
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/paf.h>
#include <psp2/power.h>
#include <psp2/sysmodule.h>
#include <taihen.h>

#define TAI_NEXT(this_func, hook, ...) ({ \
  (((struct _tai_hook_user *)hook)->next) != 0 ? \
    ((__typeof__(&this_func))((struct _tai_hook_user *)((struct _tai_hook_user *)hook)->next)->func)(__VA_ARGS__) \
  : \
    ((__typeof__(&this_func))((struct _tai_hook_user *)hook)->old)(__VA_ARGS__) \
  ; \
})

#define HookImport(module_name, library_nid, func_nid, func_name) \
	taiHookFunctionImport(&func_name ## _ref, module_name, library_nid, func_nid, func_name ## _patch)



SceWChar16 *sce_paf_private_wcschr(const SceWChar16 *s, SceWChar16 c);
SceWChar16 *sce_paf_private_wcsncpy(SceWChar16 *dst, const SceWChar16 *src, SceSize n);
int sce_paf_private_swprintf(SceWChar16 *dst, SceSize len, SceWChar16 *fmt, ...);
int scePafWidgetSetFontSize(void *widget, float size, int unk0, int pos, int len);

void *pBatteryWidget;
SceWChar16 cached_strings[0x64];

int pos_percent;
int prev_percent_battery;

int pos_percent_cpu;
SceKernelSysClock prevIdleClock[4];

tai_hook_ref_t ScePafWidget_FB7FE189_ref;
int ScePafWidget_FB7FE189_patch(void *pWidget, int flags, void *ctx, int a4){

	int res = TAI_CONTINUE(int, ScePafWidget_FB7FE189_ref, pWidget, flags, ctx, a4);

	if(ctx != NULL && flags == 0x800009 && *(uint32_t *)(*(void **)(ctx + 8) + 0x14C) == 0x89FFAD08){
		pBatteryWidget = *(void **)(ctx + 8);
	}

	return res;
}

tai_hook_ref_t scePafWidgetSetFontSize_ref;
int scePafWidgetSetFontSize_patch(void *pWidget, float size, int unk0, int pos, SceSize len){

	if(pWidget == pBatteryWidget && size == 16.0f)
		return 0;

	int res = TAI_NEXT(scePafWidgetSetFontSize_patch, scePafWidgetSetFontSize_ref, pWidget, size, unk0, pos, len);

	if(pWidget == pBatteryWidget && size == 22.0f){

		SceWChar16 *pMStr = sce_paf_private_wcschr(cached_strings, *L"M");
		if(pMStr != NULL && pMStr != cached_strings && ((pMStr[-1] == *L"A" || pMStr[-1] == *L"P"))){
			scePafWidgetSetFontSize(pBatteryWidget, 16.0f, 1, (int)(pMStr - cached_strings) - 2, 3);
		}

		if(pos_percent != 0){
			scePafWidgetSetFontSize(pBatteryWidget, 16.0f, 1, pos_percent, 1);
		}

		if(pos_percent_cpu != 0){
			scePafWidgetSetFontSize(pBatteryWidget, 16.0f, 1, pos_percent_cpu, 1);
		}
	}

	return res;
}

int getCpuUsagePercent(int core, uint64_t val);

int updateCpuUsagePercent(int core, SceKernelSystemInfo *info){

	int cpu = getCpuUsagePercent(core, info->cpuInfo[core].idleClock - prevIdleClock[core]);

	prevIdleClock[core] = info->cpuInfo[core].idleClock;

	return cpu;
}

tai_hook_ref_t scePafGetTimeDataStrings_ref;
int scePafGetTimeDataStrings_patch(ScePafDateTime *data, SceWChar16 *dst, SceSize len, void *a4, int a5){

	if(len != 0x64)
		return TAI_CONTINUE(int, scePafGetTimeDataStrings_ref, data, dst, len, a4, a5);

	int res = 0, out_len = 0;

	if(0 /* is_disp_cpu */){

		SceKernelSystemInfo info;
		info.size = sizeof(info);

		sceKernelGetSystemInfo(&info);

		int cpu = 0;

		cpu += updateCpuUsagePercent(0, &info);
		cpu += updateCpuUsagePercent(1, &info);
		cpu += updateCpuUsagePercent(2, &info);
		cpu += updateCpuUsagePercent(3, &info);

		res = sce_paf_private_swprintf(&dst[out_len], len, L"CPU %2d%% ", cpu >> 2);
		out_len += res; len -= res;
		pos_percent_cpu = out_len - 2;
	}

	res = TAI_CONTINUE(int, scePafGetTimeDataStrings_ref, data, &dst[out_len], len, a4, a5);
	out_len += res; len -= res;

	if(1 /* is_disp_battery */){
		int percent = scePowerGetBatteryLifePercent();
		if(percent < 0)
			percent = prev_percent_battery; // for resume

		prev_percent_battery = percent;

		res = sce_paf_private_swprintf(&dst[out_len], len, L" %d%%", percent);
		out_len += res; len -= res;
		pos_percent = out_len - 1;
	}

	sce_paf_private_wcsncpy(cached_strings, dst, sizeof(cached_strings) / 2);

	return out_len;
}

tai_hook_ref_t sceSysmoduleLoadModuleInternalWithArg_ref;
int sceSysmoduleLoadModuleInternalWithArg_patch(SceSysmoduleInternalModuleId id, SceSize args, void *argp, void *unk){

	int res = TAI_CONTINUE(int, sceSysmoduleLoadModuleInternalWithArg_ref, id, args, argp, unk);

	if(res >= 0 && id == SCE_SYSMODULE_INTERNAL_PAF){
		HookImport("SceShell", 0x3D643CE8, 0xF8D1975F, scePafGetTimeDataStrings);
		HookImport("SceShell", 0x073F8C68, 0x39B15B98, scePafWidgetSetFontSize);
		HookImport("SceShell", 0x073F8C68, 0xFB7FE189, ScePafWidget_FB7FE189);
	}

	return res;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp){

	HookImport("SceShell", 0x03FCF19D, 0xC3C26339, sceSysmoduleLoadModuleInternalWithArg);

	return SCE_KERNEL_START_SUCCESS;
}
