/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
using byte = int8_t;
#include <exedit/Filter.hpp>

#include "multi_thread.hpp"


////////////////////////////////
// 主要情報源の変数アドレス．
////////////////////////////////
inline constinit struct ExEdit092 {
	void init(AviUtl::FilterPlugin* efp)
	{
		if (fp == nullptr)
			init_pointers(efp);
	}
	AviUtl::FilterPlugin* fp = nullptr;

	void**	memory_ptr;			// 0x1a5328 // 少なくとも最大画像サイズと同サイズは保証されるっぽい．

	int32_t	yca_max_w;			// 0x196748
	int32_t	yca_max_h;			// 0x1920e0

	void(*update_any_exdata)(ExEdit::ObjectFilterIndex processing, const char* exdata_use_name);	// 0x04a7e0
	void(*nextundo)();			// 0x08d150

private:
	void init_pointers(AviUtl::FilterPlugin* efp) {
		fp = efp;

		auto pick_addr = [exedit_base = reinterpret_cast<int32_t>(efp->dll_hinst), this]
			<class T>(T & target, ptrdiff_t offset) { target = reinterpret_cast<T>(exedit_base + offset); };
		auto pick_val = [exedit_base = reinterpret_cast<int32_t>(efp->dll_hinst), this]
			<class T>(T & target, ptrdiff_t offset) { target = *reinterpret_cast<T*>(exedit_base + offset); };

		pick_addr(memory_ptr,			0x1a5328);

		pick_val (yca_max_w,			0x196748);
		pick_val (yca_max_h,			0x1920e0);

		pick_addr(update_any_exdata,	0x04a7e0);
		pick_addr(nextundo,				0x08d150);

		// make ready the multi-thread function feature.
		constexpr ptrdiff_t ofs_num_threads_address = 0x086384;
		auto aviutl_base = reinterpret_cast<uintptr_t>(fp->hinst_parent);
		multi_thread.init(fp->exfunc->exec_multi_thread_func,
			reinterpret_cast<int32_t*>(aviutl_base + ofs_num_threads_address));
	}
} exedit{};
