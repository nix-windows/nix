#pragma once
/**
 * @file
 *
 * This struct isn't defined in the normal Windows SDK, but only in the Windows Driver Kit.
 *
 * I (@Ericson2314) would not normally do something like this, but LLVM
 * has decided that this is in fact stable, per
 * https://github.com/llvm/llvm-project/blob/main/libcxx/src/filesystem/posix_compat.h,
 * so I guess that is good enough for us. GCC doesn't support symlinks
 * at all on windows so we have to put it here, not grab it from private
 * c++ standard library headers anyways.
 */

namespace nix::windows {

struct ReparseDataBuffer
{
    unsigned long ReparseTag;
    unsigned short ReparseDataLength;
    unsigned short Reserved;

    union
    {
        struct
        {
            unsigned short SubstituteNameOffset;
            unsigned short SubstituteNameLength;
            unsigned short PrintNameOffset;
            unsigned short PrintNameLength;
            unsigned long Flags;
            wchar_t PathBuffer[1];
        } SymbolicLinkReparseBuffer;

        struct
        {
            unsigned short SubstituteNameOffset;
            unsigned short SubstituteNameLength;
            unsigned short PrintNameOffset;
            unsigned short PrintNameLength;
            wchar_t PathBuffer[1];
        } MountPointReparseBuffer;

        struct
        {
            unsigned char DataBuffer[1];
        } GenericReparseBuffer;
    };
};

} // namespace nix::windows
