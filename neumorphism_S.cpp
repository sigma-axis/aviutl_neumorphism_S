/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <numeric>
#include <numbers>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commdlg.h>

using byte = int8_t;
#include <exedit.hpp>

#include "multi_thread.hpp"
#include "exedit_memory.hpp"

using i16 = int16_t;
using i32 = int32_t;


////////////////////////////////
// 仕様書．
////////////////////////////////
struct check_data {
	enum def : int32_t {
		unchecked = 0,
		checked = 1,
		button = -1,
		dropdown = -2,
	};
};

#define PLUGIN_VERSION	"v0.10-beta5"
#define PLUGIN_AUTHOR	"sigma-axis"
#define FILTER_INFO_FMT(name, ver, author)	(name##" "##ver##" by "##author)
#define FILTER_INFO(name)	constexpr char filter_name[] = name, info[] = FILTER_INFO_FMT(name, PLUGIN_VERSION, PLUGIN_AUTHOR)
FILTER_INFO("ニューモーフィズムσ");

// trackbars.
constexpr char const* track_names[] = { "幅", "ぼかし比", "強さ", "バランス", "光角度" };
constexpr int32_t
	track_den[]			= {    1,   10,   10,    10,    10 },
	track_min[]			= { -100,    0,    0, -1000, -7200 },
	track_min_drag[]	= { -100,    0,    0, -1000, -3600 },
	track_def[]			= {   30,  500,  500,     0,  -450 },
	track_max_drag[]	= { +100, 1000, 1000, +1000, +3600 },
	track_max[]			= { +100, 5000, 5000, +1000, +7200 };

namespace idx_track
{
	enum id : int {
		size,
		blur_rate,
		alpha,
		balance,
		angle,
	};
	constexpr int count_entries = std::size(track_names);
};

static_assert(
	std::size(track_names) == std::size(track_den) &&
	std::size(track_names) == std::size(track_min) &&
	std::size(track_names) == std::size(track_min_drag) &&
	std::size(track_names) == std::size(track_def) &&
	std::size(track_names) == std::size(track_max_drag) &&
	std::size(track_names) == std::size(track_max));

// checks.
constexpr char const* check_names[]
	= { "光色の設定", "影色の設定" };
constexpr int32_t
	check_default[] = { check_data::button, check_data::button };
namespace idx_check
{
	enum id : int {
		light_color,
		shadow_color,
	};
	constexpr int count_entries = std::size(check_names);
};
static_assert(std::size(check_names) == std::size(check_default));

constexpr auto color_format = L"RGB ( %d , %d , %d )";
constexpr size_t size_col_fmt = std::wstring_view{ color_format }.size() + 1
	+ 3 * (std::size(L"255") - std::size(L"%d"));

// exdata.
constexpr ExEdit::ExdataUse exdata_use[] =
{
	{.type = ExEdit::ExdataUse::Type::Binary,  .size = 3, .name = "light" },
	{.type = ExEdit::ExdataUse::Type::Padding, .size = 1, .name = nullptr },
	{.type = ExEdit::ExdataUse::Type::Binary,  .size = 3, .name = "shadow" },
	{.type = ExEdit::ExdataUse::Type::Padding, .size = 1, .name = nullptr },
};
namespace idx_data
{
	namespace _impl
	{
		static consteval size_t idx(auto name) {
			auto ret = std::find_if(std::begin(exdata_use), std::end(exdata_use),
				[name](auto d) { return d.name != nullptr && std::string_view{ d.name } == name; }) - exdata_use;
			if (ret < std::size(exdata_use)) return ret;
			std::unreachable();
		}
	}
	enum id : int {
		light = _impl::idx("light"),
		shadow = _impl::idx("shadow"),
	};
	constexpr int count_entries = 2;
}
//#pragma pack(push, 1)
struct Exdata {
	ExEdit::Exdata::ExdataColor light { .r = 255, .g = 255, .b = 255 };
	ExEdit::Exdata::ExdataColor shadow{ .r = 0, .g = 0, .b = 0 };
};

