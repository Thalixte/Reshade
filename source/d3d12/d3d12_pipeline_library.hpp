/*
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#pragma once

#include <d3d12.h>
#include "state_tracking.hpp"

struct DECLSPEC_UUID("13FD226A-E126-11EA-87D0-0242AC130003") D3D12PipelineLibrary : ID3D12PipelineLibrary1
{
	D3D12PipelineLibrary(D3D12Device * device, ID3D12PipelineLibrary * original);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObj) override;
	ULONG   STDMETHODCALLTYPE AddRef() override;
	ULONG   STDMETHODCALLTYPE Release() override;

#pragma region ID3D12Object
	HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT * pDataSize, void *pData) override;
	HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void *pData) override;
	HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown * pData) override;
	HRESULT STDMETHODCALLTYPE SetName(LPCWSTR Name) override;
#pragma endregion
#pragma region ID3D12DeviceChild
	HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppvDevice) override;
#pragma endregion
#pragma region ID3D12PipelineLibrary
	HRESULT STDMETHODCALLTYPE StorePipeline(LPCWSTR pName, ID3D12PipelineState * pPipeline);
	HRESULT STDMETHODCALLTYPE LoadGraphicsPipeline(LPCWSTR pName, const D3D12_GRAPHICS_PIPELINE_STATE_DESC * pDesc, REFIID riid, void **ppPipelineState);
	HRESULT STDMETHODCALLTYPE LoadComputePipeline(LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC * pDesc, REFIID riid, void **ppPipelineState);
	HRESULT STDMETHODCALLTYPE Serialize(void *pData, SIZE_T DataSizeInBytes);
	SIZE_T STDMETHODCALLTYPE GetSerializedSize();
#pragma endregion
#pragma region ID3D12PipelineLibrary1
	HRESULT STDMETHODCALLTYPE LoadPipeline(LPCWSTR pName, const D3D12_PIPELINE_STATE_STREAM_DESC * pDesc, REFIID riid, void **ppPipelineState);
#pragma endregion

	bool check_and_upgrade_interface(REFIID riid);

	ULONG _ref = 1;
	ID3D12PipelineLibrary *_orig;
	unsigned int _interface_version;
	D3D12Device *const _device;
	reshade::d3d12::state_tracking _state;
};
