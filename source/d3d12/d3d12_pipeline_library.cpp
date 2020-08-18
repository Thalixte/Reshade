/*
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#include "dll_log.hpp"
#include "d3d12_device.hpp"
#include "d3d12_pipeline_library.hpp"

D3D12PipelineLibrary::D3D12PipelineLibrary(D3D12Device *device, ID3D12PipelineLibrary *original) :
	_orig(original),
	_interface_version(0),
	_device(device) {
	assert(_orig != nullptr && _device != nullptr);
}

bool D3D12PipelineLibrary::check_and_upgrade_interface(REFIID riid)
{
	if (riid == __uuidof(this) ||
		riid == __uuidof(IUnknown) ||
		riid == __uuidof(ID3D12Object) ||
		riid == __uuidof(ID3D12DeviceChild) ||
		riid == __uuidof(ID3D12PipelineLibrary))
		return true;

	static const IID iid_lookup[] = {
		__uuidof(ID3D12PipelineLibrary),
		__uuidof(ID3D12PipelineLibrary1),
	};

	for (unsigned int version = 0; version < ARRAYSIZE(iid_lookup); ++version)
	{
		if (riid != iid_lookup[version])
			continue;

		if (version > _interface_version)
		{
			IUnknown *new_interface = nullptr;
			if (FAILED(_orig->QueryInterface(riid, reinterpret_cast<void **>(&new_interface))))
				return false;
#if RESHADE_VERBOSE_LOG
			LOG(DEBUG) << "Upgraded ID3D12PipelineLibrary" << _interface_version << " object " << this << " to ID3D12PipelineLibrary" << version << '.';
#endif
			_orig->Release();
			_orig = static_cast<ID3D12PipelineLibrary *>(new_interface);
			_interface_version = version;
		}

		return true;
	}

	return false;
}

HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::QueryInterface(REFIID riid, void **ppvObj)
{
	if (ppvObj == nullptr)
		return E_POINTER;

	if (check_and_upgrade_interface(riid))
	{
		AddRef();
		*ppvObj = this;
		return S_OK;
	}

	return _orig->QueryInterface(riid, ppvObj);
}
ULONG   STDMETHODCALLTYPE D3D12PipelineLibrary::AddRef()
{
	_orig->AddRef();
	return InterlockedIncrement(&_ref);
}
ULONG   STDMETHODCALLTYPE D3D12PipelineLibrary::Release()
{
	const ULONG ref = InterlockedDecrement(&_ref);
	const ULONG ref_orig = _orig->Release();
	if (ref != 0)
		return ref;
	if (ref_orig != 0)
		LOG(WARN) << "Reference count for ID3D12PipelineLibrary" << _interface_version << " object " << this << " is inconsistent.";

	delete this;

	return 0;
}

HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData)
{
	return _orig->GetPrivateData(guid, pDataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
{
	return _orig->SetPrivateData(guid, DataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
{
	return _orig->SetPrivateDataInterface(guid, pData);
}
HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::SetName(LPCWSTR Name)
{
	return _orig->SetName(Name);
}

HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::GetDevice(REFIID riid, void **ppvDevice)
{
	return _device->QueryInterface(riid, ppvDevice);
}

HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::StorePipeline(LPCWSTR pName, ID3D12PipelineState *pPipeline)
{
	return _orig->StorePipeline(pName, pPipeline);
}
HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::LoadGraphicsPipeline(LPCWSTR pName, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState)
{
#if RESHADE_WIREFRAME
	D3D12_GRAPHICS_PIPELINE_STATE_DESC newdesc = *pDesc;
	D3D12_RASTERIZER_DESC &desc = newdesc.RasterizerState;

	if (desc.FillMode == D3D12_FILL_MODE_SOLID)
	{
		newdesc.BlendState.AlphaToCoverageEnable = false;
		desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		desc.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthClipEnable = false;
		desc.FrontCounterClockwise = true;
	}

	return _orig->LoadGraphicsPipeline(pName, &newdesc, riid, ppPipelineState);
#else
	return _orig->LoadGraphicsPipeline(pName, pDesc, riid, ppPipelineState);
#endif
}
HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::LoadComputePipeline(LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState)
{
	return _orig->LoadComputePipeline(pName, pDesc, riid, ppPipelineState);
}
HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::Serialize(void *pData, SIZE_T DataSizeInBytes)
{
	return _orig->Serialize(pData, DataSizeInBytes);
}
SIZE_T STDMETHODCALLTYPE D3D12PipelineLibrary::GetSerializedSize()
{
	return _orig->GetSerializedSize();
}

HRESULT STDMETHODCALLTYPE D3D12PipelineLibrary::LoadPipeline(LPCWSTR pName, const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc, REFIID riid, void **ppPipelineState)
{
	assert(_interface_version >= 1);
	return static_cast<ID3D12PipelineLibrary1 *>(_orig)->LoadPipeline(pName, pDesc, riid, ppPipelineState);
}