//#pragma pack(pop)
constexpr static Exdata exdata_def = {};

static_assert(sizeof(Exdata) == std::accumulate(
	std::begin(exdata_use), std::end(exdata_use), size_t{ 0 }, [](auto v, auto d) { return v + d.size; }));

// callbacks.
BOOL func_proc(ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip);
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, ExEdit::Filter* efp);
int32_t func_window_init(HINSTANCE hinstance, HWND hwnd, int y, int base_id, int sw_param, ExEdit::Filter* efp);
static inline BOOL func_init(ExEdit::Filter* efp) { exedit.init(efp->exedit_fp); return TRUE; }


static inline constinit ExEdit::Filter filter = {
	.flag = ExEdit::Filter::Flag::Effect,
	.name = const_cast<char*>(filter_name),
	.track_n = std::size(track_names),
	.track_name = const_cast<char**>(track_names),
	.track_default = const_cast<int*>(track_def),
	.track_s = const_cast<int*>(track_min),
	.track_e = const_cast<int*>(track_max),
	.check_n = std::size(check_names),
	.check_name = const_cast<char**>(check_names),
	.check_default = const_cast<int*>(check_default),
	.func_proc = &func_proc,
	.func_init = &func_init,
	.func_WndProc = &func_WndProc,
	.exdata_size = sizeof(exdata_def),
	.information = const_cast<char*>(info),
	.func_window_init = &func_window_init,
	.exdata_def = const_cast<Exdata*>(&exdata_def),
	.exdata_use = exdata_use,
	.track_scale = const_cast<int*>(track_den),
	.track_drag_min = const_cast<int*>(track_min_drag),
	.track_drag_max = const_cast<int*>(track_max_drag),
};


////////////////////////////////
// ウィンドウ状態の保守．
////////////////////////////////
/*
efp->exfunc->get_hwnd(efp->processing, i, j):
	i = 0:		j 番目のスライダーの中央ボタン．
	i = 1:		j 番目のスライダーの左トラックバー．
	i = 2:		j 番目のスライダーの右トラックバー．
	i = 3:		j 番目のチェック枠のチェックボックス．
	i = 4:		j 番目のチェック枠のボタン．
	i = 5, 7:	j 番目のチェック枠の右にある static (テキスト).
	i = 6:		j 番目のチェック枠のコンボボックス．
	otherwise -> nullptr.
*/
static inline void update_color_text(ExEdit::Filter* efp, ExEdit::Exdata::ExdataColor color, idx_check::id idx)
{
	wchar_t fmt[size_col_fmt] = L"";

	// format color.
	::swprintf_s(fmt, color_format, color.r, color.g, color.b);

	// set label text next to the button.
	::SetWindowTextW(efp->exfunc->get_hwnd(efp->processing, 5, idx), fmt);
}

BOOL func_WndProc(HWND, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle*, ExEdit::Filter* efp)
{
	if (message != ExEdit::ExtendedFilter::Message::WM_EXTENDEDFILTER_COMMAND) return FALSE;

	auto* exdata = reinterpret_cast<Exdata*>(efp->exdata_ptr);
	auto chk = static_cast<idx_check::id>(wparam >> 16);
	auto cmd = wparam & 0xffff;

	switch (cmd) {
		using namespace ExEdit::ExtendedFilter::CommandId;
	case EXTENDEDFILTER_PUSH_BUTTON:
		switch (chk) {
			ExEdit::Exdata::ExdataColor* color;
			char const* name;
		case idx_check::light_color:
		{
			color = &exdata->light, name = exdata_use[idx_data::light].name;
			goto show_color_dialog;
		}
		case idx_check::shadow_color:
		{
			color = &exdata->shadow, name = exdata_use[idx_data::shadow].name;
			goto show_color_dialog;
		}
		show_color_dialog:
		{
			efp->exfunc->set_undo(efp->processing, 0);
			if (efp->exfunc->x6c(efp, color, 0x002)) { // color_dialog
				exedit.update_any_exdata(efp->processing, name);
				update_color_text(efp, *color, chk);
			}
			return TRUE;
		}
		}
		break;
	}
	return FALSE;
}

