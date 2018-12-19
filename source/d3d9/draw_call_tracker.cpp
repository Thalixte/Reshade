#include "draw_call_tracker.hpp"
#include "log.hpp"
#include <math.h>

const auto D3DFMT_INTZ = static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z'));
const auto D3DFMT_DF16 = static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '1', '6'));
const auto D3DFMT_DF24 = static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '2', '4'));

namespace reshade::d3d9
{
	void draw_call_tracker::merge(const draw_call_tracker& source)
	{
		_global_counter.vertices += source.total_vertices();
		_global_counter.drawcalls += source.total_drawcalls();

		// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS
		for (const auto &[depthstencil, snapshot] : source._counters_per_used_depthstencil)
		{
			_counters_per_used_depthstencil[depthstencil].stats.vertices += snapshot.stats.vertices;
			_counters_per_used_depthstencil[depthstencil].stats.drawcalls += snapshot.stats.drawcalls;
		}

		for (auto source_entry : source._cleared_depth_surfaces)
		{
			const auto destination_entry = _cleared_depth_surfaces.find(source_entry.first);

			if (destination_entry == _cleared_depth_surfaces.end())
			{
				_cleared_depth_surfaces.emplace(source_entry.first, source_entry.second);
			}
		}
		// #endif
#if RESHADE_DX9_CAPTURE_CONSTANT_BUFFERS
#endif
	}

	void draw_call_tracker::reset()
	{
		_global_counter.vertices = 0;
		_global_counter.drawcalls = 0;
		// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS
		_counters_per_used_depthstencil.clear();
		_cleared_depth_surfaces.clear();
		// #endif
#if RESHADE_DX9_CAPTURE_CONSTANT_BUFFERS
#endif
	}

	void draw_call_tracker::on_draw(IDirect3DDevice9 *device, IDirect3DSurface9 *depthstencil_replacement, IDirect3DSurface9 *current_depthstencil, UINT vertices, BOOL wireframeMode)
	{
		UNREFERENCED_PARAMETER(device);

		_global_counter.vertices += vertices;
		_global_counter.drawcalls += 1;

		if (wireframeMode)
		{
			device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		}
		else
		{
			device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		}

		// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS
		com_ptr<IDirect3DSurface9> depthstencil;

		device->GetDepthStencilSurface(&depthstencil);

		if (depthstencil == nullptr)
			// This is a draw call with no depth stencil
			return;

		// during the transition phase between depthstencils, the stats must be redirected to the good one
		if (depthstencil == depthstencil_replacement)
		{
			depthstencil = current_depthstencil;
		}

		if (const auto intermediate_snapshot = _counters_per_used_depthstencil.find(depthstencil); intermediate_snapshot != _counters_per_used_depthstencil.end())
		{
			intermediate_snapshot->second.stats.vertices += vertices;
			intermediate_snapshot->second.stats.drawcalls += 1;
		}
		// #endif
#if RESHADE_DX9_CAPTURE_CONSTANT_BUFFERS		
#endif
	}

	// #if RESHADE_DX9_CAPTURE_DEPTH_BUFFERS

	bool draw_call_tracker::check_depth_surface_format(int depth_buffer_surface_format, IDirect3DSurface9 *pDepthSurface)
	{
		if (pDepthSurface == nullptr)
		{
			return false;
		}

		D3DSURFACE_DESC  desc;
		pDepthSurface->GetDesc(&desc);
		D3DFORMAT depth_surface_format = desc.Format;

		const D3DFORMAT depth_surface_formats[] = {
			D3DFMT_UNKNOWN,
			D3DFMT_INTZ,
			D3DFMT_D15S1,
			D3DFMT_D24S8,
			D3DFMT_D24X8,
			D3DFMT_D24X4S4,
			D3DFMT_D24FS8,
			D3DFMT_D16,
			D3DFMT_DF16,
			D3DFMT_DF24
		};

		assert(depth_buffer_surface_format >= 0 && depth_buffer_surface_format < ARRAYSIZE(depth_surface_formats));

		if (depth_surface_formats[depth_buffer_surface_format] != D3DFMT_UNKNOWN && depth_surface_format != depth_surface_formats[depth_buffer_surface_format])
		{
			return false;
		}

		return true;
	}
	bool draw_call_tracker::check_depthstencil(IDirect3DSurface9 *depthstencil) const
	{
		return _counters_per_used_depthstencil.find(depthstencil) != _counters_per_used_depthstencil.end();
	}

