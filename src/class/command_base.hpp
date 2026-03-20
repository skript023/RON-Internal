#pragma once

#pragma pack(push, 1)

class CommandBase
{
public:
    char pad_0000[0x1E8];
    ID3D12CommandQueue* m_command_base;
};

class CommandContext
{
public:
    char pad_0000[0x50]; //offset added by 0x2 from 0x48
    CommandBase* m_command_subsystem;
};

class RootCommand
{
public:
    char pad_0000[0x20];
    CommandContext* m_command_context;
};

#pragma pack(pop)