int32_t func_window_init(HINSTANCE hinstance, HWND hwnd, int y, int base_id, int sw_param, ExEdit::Filter* efp)
{
	if (sw_param != 0) {
		auto* exdata = reinterpret_cast<Exdata*>(efp->exdata_ptr);
		update_color_text(efp, exdata->light, idx_check::light_color);
		update_color_text(efp, exdata->shadow, idx_check::shadow_color);
	}
	return 0;
}


////////////////////////////////
// フィルタ処理．
////////////////////////////////
constexpr int log2_max_alpha = 12, max_alpha = 1 << log2_max_alpha;

void convex_edge(int len, int cos16, int sin16, int src_w, int src_h,
	ExEdit::PixelYCA* src_buf, size_t src_stride, i32* dst_buf, size_t dst_stride)
{
	int const
		abs_cos16 = std::abs(cos16), abs_sin16 = std::abs(sin16),
		dx_min = ((1 << 15) - len * abs_cos16) >> 16,
		dx_max = ((1 << 15) + len * abs_cos16) >> 16,
		dy_min = ((1 << 15) - len * abs_sin16) >> 16,
		dy_max = ((1 << 15) + len * abs_sin16) >> 16;

	int const dst_w = src_w + 2 * len, dst_h = src_h + 2 * len;
	multi_thread(dst_h, [&](int thread_id, int thread_num) {
		for (int y = thread_id; y < dst_h; y += thread_num) {
			// calculate the bound.
			int step_min_y = -len, step_max_y = len;
			if (y <= len) step_min_y = abs_sin16 == 0 ? len + 1 :
				std::max(-len, (((len - y) << 16) - (1 << 15) + abs_sin16 - 1) / abs_sin16);
			else if (y + dy_min < len) step_min_y =
				std::max(-len, (((len - y) << 16) - (1 << 15)) / abs_sin16);

			if (y >= dst_h - len - 1) step_max_y = abs_sin16 == 0 ? -len - 1 :
				std::min(+len, (((dst_h - len - y - 1) << 16) - (1 << 15) - abs_sin16 + 1) / abs_sin16);
			else if (y + dy_max >= dst_h - len) step_max_y =
				std::min(+len, (((dst_h - len - y - 1) << 16) - (1 << 15)) / abs_sin16);

			if (sin16 < 0)
				std::tie(step_min_y, step_max_y) = std::pair{ -step_max_y , -step_min_y };

			auto s_buf_y = src_buf - len + (y - len) * src_stride;
			auto d_buf_y = dst_buf + y * dst_stride;
			for (int x = 0; x < dst_w; x++, s_buf_y++, d_buf_y++) {
				// calculate the bound.
				int step_min_x = -len, step_max_x = len;
				if (x <= len) step_min_x = abs_cos16 == 0 ? len + 1 :
					std::max(-len, (((len - x) << 16) - (1 << 15) + abs_cos16 - 1) / abs_cos16);
				else if (x + dx_min < len) step_min_x =
					std::max(-len, (((len - x) << 16) - (1 << 15)) / abs_cos16);

				if (x >= dst_w - len - 1) step_max_x = abs_cos16 == 0 ? -len - 1 :
					std::min(+len, (((dst_w - len - x - 1) << 16) - (1 << 15) - abs_cos16 + 1) / abs_cos16);
				else if (x + dx_max >= dst_w - len) step_max_x = std::min(+len,
					(((dst_w - len - x - 1) << 16) - (1 << 15)) / abs_cos16);

				if (cos16 < 0)
					std::tie(step_min_x, step_max_x) = std::pair{ -step_max_x , -step_min_x };

				// calculate weighted sum of alpha values.
				int const
					n_min = std::max(step_min_x, step_min_y),
					n_max = std::min(step_max_x, step_max_y),
					n_med0 = std::min(-1, n_max), n_med1 = std::max(1, n_min);
				int sum_a = 0;
				int frac_dx = (1 << 15) + n_med0 * cos16,
					frac_dy = (1 << 15) + n_med0 * sin16;
				for (int n = n_med0; n >= n_min; n--, frac_dx -= cos16, frac_dy -= sin16)
					sum_a += s_buf_y[(frac_dx >> 16) + (frac_dy >> 16) * src_stride].a;
				frac_dx = (1 << 15) + n_med1 * cos16,
				frac_dy = (1 << 15) + n_med1 * sin16;
				for (int n = n_med1; n <= n_max; n++, frac_dx += cos16, frac_dy += sin16)
					sum_a -= s_buf_y[(frac_dx >> 16) + (frac_dy >> 16) * src_stride].a;

				// write to the destination.
				if (sum_a >= 0) sum_a += len >> 1; else sum_a -= len >> 1;
				*d_buf_y = sum_a / len;
			}
		}
	});
}

