#pragma once
#ifdef DK_NO_OPAQUE_DUMMY
#undef DK_NO_OPAQUE_DUMMY
#endif
#include <type_traits>
#include <initializer_list>
#include <array>
#ifdef DK_HPP_SUPPORT_VECTOR
#include <vector>
#endif
#include "deko3d.h"

namespace dk
{
	namespace detail
	{
		template <typename T>
		class Handle
		{
			T m_handle;
		protected:
			void _clear() noexcept { m_handle = nullptr; }
			void _set(T h) noexcept { _clear(); m_handle = h; }
		public:
			using Type = T;
			constexpr Handle() noexcept : m_handle{} { }
			constexpr Handle(std::nullptr_t) noexcept : m_handle{} { }
			constexpr Handle(T handle) noexcept : m_handle{handle} { }
			constexpr operator T() const noexcept { return m_handle; }
			constexpr operator bool() const noexcept { return m_handle != nullptr; }
			constexpr bool operator !() const noexcept { return m_handle == nullptr; }
		};

		template <typename T>
		constexpr bool operator==(Handle<T> const& lhs, Handle<T> const& rhs)
		{
			return static_cast<T>(lhs) == static_cast<T>(rhs);
		}

		template <typename T>
		constexpr bool operator!=(Handle<T> const& lhs, Handle<T> const& rhs)
		{
			return !(lhs == rhs);
		}

		template <typename T>
		struct UniqueHandle final : public T
		{
			UniqueHandle() noexcept : T{} { }
			UniqueHandle(UniqueHandle&) = delete;
			constexpr UniqueHandle(T&& rhs) noexcept : T{rhs} { rhs = nullptr; }
			~UniqueHandle()
			{
				if (*this) T::destroy();
			}
			UniqueHandle& operator=(UniqueHandle const&) = delete;
			UniqueHandle& operator=(UniqueHandle&& rhs) { T::_set(rhs); rhs._clear(); return *this; }
		};

		template <typename T>
		struct Opaque : public T
		{
			constexpr Opaque() noexcept : T{} { }
		};

		// ArrayProxy borrowed from Vulkan-Hpp with ♥
		template <typename T>
		class ArrayProxy
		{
			using nonconst_T = typename std::remove_const<T>::type;
			uint32_t m_count;
			T* m_ptr;
		public:
			constexpr ArrayProxy(std::nullptr_t) noexcept : m_count{}, m_ptr{} { }
			ArrayProxy(T& ptr) : m_count{1}, m_ptr(&ptr) { }
			ArrayProxy(uint32_t count, T* ptr) noexcept : m_count{count}, m_ptr{ptr} { }

			template <size_t N>
			ArrayProxy(std::array<nonconst_T, N>& data) noexcept :
				m_count{N}, m_ptr{data.data()} { }

			template <size_t N>
			ArrayProxy(std::array<nonconst_T, N> const& data) noexcept :
				m_count{N} , m_ptr{data.data()} { }

			ArrayProxy(std::initializer_list<T> const& data) noexcept :
				m_count{static_cast<uint32_t>(data.end() - data.begin())},
				m_ptr{data.begin()} { }

#ifdef DK_HPP_SUPPORT_VECTOR
			template <class Allocator = std::allocator<nonconst_T>>
			ArrayProxy(std::vector<nonconst_T, Allocator> & data) noexcept :
				m_count{static_cast<uint32_t>(data.size())},
				m_ptr{data.data()} { }

			template <class Allocator = std::allocator<nonconst_T>>
			ArrayProxy(std::vector<nonconst_T, Allocator> const& data) noexcept :
				m_count{static_cast<uint32_t>(data.size())},
				m_ptr{data.data()} { }
#endif

