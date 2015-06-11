/*
    Copyright (c) 2015, namreeb (legal@namreeb.org)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    The views and conclusions contained in the software and documentation are those
    of the authors and should not be interpreted as representing official policies,
    either expressed or implied, of the FreeBSD Project.
*/

#include <memory>

#include <hadesmem/patcher.hpp>

#include "misc.hpp"
#include "offsets.hpp"

double(__fastcall *LuaToNumber)(PVOID, unsigned int);

void RegisterLuaFunction()
{
    // note: there are many suitable locations for this trampoline in the 1.12.1 client.
    // this offset is what you would change if you care to use a different one.
    const DWORD trampolineAddress = Offsets::gTrampoline;
    const hadesmem::Process process(::GetCurrentProcessId());

    std::vector<BYTE> patch(5);

    patch[0] = 0xE9;    // JMP

    const DWORD castSpellLocation = hadesmem::detail::AliasCast<DWORD>(&CastSpellAtTarget);
    const DWORD relativeJumpValue = castSpellLocation - trampolineAddress - 5;

    memcpy(&patch[1], &relativeJumpValue, sizeof(DWORD));

    // write JMP to wow's .text section so it can be registered with lua
    auto trampoline = new hadesmem::PatchRaw(process, (PVOID)trampolineAddress, patch);
    trampoline->Apply();

    // register with lua
    FrameScriptRegisterT frameScriptRegister = hadesmem::detail::AliasCast<decltype(frameScriptRegister)>(Offsets::FrameScript__Register);
    frameScriptRegister("CastSpellAtTarget", trampolineAddress);
}

void LuaLoadScripts(hadesmem::PatchDetourBase *detour)
{
    LuaLoadScriptsT originalRegister = detour->GetTrampolineT<decltype(originalRegister)>();
    (*originalRegister)();

    RegisterLuaFunction();
}