void blur_convol_v(int blur, int w, int src_h, i32* buf, size_t stride, i32* sum_vals)
{
	// convolution by rectangular distribution.
	multi_thread(w, [&](int thread_id, int thread_num) {
		int const x0 = w * thread_id / thread_num, x1 = w * (thread_id + 1) / thread_num;

		auto sum_val_0 = sum_vals + x0;
		auto s_buf_y = buf + x0 + (src_h - 1) * stride, d_buf_y = s_buf_y + blur * stride;

		std::memset(sum_val_0, 0, sizeof(*sum_val_0) * (x1 - x0));
		for (int y = std::min(src_h, blur); --y >= 0; s_buf_y -= stride, d_buf_y -= stride) {
			auto s_buf_x = s_buf_y, d_buf_x = d_buf_y;
			auto sum_val_x = sum_val_0;
			for (int x = x1 - x0; --x >= 0; sum_val_x++, s_buf_x++, d_buf_x++) {
				*sum_val_x += *s_buf_x;
				*d_buf_x = *sum_val_x / (blur + 1);
			}
		}

		if (src_h < blur) {
			for (int y = blur - src_h; --y >= 0; s_buf_y -= stride, d_buf_y -= stride) {
				auto d_buf_x = d_buf_y;
				for (int x = x1 - x0; --x >= 0; d_buf_x++)
					*d_buf_x = d_buf_x[stride];
			}
		}
		else {
			for (int y = src_h - blur; --y >= 0; s_buf_y -= stride, d_buf_y -= stride) {
				auto s_buf_x = s_buf_y, d_buf_x = d_buf_y;
				auto sum_val_x = sum_val_0;
				for (int x = x1 - x0; --x >= 0; sum_val_x++, s_buf_x++, d_buf_x++) {
					auto const d_val = *d_buf_x;
					*sum_val_x += *s_buf_x;
					*d_buf_x = *sum_val_x / (blur + 1);
					*sum_val_x -= d_val;
				}
			}
		}

		for (int y = std::min(src_h, blur); --y >= 0; d_buf_y -= stride) {
			auto d_buf_x = d_buf_y;
			auto sum_val_x = sum_val_0;
			for (int x = x1 - x0; --x >= 0; sum_val_x++, d_buf_x++) {
				auto const d_val = *d_buf_x;
				*d_buf_x = *sum_val_x / (blur + 1);
				*sum_val_x -= d_val;
			}
		}
	});
}