			const T* begin() const noexcept { return m_ptr; }
			const T* end()   const noexcept { return m_ptr + m_count; }
			const T& front() const noexcept { return *m_ptr; }
			const T& back()  const noexcept { return *(m_ptr + m_count - 1); }
			bool     empty() const noexcept { return m_count == 0; }
			uint32_t size()  const noexcept { return m_count; }
			T*       data()  const noexcept { return m_ptr; }
		};
	}

#define DK_HANDLE_COMMON_MEMBERS(_name) \
	constexpr _name() noexcept : Handle{} { } \
	constexpr _name(std::nullptr_t arg) noexcept : Handle{arg} { } \
	constexpr _name(::Dk##_name arg) noexcept : Handle{arg} { } \
	_name& operator=(std::nullptr_t) noexcept { _clear(); return *this; } \
	void destroy()

#define DK_OPAQUE_COMMON_MEMBERS(_name) \
	constexpr _name() noexcept : Opaque{} { }

	struct Device : public detail::Handle<::DkDevice>
	{
		DK_HANDLE_COMMON_MEMBERS(Device);
	};

	struct MemBlock : public detail::Handle<::DkMemBlock>
	{
		DK_HANDLE_COMMON_MEMBERS(MemBlock);
		void* getCpuAddr();
		DkGpuAddr getGpuAddr();
		uint32_t getSize();
		DkResult flushCpuCache(uint32_t offset, uint32_t size);
		DkResult invalidateCpuCache(uint32_t offset, uint32_t size);
	};

	struct Fence : public detail::Opaque<::DkFence>
	{
		DK_OPAQUE_COMMON_MEMBERS(Fence);
		DkResult wait(int64_t timeout_ns = -1);
	};

	struct CmdBuf : public detail::Handle<::DkCmdBuf>
	{
		DK_HANDLE_COMMON_MEMBERS(CmdBuf);
		void addMemory(DkMemBlock mem, uint32_t offset, uint32_t size);
		DkCmdList finishList();
		void clear();
		void waitFence(DkFence& fence);
		void signalFence(DkFence& fence, bool flush = false);
		void barrier(DkBarrier mode, uint32_t invalidateFlags);
		void bindShader(DkShader const& shader);
		void bindShaders(uint32_t stageMask, detail::ArrayProxy<DkShader const* const> shaders);
		void bindUniformBuffer(DkStage stage, uint32_t id, DkGpuAddr bufAddr, uint32_t bufSize);
		void bindUniformBuffers(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkBufExtents const> buffers);
		void bindStorageBuffer(DkStage stage, uint32_t id, DkGpuAddr bufAddr, uint32_t bufSize);
		void bindStorageBuffers(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkBufExtents const> buffers);
		void bindTexture(DkStage stage, uint32_t id, DkResHandle handle);
		void bindTextures(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkResHandle const> handles);
		void bindImage(DkStage stage, uint32_t id, DkResHandle handle);
		void bindImages(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkResHandle const> handles);
		void bindImageDescriptorSet(DkGpuAddr setAddr, uint32_t numDescriptors);
		void bindSamplerDescriptorSet(DkGpuAddr setAddr, uint32_t numDescriptors);
		void bindRenderTargets(detail::ArrayProxy<DkImageView const* const> colorTargets, DkImageView const* depthTarget = nullptr);
		void bindRasterizerState(DkRasterizerState const& state);
		void bindDepthStencilState(DkDepthStencilState const& state);
		void bindVtxAttribState(detail::ArrayProxy<DkVtxAttribState const> attribs);
		void bindVtxBufferState(detail::ArrayProxy<DkVtxBufferState const> buffers);
		void bindVtxBuffer(uint32_t id, DkGpuAddr bufAddr, uint32_t bufSize);
		void bindVtxBuffers(uint32_t firstId, detail::ArrayProxy<DkBufExtents const> buffers);
		void bindIdxBuffer(DkIdxFormat format, DkGpuAddr address);
		void setViewports(uint32_t firstId, detail::ArrayProxy<DkViewport const> viewports);
		void setScissors(uint32_t firstId, detail::ArrayProxy<DkScissor const> scissors);
		void setDepthBounds(bool enable, float near, float far);
		void setStencil(DkFace face, uint8_t mask, uint8_t funcRef, uint8_t funcMask);
		void setPrimitiveRestart(bool enable, uint32_t index);
		void setTileSize(uint32_t width, uint32_t height);
		void tiledCacheOp(DkTiledCacheOp op);
		void clearColor(uint32_t targetId, uint32_t clearMask, const void* clearData);
		template<typename T> void clearColor(uint32_t targetId, uint32_t clearMask, T red = T{0}, T green = T{0}, T blue = T{0}, T alpha = T{0});
		void clearDepthStencil(bool clearDepth, float depthValue, uint8_t stencilMask, uint8_t stencilValue);
		void discardColor(uint32_t targetId);
		void discardDepthStencil();
		void draw(DkPrimitive prim, uint32_t numVertices, uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance);
		void drawIndirect(DkPrimitive prim, DkGpuAddr indirect);
		void drawIndexed(DkPrimitive prim, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
		void drawIndexedIndirect(DkPrimitive prim, DkGpuAddr indirect);
		void dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ);
		void dispatchComputeIndirect(DkGpuAddr indirect);
		void pushConstants(DkGpuAddr uboAddr, uint32_t uboSize, uint32_t offset, uint32_t size, const void* data);
		void pushData(DkGpuAddr addr, const void* data, uint32_t size);
		void copyBuffer(DkGpuAddr srcAddr, DkGpuAddr dstAddr, uint32_t size);
		void copyImage(DkImageView const& srcView, DkBlitRect const& srcRect, DkImageView const& dstView, DkBlitRect const& dstRect, uint32_t flags = 0);
		void blitImage(DkImageView const& srcView, DkBlitRect const& srcRect, DkImageView const& dstView, DkBlitRect const& dstRect, uint32_t flags = 0, uint32_t factor = 0);
		void resolveImage(DkImageView const& srcView, DkImageView const& dstView);
		void copyBufferToImage(DkCopyBuf const& src, DkImageView const& dstView, DkBlitRect const& dstRect, uint32_t flags = 0);
		void copyImageToBuffer(DkImageView const& srcView, DkBlitRect const& srcRect, DkCopyBuf const& dst, uint32_t flags = 0);
	};

	struct Queue : public detail::Handle<::DkQueue>
	{
		DK_HANDLE_COMMON_MEMBERS(Queue);
		bool isInErrorState();
		void waitFence(DkFence& fence);
		void signalFence(DkFence& fence, bool flush = false);
		void submitCommands(DkCmdList cmds);
		void flush();
		void waitIdle();
		int acquireImage(DkSwapchain swapchain);
		void presentImage(DkSwapchain swapchain, int imageSlot);
	};

	struct Shader : public detail::Opaque<::DkShader>
	{
		DK_OPAQUE_COMMON_MEMBERS(Shader);
		bool isValid() const;
		DkStage getStage() const;
	};

	struct ImageLayout : public detail::Opaque<::DkImageLayout>
	{
		DK_OPAQUE_COMMON_MEMBERS(ImageLayout);
		uint64_t getSize() const;
		uint32_t getAlignment() const;
	};

	struct Image : public detail::Opaque<::DkImage>
	{
		DK_OPAQUE_COMMON_MEMBERS(Image);
		void initialize(ImageLayout const& layout, DkMemBlock memBlock, uint32_t offset);
		DkGpuAddr getGpuAddr() const;
		ImageLayout const& getLayout() const;
	};

	struct Swapchain : public detail::Handle<::DkSwapchain>
	{
		DK_HANDLE_COMMON_MEMBERS(Swapchain);
		void acquireImage(int& imageSlot, DkFence& fence);
		void setCrop(int32_t left, int32_t top, int32_t right, int32_t bottom);
		void setSwapInterval(uint32_t interval);
	};

	struct DeviceMaker : public ::DkDeviceMaker
	{
		DeviceMaker() noexcept : DkDeviceMaker{} { ::dkDeviceMakerDefaults(this); }
		DeviceMaker(DeviceMaker&) = default;
		DeviceMaker(DeviceMaker&&) = default;
		DeviceMaker& setUserData(void* userData) noexcept { this->userData = userData; return *this; }
		DeviceMaker& setCbError(DkErrorFunc cbError) noexcept { this->cbError = cbError; return *this; }
		DeviceMaker& setCbAlloc(DkAllocFunc cbAlloc) noexcept { this->cbAlloc = cbAlloc; return *this; }
		DeviceMaker& setCbFree(DkFreeFunc cbFree) noexcept { this->cbFree = cbFree; return *this; }
		DeviceMaker& setFlags(uint32_t flags) noexcept { this->flags = flags; return *this; }
		Device create();
	};

	struct MemBlockMaker : public ::DkMemBlockMaker
	{
		MemBlockMaker(DkDevice device, uint32_t size) noexcept : DkMemBlockMaker{} { ::dkMemBlockMakerDefaults(this, device, size); }
		MemBlockMaker(MemBlockMaker&) = default;
		MemBlockMaker(MemBlockMaker&&) = default;
		MemBlockMaker& setFlags(uint32_t flags) noexcept { this->flags = flags; return *this; }
		MemBlockMaker& setStorage(void* storage) noexcept { this->storage = storage; return *this; }
		MemBlock create();
	};

	struct CmdBufMaker : public ::DkCmdBufMaker
	{
		CmdBufMaker(DkDevice device) noexcept : DkCmdBufMaker{} { ::dkCmdBufMakerDefaults(this, device); }
		CmdBufMaker& setUserData(void* userData) noexcept { this->userData = userData; return *this; }
		CmdBufMaker& setCbAddMem(DkCmdBufAddMemFunc cbAddMem) noexcept { this->cbAddMem = cbAddMem; return *this; }
		CmdBuf create();
	};

	struct QueueMaker : public ::DkQueueMaker
	{
		QueueMaker(DkDevice device) noexcept : DkQueueMaker{} { ::dkQueueMakerDefaults(this, device); }
		QueueMaker& setFlags(uint32_t flags) noexcept { this->flags = flags; return *this; }
		QueueMaker& setCommandMemorySize(uint32_t commandMemorySize) noexcept { this->commandMemorySize = commandMemorySize; return *this; }
		QueueMaker& setFlushThreshold(uint32_t flushThreshold) noexcept { this->flushThreshold = flushThreshold; return *this; }
		QueueMaker& setPerWarpScratchMemorySize(uint32_t perWarpScratchMemorySize) noexcept { this->perWarpScratchMemorySize = perWarpScratchMemorySize; return *this; }
		QueueMaker& setMaxConcurrentComputeJobs(uint32_t maxConcurrentComputeJobs) noexcept { this->maxConcurrentComputeJobs = maxConcurrentComputeJobs; return *this; }
		Queue create();
	};

	struct ShaderMaker : public ::DkShaderMaker
	{
		ShaderMaker(DkMemBlock codeMem, uint32_t codeOffset) noexcept : DkShaderMaker{} { ::dkShaderMakerDefaults(this, codeMem, codeOffset); }
		ShaderMaker& setControl(const void* control) noexcept { this->control = control; return *this; }
		ShaderMaker& setProgramId(uint32_t programId) noexcept { this->programId = programId; return *this; }
		void initialize(Shader& obj);
	};

	struct ImageLayoutMaker : public ::DkImageLayoutMaker
	{
		ImageLayoutMaker(DkDevice device) noexcept : DkImageLayoutMaker{} { ::dkImageLayoutMakerDefaults(this, device); }
		ImageLayoutMaker& setType(DkImageType type) noexcept { this->type = type; return *this; }
		ImageLayoutMaker& setFlags(uint32_t flags) noexcept { this->flags = flags; return *this; }
		ImageLayoutMaker& setFormat(DkImageFormat format) noexcept { this->format = format; return *this; }
		ImageLayoutMaker& setMsMode(DkMsMode msMode) noexcept { this->msMode = msMode; return *this; }
		ImageLayoutMaker& setDimensions(uint32_t width, uint32_t height = 0, uint32_t depth = 0) noexcept
		{
			this->dimensions[0] = width;
			this->dimensions[1] = height;
			this->dimensions[2] = depth;
			return *this;
		}
		ImageLayoutMaker& setMipLevels(uint32_t mipLevels) noexcept { this->mipLevels = mipLevels; return *this; }
		ImageLayoutMaker& setPitchStride(uint32_t pitchStride) noexcept { this->pitchStride = pitchStride; return *this; }
		ImageLayoutMaker& setTileSize(DkTileSize tileSize) noexcept { this->tileSize = tileSize; return *this; }
		void initialize(ImageLayout& obj);
	};

	struct ImageView : public ::DkImageView
	{
		ImageView(Image const& image) noexcept : DkImageView{} { ::dkImageViewDefaults(this, &image); }
		void setType(DkImageType type = DkImageType_None) noexcept { this->type = type; }
		void setFormat(DkImageFormat format = DkImageFormat_None) noexcept { this->format = format; }
		void setSwizzle(DkSwizzle x = DkSwizzle_Red, DkSwizzle y = DkSwizzle_Green, DkSwizzle z = DkSwizzle_Blue, DkSwizzle w = DkSwizzle_Alpha) noexcept
		{
			this->swizzle[0] = x;
			this->swizzle[1] = y;
			this->swizzle[2] = z;
			this->swizzle[3] = w;
		}
		void setDsSource(DkDsSource dsSource) noexcept { this->dsSource = dsSource; }
		void setLayers(uint16_t layerOffset = 0, uint16_t layerCount = 0) noexcept
		{
			this->layerOffset = layerOffset;
			this->layerCount = layerCount;
		}
		void setMipLevels(uint8_t mipLevelOffset = 0, uint8_t mipLevelCount = 0) noexcept
		{
			this->mipLevelOffset = mipLevelOffset;
			this->mipLevelCount = mipLevelCount;
		}
	};

	struct ImageDescriptor : public detail::Opaque<::DkImageDescriptor>
	{
		DK_OPAQUE_COMMON_MEMBERS(ImageDescriptor);
		void initialize(ImageView const& view, bool usesLoadOrStore = false, bool decayMS = false);
	};

	struct Sampler : public ::DkSampler
	{
		Sampler() noexcept : DkSampler{} { ::dkSamplerDefaults(this); }
		Sampler& setFilter(DkFilter min = DkFilter_Nearest, DkFilter mag = DkFilter_Nearest, DkMipFilter mip = DkMipFilter_None)
		{
			this->minFilter = min;
			this->magFilter = mag;
			this->mipFilter = mip;
			return *this;
		}
		Sampler& setWrapMode(DkWrapMode u = DkWrapMode_Repeat, DkWrapMode v = DkWrapMode_Repeat, DkWrapMode p = DkWrapMode_Repeat)
		{
			this->wrapMode[0] = u;
			this->wrapMode[1] = v;
			this->wrapMode[2] = p;
			return *this;
		}
		Sampler& setLodClamp(float min, float max)
		{
			this->lodClampMin = min;
			this->lodClampMax = max;
			return *this;
		}
		Sampler& setLodBias(float bias) { this->lodBias = bias; return *this; }
		Sampler& setLodSnap(float snap) { this->lodSnap = snap; return *this; }
		Sampler& setDepthCompare(bool enable, DkCompareOp op = DkCompareOp_Less)
		{
			this->compareEnable = enable;
			this->compareOp = op;
			return *this;
		}
		Sampler& setBorderColor(float r, float g, float b, float a)
		{
			this->borderColor[0].value_f = r;
			this->borderColor[1].value_f = g;
			this->borderColor[2].value_f = b;
			this->borderColor[3].value_f = a;
			return *this;
		}
		Sampler& setBorderColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
		{
			this->borderColor[0].value_ui = r;
			this->borderColor[1].value_ui = g;
			this->borderColor[2].value_ui = b;
			this->borderColor[3].value_ui = a;
			return *this;
		}
		Sampler& setBorderColor(int32_t r, int32_t g, int32_t b, int32_t a)
		{
			this->borderColor[0].value_i = r;
			this->borderColor[1].value_i = g;
			this->borderColor[2].value_i = b;
			this->borderColor[3].value_i = a;
			return *this;
		}
		Sampler& setMaxAnisotropy(float max) { this->maxAnisotropy = max; return *this; }
		Sampler& setReductionMode(DkSamplerReduction mode) { this->reductionMode = mode; return *this; }
	};

	struct SamplerDescriptor : public detail::Opaque<::DkSamplerDescriptor>
	{
		DK_OPAQUE_COMMON_MEMBERS(SamplerDescriptor);
		void initialize(Sampler const& sampler);
	};

	struct RasterizerState : public ::DkRasterizerState
	{
		RasterizerState() : DkRasterizerState{} { ::dkRasterizerStateDefaults(this); }
		RasterizerState& setDepthClampEnable(bool enable) { this->depthClampEnable = enable; return *this; }
		RasterizerState& setRasterizerDiscardEnable(bool enable) { this->rasterizerDiscardEnable = enable; return *this; }
		RasterizerState& setPolygonMode(DkPolygonMode polygonMode) { this->polygonMode = polygonMode; return *this; }
		RasterizerState& setCullMode(DkFace cullMode) { this->cullMode = cullMode; return *this; }
		RasterizerState& setFrontFace(DkFrontFace frontFace) { this->frontFace = frontFace; return *this; }
		RasterizerState& setDepthBiasEnable(bool enable) { this->depthBiasEnable = enable; return *this; }
		RasterizerState& setDepthBiasConstantFactor(float value) { this->depthBiasConstantFactor = value; return *this; }
		RasterizerState& setDepthBiasClamp(float value) { this->depthBiasClamp = value; return *this; }
		RasterizerState& setDepthBiasSlopeFactor(float value) { this->depthBiasSlopeFactor = value; return *this; }
		RasterizerState& setLineWidth(float width) { this->lineWidth = width; return *this; }
	};

	struct DepthStencilState : public ::DkDepthStencilState
	{
		DepthStencilState() : DkDepthStencilState{} { ::dkDepthStencilStateDefaults(this); }
		DepthStencilState& setDepthTestEnable(bool enable) { this->depthTestEnable = enable; return *this; }
		DepthStencilState& setDepthWriteEnable(bool enable) { this->depthWriteEnable = enable; return *this; }
		DepthStencilState& setStencilTestEnable(bool enable) { this->stencilTestEnable = enable; return *this; }
		DepthStencilState& setDepthCompareOp(DkCompareOp op) { this->depthCompareOp = op; return *this; }
		DepthStencilState& setStencilFrontFailOp(DkStencilOp op) { this->stencilFrontFailOp = op; return *this; }
		DepthStencilState& setStencilFrontPassOp(DkStencilOp op) { this->stencilFrontPassOp = op; return *this; }
		DepthStencilState& setStencilFrontDepthFailOp(DkStencilOp op) { this->stencilFrontDepthFailOp = op; return *this; }
		DepthStencilState& setStencilFrontCompareOp(DkCompareOp op) { this->stencilFrontCompareOp = op; return *this; }
		DepthStencilState& setStencilBackFailOp(DkStencilOp op) { this->stencilBackFailOp = op; return *this; }
		DepthStencilState& setStencilBackPassOp(DkStencilOp op) { this->stencilBackPassOp = op; return *this; }
		DepthStencilState& setStencilBackDepthFailOp(DkStencilOp op) { this->stencilBackDepthFailOp = op; return *this; }
		DepthStencilState& setStencilBackCompareOp(DkCompareOp op) { this->stencilBackCompareOp = op; return *this; }
	};

	struct SwapchainMaker : public ::DkSwapchainMaker
	{
		SwapchainMaker(DkDevice device, void* nativeWindow, DkImage const* const pImages[], uint32_t numImages) noexcept : DkSwapchainMaker{} { ::dkSwapchainMakerDefaults(this, device, nativeWindow, pImages, numImages); }
		Swapchain create();
	};

	inline Device DeviceMaker::create()
	{
		return Device{::dkDeviceCreate(this)};
	}

	inline void Device::destroy()
	{
		::dkDeviceDestroy(*this);
		_clear();
	}

	inline MemBlock MemBlockMaker::create()
	{
		return MemBlock{::dkMemBlockCreate(this)};
	}

	inline void MemBlock::destroy()
	{
		::dkMemBlockDestroy(*this);
		_clear();
	}

	inline void* MemBlock::getCpuAddr()
	{
		return ::dkMemBlockGetCpuAddr(*this);
	}

	inline DkGpuAddr MemBlock::getGpuAddr()
	{
		return ::dkMemBlockGetGpuAddr(*this);
	}

	inline uint32_t MemBlock::getSize()
	{
		return ::dkMemBlockGetSize(*this);
	}

	inline DkResult MemBlock::flushCpuCache(uint32_t offset, uint32_t size)
	{
		return ::dkMemBlockFlushCpuCache(*this, offset, size);
	}

	inline DkResult MemBlock::invalidateCpuCache(uint32_t offset, uint32_t size)
	{
		return ::dkMemBlockInvalidateCpuCache(*this, offset, size);
	}

	inline DkResult Fence::wait(int64_t timeout_ns)
	{
		return ::dkFenceWait(this, timeout_ns);
	}

	inline CmdBuf CmdBufMaker::create()
	{
		return CmdBuf{::dkCmdBufCreate(this)};
	}

	inline void CmdBuf::destroy()
	{
		::dkCmdBufDestroy(*this);
		_clear();
	}

	inline void CmdBuf::addMemory(DkMemBlock mem, uint32_t offset, uint32_t size)
	{
		::dkCmdBufAddMemory(*this, mem, offset, size);
	}

	inline DkCmdList CmdBuf::finishList()
	{
		return ::dkCmdBufFinishList(*this);
	}

	inline void CmdBuf::clear()
	{
		::dkCmdBufClear(*this);
	}

	inline void CmdBuf::waitFence(DkFence& fence)
	{
		return ::dkCmdBufWaitFence(*this, &fence);
	}

	inline void CmdBuf::signalFence(DkFence& fence, bool flush)
	{
		return ::dkCmdBufSignalFence(*this, &fence, flush);
	}

	inline void CmdBuf::barrier(DkBarrier mode, uint32_t invalidateFlags)
	{
		::dkCmdBufBarrier(*this, mode, invalidateFlags);
	}

	inline void CmdBuf::bindShader(DkShader const& shader)
	{
		::dkCmdBufBindShader(*this, &shader);
	}

	inline void CmdBuf::bindShaders(uint32_t stageMask, detail::ArrayProxy<DkShader const* const> shaders)
	{
		::dkCmdBufBindShaders(*this, stageMask, shaders.data(), shaders.size());
	}

	inline void CmdBuf::bindUniformBuffer(DkStage stage, uint32_t id, DkGpuAddr bufAddr, uint32_t bufSize)
	{
		::dkCmdBufBindUniformBuffer(*this, stage, id, bufAddr, bufSize);
	}

	inline void CmdBuf::bindUniformBuffers(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkBufExtents const> buffers)
	{
		::dkCmdBufBindUniformBuffers(*this, stage, firstId, buffers.data(), buffers.size());
	}

	inline void CmdBuf::bindStorageBuffer(DkStage stage, uint32_t id, DkGpuAddr bufAddr, uint32_t bufSize)
	{
		::dkCmdBufBindStorageBuffer(*this, stage, id, bufAddr, bufSize);
	}

	inline void CmdBuf::bindStorageBuffers(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkBufExtents const> buffers)
	{
		::dkCmdBufBindStorageBuffers(*this, stage, firstId, buffers.data(), buffers.size());
	}

	inline void CmdBuf::bindTexture(DkStage stage, uint32_t id, DkResHandle handle)
	{
		::dkCmdBufBindTexture(*this, stage, id, handle);
	}

	inline void CmdBuf::bindTextures(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkResHandle const> handles)
	{
		::dkCmdBufBindTextures(*this, stage, firstId, handles.data(), handles.size());
	}

	inline void CmdBuf::bindImage(DkStage stage, uint32_t id, DkResHandle handle)
	{
		::dkCmdBufBindImage(*this, stage, id, handle);
	}

	inline void CmdBuf::bindImages(DkStage stage, uint32_t firstId, detail::ArrayProxy<DkResHandle const> handles)
	{
		::dkCmdBufBindImages(*this, stage, firstId, handles.data(), handles.size());
	}

	inline void CmdBuf::bindImageDescriptorSet(DkGpuAddr setAddr, uint32_t numDescriptors)
	{
		::dkCmdBufBindImageDescriptorSet(*this, setAddr, numDescriptors);
	}

	inline void CmdBuf::bindSamplerDescriptorSet(DkGpuAddr setAddr, uint32_t numDescriptors)
	{
		::dkCmdBufBindSamplerDescriptorSet(*this, setAddr, numDescriptors);
	}

	inline void CmdBuf::bindRenderTargets(detail::ArrayProxy<DkImageView const* const> colorTargets, DkImageView const* depthTarget)
	{
		::dkCmdBufBindRenderTargets(*this, colorTargets.data(), colorTargets.size(), depthTarget);
	}

	inline void CmdBuf::bindRasterizerState(DkRasterizerState const& state)
	{
		::dkCmdBufBindRasterizerState(*this, &state);
	}

	inline void CmdBuf::bindDepthStencilState(DkDepthStencilState const& state)
	{
		::dkCmdBufBindDepthStencilState(*this, &state);
	}

	inline void CmdBuf::bindVtxAttribState(detail::ArrayProxy<DkVtxAttribState const> attribs)
	{
		::dkCmdBufBindVtxAttribState(*this, attribs.data(), attribs.size());
	}

	inline void CmdBuf::bindVtxBufferState(detail::ArrayProxy<DkVtxBufferState const> buffers)
	{
		::dkCmdBufBindVtxBufferState(*this, buffers.data(), buffers.size());
	}

	inline void CmdBuf::bindVtxBuffer(uint32_t id, DkGpuAddr bufAddr, uint32_t bufSize)
	{
		::dkCmdBufBindVtxBuffer(*this, id, bufAddr, bufSize);
	}

	inline void CmdBuf::bindVtxBuffers(uint32_t firstId, detail::ArrayProxy<DkBufExtents const> buffers)
	{
		::dkCmdBufBindVtxBuffers(*this, firstId, buffers.data(), buffers.size());
	}

	inline void CmdBuf::bindIdxBuffer(DkIdxFormat format, DkGpuAddr address)
	{
		::dkCmdBufBindIdxBuffer(*this, format, address);
	}

	inline void CmdBuf::setViewports(uint32_t firstId, detail::ArrayProxy<DkViewport const> viewports)
	{
		::dkCmdBufSetViewports(*this, firstId, viewports.data(), viewports.size());
	}

	inline void CmdBuf::setScissors(uint32_t firstId, detail::ArrayProxy<DkScissor const> scissors)
	{
		::dkCmdBufSetScissors(*this, firstId, scissors.data(), scissors.size());
	}

	inline void CmdBuf::setDepthBounds(bool enable, float near, float far)
	{
		::dkCmdBufSetDepthBounds(*this, enable, near, far);
	}

	inline void CmdBuf::setStencil(DkFace face, uint8_t mask, uint8_t funcRef, uint8_t funcMask)
	{
		::dkCmdBufSetStencil(*this, face, mask, funcRef, funcMask);
	}

	inline void CmdBuf::setPrimitiveRestart(bool enable, uint32_t index)
	{
		::dkCmdBufSetPrimitiveRestart(*this, enable, index);
	}

	inline void CmdBuf::setTileSize(uint32_t width, uint32_t height)
	{
		::dkCmdBufSetTileSize(*this, width, height);
	}

	inline void CmdBuf::tiledCacheOp(DkTiledCacheOp op)
	{
		::dkCmdBufTiledCacheOp(*this, op);
	}

	inline void CmdBuf::clearColor(uint32_t targetId, uint32_t clearMask, const void* clearData)
	{
		::dkCmdBufClearColor(*this, targetId, clearMask, clearData);
	}

	template <typename T>
	inline void CmdBuf::clearColor(uint32_t targetId, uint32_t clearMask, T red, T green, T blue, T alpha)
	{
		static_assert(sizeof(T) == 4, "Bad size for T");
		T data[] = { red, green, blue, alpha };
		::dkCmdBufClearColor(*this, targetId, clearMask, data);
	}

	inline void CmdBuf::clearDepthStencil(bool clearDepth, float depthValue, uint8_t stencilMask, uint8_t stencilValue)
	{
		::dkCmdBufClearDepthStencil(*this, clearDepth, depthValue, stencilMask, stencilValue);
	}

	inline void CmdBuf::discardColor(uint32_t targetId)
	{
		::dkCmdBufDiscardColor(*this, targetId);
	}

	inline void CmdBuf::discardDepthStencil()
	{
		::dkCmdBufDiscardDepthStencil(*this);
	}

	inline void CmdBuf::draw(DkPrimitive prim, uint32_t numVertices, uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance)
	{
		::dkCmdBufDraw(*this, prim, numVertices, numInstances, firstVertex, firstInstance);
	}

	inline void CmdBuf::drawIndirect(DkPrimitive prim, DkGpuAddr indirect)
	{
		::dkCmdBufDrawIndirect(*this, prim, indirect);
	}

	inline void CmdBuf::drawIndexed(DkPrimitive prim, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		::dkCmdBufDrawIndexed(*this, prim, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	inline void CmdBuf::drawIndexedIndirect(DkPrimitive prim, DkGpuAddr indirect)
	{
		::dkCmdBufDrawIndexedIndirect(*this, prim, indirect);
	}

	inline void CmdBuf::dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
	{
		::dkCmdBufDispatchCompute(*this, numGroupsX, numGroupsY, numGroupsZ);
	}

	inline void CmdBuf::dispatchComputeIndirect(DkGpuAddr indirect)
	{
		::dkCmdBufDispatchComputeIndirect(*this, indirect);
	}

	inline void CmdBuf::pushConstants(DkGpuAddr uboAddr, uint32_t uboSize, uint32_t offset, uint32_t size, const void* data)
	{
		::dkCmdBufPushConstants(*this, uboAddr, uboSize, offset, size, data);
	}

	inline void CmdBuf::pushData(DkGpuAddr addr, const void* data, uint32_t size)
	{
		::dkCmdBufPushData(*this, addr, data, size);
	}

	inline void CmdBuf::copyBuffer(DkGpuAddr srcAddr, DkGpuAddr dstAddr, uint32_t size)
	{
		::dkCmdBufCopyBuffer(*this, srcAddr, dstAddr, size);
	}

	inline void CmdBuf::copyImage(DkImageView const& srcView, DkBlitRect const& srcRect, DkImageView const& dstView, DkBlitRect const& dstRect, uint32_t flags)
	{
		::dkCmdBufCopyImage(*this, &srcView, &srcRect, &dstView, &dstRect, flags);
	}

	inline void CmdBuf::blitImage(DkImageView const& srcView, DkBlitRect const& srcRect, DkImageView const& dstView, DkBlitRect const& dstRect, uint32_t flags, uint32_t factor)
	{
		::dkCmdBufBlitImage(*this, &srcView, &srcRect, &dstView, &dstRect, flags, factor);
	}

	inline void CmdBuf::resolveImage(DkImageView const& srcView, DkImageView const& dstView)
	{
		::dkCmdBufResolveImage(*this, &srcView, &dstView);
	}

	inline void CmdBuf::copyBufferToImage(DkCopyBuf const& src, DkImageView const& dstView, DkBlitRect const& dstRect, uint32_t flags)
	{
		::dkCmdBufCopyBufferToImage(*this, &src, &dstView, &dstRect, flags);
	}

	inline void CmdBuf::copyImageToBuffer(DkImageView const& srcView, DkBlitRect const& srcRect, DkCopyBuf const& dst, uint32_t flags)
	{
		::dkCmdBufCopyImageToBuffer(*this, &srcView, &srcRect, &dst, flags);
	}

	inline Queue QueueMaker::create()
	{
		return Queue{::dkQueueCreate(this)};
	}

	inline void Queue::destroy()
	{
		::dkQueueDestroy(*this);
		_clear();
	}

	inline bool Queue::isInErrorState()
	{
		return ::dkQueueIsInErrorState(*this);
	}

	inline void Queue::waitFence(DkFence& fence)
	{
		::dkQueueWaitFence(*this, &fence);
	}

	inline void Queue::signalFence(DkFence& fence, bool flush)
	{
		::dkQueueSignalFence(*this, &fence, flush);
	}

	inline void Queue::submitCommands(DkCmdList cmds)
	{
		::dkQueueSubmitCommands(*this, cmds);
	}

	inline void Queue::flush()
	{
		::dkQueueFlush(*this);
	}

	inline void Queue::waitIdle()
	{
		::dkQueueWaitIdle(*this);
	}

	inline int Queue::acquireImage(DkSwapchain swapchain)
	{
		return ::dkQueueAcquireImage(*this, swapchain);
	}

	inline void Queue::presentImage(DkSwapchain swapchain, int imageSlot)
	{
		::dkQueuePresentImage(*this, swapchain, imageSlot);
	}

	inline void ShaderMaker::initialize(Shader& obj)
	{
		::dkShaderInitialize(&obj, this);
	}

	inline bool Shader::isValid() const
	{
		return ::dkShaderIsValid(this);
	}

	inline DkStage Shader::getStage() const
	{
		return ::dkShaderGetStage(this);
	}

	inline void ImageLayoutMaker::initialize(ImageLayout& obj)
	{
		::dkImageLayoutInitialize(&obj, this);
	}

	inline uint64_t ImageLayout::getSize() const
	{
		return ::dkImageLayoutGetSize(this);
	}

	inline uint32_t ImageLayout::getAlignment() const
	{
		return ::dkImageLayoutGetAlignment(this);
	}

	inline void Image::initialize(ImageLayout const& layout, DkMemBlock memBlock, uint32_t offset)
	{
		::dkImageInitialize(this, &layout, memBlock, offset);
	}

	inline DkGpuAddr Image::getGpuAddr() const
	{
		return ::dkImageGetGpuAddr(this);
	}

	inline ImageLayout const& Image::getLayout() const
	{
		return *static_cast<ImageLayout const*>(::dkImageGetLayout(this));
	}

	inline void ImageDescriptor::initialize(ImageView const& view, bool usesLoadOrStore, bool decayMS)
	{
		::dkImageDescriptorInitialize(this, &view, usesLoadOrStore, decayMS);
	}

	inline void SamplerDescriptor::initialize(Sampler const& sampler)
	{
		::dkSamplerDescriptorInitialize(this, &sampler);
	}

	inline Swapchain SwapchainMaker::create()
	{
		return Swapchain{::dkSwapchainCreate(this)};
	}

	inline void Swapchain::destroy()
	{
		::dkSwapchainDestroy(*this);
		_clear();
	}

	inline void Swapchain::acquireImage(int& imageSlot, DkFence& fence)
	{
		::dkSwapchainAcquireImage(*this, &imageSlot, &fence);
	}

	inline void Swapchain::setCrop(int32_t left, int32_t top, int32_t right, int32_t bottom)
	{
		::dkSwapchainSetCrop(*this, left, top, right, bottom);
	}

	inline void Swapchain::setSwapInterval(uint32_t interval)
	{
		::dkSwapchainSetSwapInterval(*this, interval);
	}

	using UniqueDevice = detail::UniqueHandle<Device>;
	using UniqueMemBlock = detail::UniqueHandle<MemBlock>;
	using UniqueCmdBuf = detail::UniqueHandle<CmdBuf>;
	using UniqueQueue = detail::UniqueHandle<Queue>;
	using UniqueSwapchain = detail::UniqueHandle<Swapchain>;
}
