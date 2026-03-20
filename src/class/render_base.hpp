#pragma once

#pragma pack(push, 1)

class CommandQueueWrapper
{
public:
    char pad_0000[0x168];
    ID3D12CommandQueue* m_command_queue;
};

class SwapchainWrapper
{
public:
    char pad_0000[0x60]; //0x0000
    IDXGISwapChain3* m_swapchain; //0x0060
	char pad_0068[8]; //0x0068
    CommandQueueWrapper* m_command_queue_wrapper; //0x0070
};

class URenderingContext
{
public:
    char pad_0000[0xA0];
    SwapchainWrapper* m_swapchain_wrapper;
};

class URendererSubSystem
{
public:
    char pad_0000[0xE30];
    URenderingContext* m_renderer_context;
};

class GameInstance
{
public:
    char pad_0000[0x20];
    URendererSubSystem* m_renderer_subsystem;
};

class GWorld
{
public:
    char pad_0000[0x190];
    GameInstance* m_game_instance;
};

#pragma pack(pop)