void blur_convol_h(int blur, int src_w, int h, i32* buf, size_t stride)
{
	// convolution by rectangular distribution.
	multi_thread(h, [&](int thread_id, int thread_num) {
		int const y0 = h * thread_id / thread_num, y1 = h * (thread_id + 1) / thread_num;

		auto buf_0 = buf + y0 * stride;
		for (int y = y1 - y0; --y >= 0; buf_0 += stride) {
			auto s_buf_y = buf_0 + src_w - 1, d_buf_y = s_buf_y + blur;
			i32 sum_val = 0;
			for (int x = std::min(src_w, blur); --x >= 0; s_buf_y--, d_buf_y--) {
				sum_val += *s_buf_y;
				*d_buf_y = sum_val / (blur + 1);
			}

			if (src_w < blur) {
				for (int x = blur - src_w; --x >= 0; d_buf_y--)
					*d_buf_y = d_buf_y[1];
			}
			else {
				for (int x = src_w - blur; --x >= 0; s_buf_y--, d_buf_y--) {
					auto const d_val = *d_buf_y;
					sum_val += *s_buf_y;
					*d_buf_y = sum_val / (blur + 1);
					sum_val -= d_val;
				}
			}

			for (int x = std::min(src_w, blur); --x >= 0; d_buf_y--) {
				auto const d_val = *d_buf_y;
				*d_buf_y = sum_val / (blur + 1);
				sum_val -= d_val;
			}
		}
	});
}

void blur_light_map(int blur, int src_w, int src_h, i32* buf, size_t stride, void* heap)
{
	// triangular distributions for each axis.
	auto w = src_w, h = src_h;
	for (auto&& _ : { 0, 1 }) {
		blur_convol_v(blur, w, h, buf, stride, reinterpret_cast<i32*>(heap)); h += blur;
	}
	for (auto&& _ : { 0, 1 }) {
		blur_convol_h(blur, w, h, buf, stride); w += blur;
	}
}

// assumes displace > 0.
static inline void expand_foursides(int displace, ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip)
{
	auto const src_w = efpip->obj_w, src_h = efpip->obj_h,
		dst_w = src_w + 2 * displace, dst_h = src_h + 2 * displace;

	multi_thread(dst_h, [&](int thread_id, int thread_num) {
		int const y0 = dst_h * thread_id / thread_num, y1 = dst_h * (thread_id + 1) / thread_num;
		auto d_buf_y = efpip->obj_temp + y0 * efpip->obj_line;
		for (int y = y1 - y0; --y >= 0; d_buf_y += efpip->obj_line) {
			auto d_buf_x = d_buf_y;
			for (int x = dst_w; --x >= 0; d_buf_x++) d_buf_x->a = 0;
		}
	});

	multi_thread(src_h, [&](int thread_id, int thread_num) {
		int const y0 = src_h * thread_id / thread_num, y1 = src_h * (thread_id + 1) / thread_num;
		auto s_buf_y = efpip->obj_edit + y0 * efpip->obj_line,
			d_buf_y = efpip->obj_temp + displace + (displace + y0) * efpip->obj_line;
		for (int y = y1 - y0; --y >= 0; s_buf_y += efpip->obj_line, d_buf_y += efpip->obj_line)
			std::memcpy(d_buf_y, s_buf_y, sizeof(*d_buf_y) * src_w);
	});

	efpip->obj_w = dst_w; efpip->obj_h = dst_h;
	std::swap(efpip->obj_temp, efpip->obj_edit);
}

constexpr ExEdit::PixelYC fromRGB(uint8_t r, uint8_t g, uint8_t b) {
	// ripped a piece of code from exedit/pixel.hpp.
	auto r_ = (r << 6) + 18;
	auto g_ = (g << 6) + 18;
	auto b_ = (b << 6) + 18;
	return {
		static_cast<i16>(((r_* 4918)>>16)+((g_* 9655)>>16)+((b_* 1875)>>16)-3),
		static_cast<i16>(((r_*-2775)>>16)+((g_*-5449)>>16)+((b_* 8224)>>16)+1),
		static_cast<i16>(((r_* 8224)>>16)+((g_*-6887)>>16)+((b_*-1337)>>16)+1),
	};
}