	void draw_call_tracker::track_rendertargets(int depth_buffer_surface_format, IDirect3DSurface9 *depthstencil)
	{
		assert(depthstencil != nullptr);

		if (!check_depth_surface_format(depth_buffer_surface_format, depthstencil))
		{
			return;
		}

		const auto it = _counters_per_used_depthstencil.find(depthstencil);

		if (it == _counters_per_used_depthstencil.end())
		{
			_counters_per_used_depthstencil[depthstencil].depthstencil = depthstencil;
		}
	}
	void draw_call_tracker::track_depth_surface(int depth_buffer_surface_format, UINT index, com_ptr<IDirect3DSurface9> src_depthstencil, com_ptr<IDirect3DSurface9> dest_depthstencil, bool cleared)
	{
		// Function that keeps track of a cleared depth surface in an ordered map in order to retrieve it at the final rendering stage
		assert(src_depthstencil != nullptr);

		if (!check_depth_surface_format(depth_buffer_surface_format, src_depthstencil.get()))
		{
			return;
		}

		// Gather some extra info for later display
		D3DSURFACE_DESC src_depthstencil_desc;
		src_depthstencil->GetDesc(&src_depthstencil_desc);

		// check if it is really a depth surface
		if (src_depthstencil_desc.Usage != D3DUSAGE_DEPTHSTENCIL)
		{
			return;
		}

		// fill the ordered map with the saved depth surface
		if (const auto it = _cleared_depth_surfaces.find(index); it == _cleared_depth_surfaces.end())
		{
			_cleared_depth_surfaces.emplace(index, depth_surface_save_info{ src_depthstencil, src_depthstencil_desc, dest_depthstencil, cleared });
		}
		else
		{
			it->second = depth_surface_save_info{ src_depthstencil, src_depthstencil_desc, dest_depthstencil, cleared };
		}
	}

	draw_call_tracker::intermediate_snapshot_info draw_call_tracker::find_best_snapshot(UINT width, UINT height)
	{
		const float aspect_ratio = float(width) / float(height);

		D3DSURFACE_DESC desc;
		intermediate_snapshot_info best_snapshot;

		for (auto &[depthstencil, snapshot] : _counters_per_used_depthstencil)
		{
			if (snapshot.stats.drawcalls == 0 || snapshot.stats.vertices == 0)
			{
				continue;
			}
			snapshot.depthstencil->GetDesc(&desc);

			// check if it is really a depth surface
			if (desc.Usage != D3DUSAGE_DEPTHSTENCIL)
			{
				// continue;
			}

			// Check aspect ratio
			const float width_factor = desc.Width != width ? float(width) / desc.Width : 1.0f;
			const float height_factor = desc.Height != height ? float(height) / desc.Height : 1.0f;
			const float surface_aspect_ratio = float(desc.Width) / float(desc.Height);

			if (fabs(surface_aspect_ratio - aspect_ratio) > 0.1f || width_factor > 2.0f || height_factor > 2.0f || width_factor < 0.5f || height_factor < 0.5f)
			{
				// No match, not a good fit
				continue;
			}

			if (snapshot.stats.drawcalls >= best_snapshot.stats.drawcalls)
			{
				best_snapshot = snapshot;
			}
		}

		return best_snapshot;
	}

	void draw_call_tracker::keep_cleared_depth_surfaces()
	{
		// Function that keeps only the depth surfaces that has been retrieved before the last depth stencil clearance
		std::map<UINT, depth_surface_save_info>::reverse_iterator it = _cleared_depth_surfaces.rbegin();

		// Reverse loop on the cleared depth surfaces map
		while (it != _cleared_depth_surfaces.rend())
		{
			// Exit if the last cleared depth stencil is found
			if (it->second.cleared)
				return;

			// Remove the depth surface if it was retrieved after the last clearance of the depth stencil
			it = std::map<UINT, depth_surface_save_info>::reverse_iterator(_cleared_depth_surfaces.erase(std::next(it).base()));
		}
	}

	IDirect3DSurface9 *draw_call_tracker::find_best_cleared_depth_buffer_surface(UINT depth_surface_clearing_number)
	{
		// Function that selects the best cleared depth surface according to the clearing number defined in the configuration settings
		IDirect3DSurface9 *best_match = nullptr;

		// Ensure to work only on the depth surfaces retrieved before the last depth surface clearance
		keep_cleared_depth_surfaces();

		for (const auto &it : _cleared_depth_surfaces)
		{
			UINT i = it.first;
			auto &surface_counter_info = it.second;

			com_ptr<IDirect3DSurface9> surface;

			if (surface_counter_info.dest_depthstencil == nullptr)
			{
				continue;
			}

			surface = surface_counter_info.dest_depthstencil;

			if (depth_surface_clearing_number != 0 && i > depth_surface_clearing_number)
			{
				continue;
			}

			// The _cleared_dept_surfaces ordered map stores the depth surfaces, according to the order of clearing
			// if depth_buffer_clearing_number == 0, the auto select mode is defined, so the last cleared depth surface is retrieved
			// if the user selects a clearing number and the number of cleared depth surfaces is greater or equal than it, the surface corresponding to this number is retrieved
			// if the user selects a clearing number and the number of cleared depth surfaces is lower than it, the last cleared depth surface is retrieved
			best_match = surface.get();
		}

		return best_match;
	}
	// #endif
}
