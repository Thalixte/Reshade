#pragma once

#include <d3d9.h>
#include <map>
#include "com_ptr.hpp"

#define RESHADE_DX9_CAPTURE_DEPTH_BUFFERS 1
#define RESHADE_DX9_CAPTURE_CONSTANT_BUFFERS 0

namespace reshade::d3d9
{
	class draw_call_tracker
	{
	public:
		struct draw_stats
		{
			UINT vertices = 0;
			UINT drawcalls = 0;
			UINT mapped = 0;
			UINT vs_uses = 0;
			UINT ps_uses = 0;
		};

		// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS
		struct intermediate_snapshot_info
		{
			IDirect3DSurface9 *depthstencil = nullptr; // No need to use a 'com_ptr' here since '_counters_per_used_depthstencil' already keeps a reference
			draw_stats stats;
		};
		// #endif

		UINT total_vertices() const { return _global_counter.vertices; }
		UINT total_drawcalls() const { return _global_counter.drawcalls; }

		// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS
		const auto &depth_buffer_counters() const { return _counters_per_used_depthstencil; }
		const auto &cleared_depth_surfaces() const { return _cleared_depth_surfaces; }
		// #endif
#if RESHADE_DX9_CAPTURE_CONSTANT_BUFFERS
#endif

		void merge(const draw_call_tracker &source);
		void reset();

		void on_draw(IDirect3DDevice9 *device, IDirect3DSurface9 *depthstencil_replacement, IDirect3DSurface9 *current_depthstencil, UINT vertices, BOOL wireframeMode);

		// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS
		bool check_depth_surface_format(int depth_buffer_surface_format, IDirect3DSurface9 *pDepthSurface);
		bool check_depthstencil(IDirect3DSurface9 *depthstencil) const;
		void track_rendertargets(int depth_buffer_surface_format, IDirect3DSurface9 *depthstencil);
		void track_depth_surface(int depth_buffer_surface_format, UINT index, com_ptr<IDirect3DSurface9> src_depthstencil, com_ptr<IDirect3DSurface9> dest_depthstencil, bool cleared);

		void keep_cleared_depth_surfaces();

		intermediate_snapshot_info find_best_snapshot(UINT width, UINT height);
		IDirect3DSurface9 *find_best_cleared_depth_buffer_surface(UINT depth_surface_clearing_number);
		// #endif

	private:
		struct depth_surface_save_info
		{
			com_ptr<IDirect3DSurface9> src_depthstencil;
			D3DSURFACE_DESC src_surface_desc;
			com_ptr<IDirect3DSurface9> dest_depthstencil;
			bool cleared = false;
		};

		draw_stats _global_counter;
		// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS
		// Use "std::map" instead of "std::unordered_map" so that the iteration order is guaranteed
		std::map<com_ptr<IDirect3DSurface9>, intermediate_snapshot_info> _counters_per_used_depthstencil;
		std::map<UINT, depth_surface_save_info> _cleared_depth_surfaces;
		// #endif
#if RESHADE_DX9_CAPTURE_CONSTANT_BUFFERS
#endif
	};
}