BOOL func_proc(ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip)
{
	int const src_w = efpip->obj_w, src_h = efpip->obj_h;
	if (src_w <= 0 || src_h <= 0) return TRUE;

	int const len_rest = std::min(exedit.yca_max_w - src_w, exedit.yca_max_h - src_h) >> 1;
	if (len_rest <= 0) return TRUE;

	constexpr int
		den_size		= track_den[idx_track::size],
		min_size		= track_min[idx_track::size],
		max_size		= track_max[idx_track::size],

		den_blur_rate	= track_den[idx_track::blur_rate],
		min_blur_rate	= track_min[idx_track::blur_rate],
		max_blur_rate	= track_max[idx_track::blur_rate],

		den_alpha_dec	= track_den[idx_track::alpha],
		min_alpha_dec	= track_min[idx_track::alpha],
		max_alpha_dec	= track_max[idx_track::alpha],
		full_alpha_dec	= track_max_drag[idx_track::alpha],

		den_balance		= track_den[idx_track::balance],
		min_balance		= track_min[idx_track::balance],
		max_balance		= track_max[idx_track::balance],

		den_angle		= track_den[idx_track::angle],
		min_angle		= track_min[idx_track::angle],
		max_angle		= track_max[idx_track::angle];

	int const
		size = std::clamp(efp->track[idx_track::size], std::max(min_size, -len_rest), std::min(max_size, len_rest)),
		abs_size = std::abs(size),
		blur = std::min(std::clamp(efp->track[idx_track::blur_rate], min_blur_rate, max_blur_rate)
			* abs_size / (100 * den_blur_rate), len_rest - abs_size),
		alpha = std::clamp(efp->track[idx_track::alpha], min_alpha_dec, max_alpha_dec),
		balance = std::clamp(efp->track[idx_track::balance], min_balance, max_balance),
		alpha_l = balance >= 0 ? alpha * max_alpha / full_alpha_dec :
			static_cast<int>((int64_t{ balance - min_balance } *alpha * max_alpha) / (-min_balance * full_alpha_dec)),
		alpha_d = balance <= 0 ? alpha * max_alpha / full_alpha_dec :
			static_cast<int>((int64_t{ max_balance - balance } *alpha * max_alpha) / (+max_balance * full_alpha_dec)),
		angle = std::clamp(efp->track[idx_track::angle], min_angle, max_angle);

	// handle trivial cases.
	if (size == 0) return TRUE;
	if (alpha <= 0) {
		if (size > 0) expand_foursides(size + blur, efp, efpip);
		return TRUE;
	}

	auto* const exdata = reinterpret_cast<Exdata*>(efp->exdata_ptr);
	auto const light_color = fromRGB(exdata->light.r, exdata->light.g, exdata->light.b),
		shadow_color = fromRGB(exdata->shadow.r, exdata->shadow.g, exdata->shadow.b);

	int const displace = abs_size + blur,
		dst_w = src_w + 2 * displace, dst_h = src_h + 2 * displace;

	// convex edge.
	auto const angle_rad = (std::numbers::pi / (180 * den_angle)) * angle;
	auto const light_map = reinterpret_cast<i32*>(*exedit.memory_ptr);
	convex_edge(abs_size,
		static_cast<int>(std::round((1 << 16) * +std::sin(angle_rad))),
		static_cast<int>(std::round((1 << 16) * -std::cos(angle_rad))),
		src_w, src_h, efpip->obj_edit, efpip->obj_line, light_map, dst_w);

	// blur.
	if (blur > 0)
		blur_light_map(blur, src_w + 2 * abs_size, src_h + 2 * abs_size, light_map, dst_w, efpip->obj_temp);

	// blend.
	if (size > 0) {
		auto blend = [&](ExEdit::PixelYCA const& src, i32 light) -> ExEdit::PixelYCA {
			if (light == 0 || src.a >= max_alpha) return src;

			auto col = light > 0 ? light_color : shadow_color;
			int a = std::max((light > 0 ? alpha_l * light : alpha_d * (-light)) >> log2_max_alpha, 0);
			int A = src.a;
			if (A <= 0) return { .y = col.y, .cb = col.cb, .cr = col.cr, .a = static_cast<i16>(a) };

			a = (a * (max_alpha - A)) >> log2_max_alpha;
			return {
				.y  = static_cast<i16>((a * col.y  + A * src.y ) / (a + A)),
				.cb = static_cast<i16>((a * col.cb + A * src.cb) / (a + A)),
				.cr = static_cast<i16>((a * col.cr + A * src.cr) / (a + A)),
				.a  = static_cast<i16>(a + A),
			};
		};
		auto place = [&](i32 light) -> ExEdit::PixelYCA {
			if (light == 0) return { .a = 0 };

			auto col = light > 0 ? light_color : shadow_color;
			auto a = std::max((light > 0 ? alpha_l * light : alpha_d * (-light)) >> log2_max_alpha, 0);
			return { .y = col.y, .cb = col.cb, .cr = col.cr, .a = static_cast<i16>(a) };
		};

		multi_thread(dst_h, [&](int thread_id, int thread_num) {
			for (int y = thread_id; y < dst_h; y += thread_num) {
				auto dst_y = efpip->obj_temp + y * efpip->obj_line;
				auto light = light_map + y * dst_w;
				if (y < displace || y >= dst_h - displace) {
					for (int x = dst_w; --x >= 0; dst_y++, light++)
						*dst_y = place(*light);
				}
				else {
					for (int x = displace; --x >= 0; dst_y++, light++)
						*dst_y = place(*light);

					auto src_y = efpip->obj_edit + (y - displace) * efpip->obj_line;
					for (int x = src_w; --x >= 0; dst_y++, light++, src_y++)
						*dst_y = blend(*src_y, *light);

					for (int x = displace; --x >= 0; dst_y++, light++)
						*dst_y = place(*light);
				}
			}
		});

		std::swap(efpip->obj_edit, efpip->obj_temp);
		efpip->obj_w = dst_w; efpip->obj_h = dst_h;
	}
	else {
		// beware that light/shadow is swapped.
		auto blend = [&](ExEdit::PixelYCA const& src, i32 light) -> ExEdit::PixelYCA {
			if (light == 0 || src.a <= 0) return src;

			auto col = light > 0 ? shadow_color : light_color;
			int a = (light > 0 ? alpha_d * light : alpha_l * (-light)) >> log2_max_alpha;
			if (a >= max_alpha) return { .y = col.y, .cb = col.cb, .cr = col.cr, .a = src.a };

			return {
				.y  = static_cast<i16>((a * col.y  + (max_alpha - a) * src.y ) >> log2_max_alpha),
				.cb = static_cast<i16>((a * col.cb + (max_alpha - a) * src.cb) >> log2_max_alpha),
				.cr = static_cast<i16>((a * col.cr + (max_alpha - a) * src.cr) >> log2_max_alpha),
				.a  = src.a,
			};
		};

		multi_thread(src_h, [&](int thread_id, int thread_num) {
			int const y0 = src_h * thread_id / thread_num, y1 = src_h * (thread_id + 1) / thread_num;
			auto dst_y = efpip->obj_edit + y0 * efpip->obj_line;
			auto light_y = light_map + displace+ (y0 + displace) * dst_w;
			for (int y = y1 - y0; --y >= 0; dst_y += efpip->obj_line, light_y += dst_w) {
				auto dst_x = dst_y;
				auto light = light_y;
				for (int x = src_w; --x >= 0; dst_x++, light++)
					*dst_x = blend(*dst_x, *light);
			}
		});
	}

	return TRUE;
}


////////////////////////////////
// DLL 初期化．
////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hinst);
		break;
	}
	return TRUE;
}


////////////////////////////////
// エントリポイント．
////////////////////////////////
EXTERN_C __declspec(dllexport) ExEdit::Filter* const* __stdcall GetFilterTableList() {
	constexpr static ExEdit::Filter* filter_list[] = {
		&filter,
		nullptr,
	};

	return filter_list;
}

