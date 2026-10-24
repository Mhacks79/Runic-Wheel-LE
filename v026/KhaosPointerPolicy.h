#pragma once
// Copyright (c) 2026 Mhacks79. Original Runic Wheel LE model.
// Pure helpers for the KHA-OS panel pointer: coordinate mapping and visibility.
namespace rw26ui {
struct Mapping { float sx=1.f, sy=1.f; };
// Mouse messages arrive in window client pixels; the panel is drawn in back-buffer
// pixels. They differ with a scaled window, DPI virtualisation or a resized buffer.
inline Mapping client_to_buffer(float clientW,float clientH,float bufferW,float bufferH){
    Mapping m{};
    if(clientW>=64.f && clientH>=64.f && bufferW>=64.f && bufferH>=64.f){
        m.sx=bufferW/clientW;m.sy=bufferH/clientH;
        if(m.sx<0.25f||m.sx>4.f||m.sy<0.25f||m.sy>4.f){m.sx=1.f;m.sy=1.f;}
    }
    return m;
}
// CURSORINFO.flags: CURSOR_SHOWING=1, CURSOR_SUPPRESSED=2 (touch/pen, Windows 8+).
inline bool os_pointer_visible(unsigned long flags,bool hasCursor){
    return (flags&1u)!=0u && (flags&2u)==0u && hasCursor;
}
}
