#pragma once

#include <algorithm>

using std::min;
using std::max;

#include <dshow.h>
#include <driverspecs.h>
#include <ks.h>
#include <ksmedia.h>
#include <initguid.h>

#ifndef __inout_opt
#define __inout_opt
#endif

#ifndef __deref_out
#define __deref_out
#endif

#ifndef __deref_in
#define __deref_in
#endif

#ifndef __deref_inout_opt
#define __deref_inout_opt
#endif

#ifndef __in_opt
#define __in_opt
#endif

#ifndef __out_opt
#define __out_opt
#endif

#ifndef __deref_out_opt
#define __deref_out_opt
#endif

#ifndef __out_ecount_part
#define __out_ecount_part(size,length)
#endif

#ifndef __field_ecount_opt
#define __field_ecount_opt(buffer)
#endif

#ifndef __in_bcount_opt
#define __in_bcount_opt(size)
#endif

#ifndef __in_ecount_opt
#define __in_ecount_opt(size)
#endif

#ifndef __inout_ecount_full
#define __inout_ecount_full(size)
#endif

#ifndef __control_entrypoint
#define __control_entrypoint(category)
#endif

#ifndef __success
#define __success(expr)
#endif

#ifndef __format_string
#define __format_string
#endif

#ifndef __deref_out_range
#define __deref_out_range(x, y)
#endif

#ifndef __range
#define __range(x, y)
#endif

#ifndef __out_range
#define __out_range(x, y)
#endif

#ifndef _Deref_out_range_
#define _Deref_out_range_(x, y)
#endif

#ifndef DIBSIZE
#define DIBSIZE KS_DIBSIZE
#endif

#ifndef SIZE_PREHEADER
#define SIZE_PREHEADER KS_SIZE_PREHEADER
#endif

#ifndef SIZE_MASKS
#define SIZE_MASKS KS_SIZE_MASKS
#endif

#ifndef PALETTISED
#define PALETTISED(PBMIH) ((PBMIH)->bmiHeader.biBitCount <= iPALETTE)
#endif

#ifndef PALETTE_ENTRIES
#define PALETTE_ENTRIES(pbmi) ((DWORD) 1 << (pbmi)->bmiHeader.biBitCount)
#endif

#ifndef TRUECOLOR
#define TRUECOLOR(PBMIH) ((TRUECOLORINFO *)(((LPBYTE)&((PBMIH)->bmiHeader)) + (PBMIH)->bmiHeader.biSize))
#endif

#ifndef COLORS
#define COLORS(PBMIH) ((RGBQUAD *)(((LPBYTE)&((PBMIH)->bmiHeader)) + (PBMIH)->bmiHeader.biSize))
#endif

#ifndef BITMASKS
#define BITMASKS(pbmi) ((DWORD *)(((LPBYTE)&((pbmi)->bmiHeader)) + (pbmi)->bmiHeader.biSize))
#endif

#ifndef HEADER
#define HEADER(pVideoInfo) (&(((VIDEOINFOHEADER *) (pVideoInfo))->bmiHeader))
#endif

#ifndef WIDTHBYTES
#define WIDTHBYTES(BTIS)  ((DWORD)(((BTIS)+31) & (~31)) / 8)
#endif

#ifndef DIBWIDTHBYTES
#define DIBWIDTHBYTES(BI) ((DWORD)(BI).biBitCount) * (DWORD)WIDTHBYTES((DWORD)(BI).biWidth)
#endif

#ifndef SIZE_PALETTE
#define SIZE_PALETTE (iPALETTE_COLORS * sizeof(RGBQUAD))
#endif

#ifndef SIZE_VIDEOHEADER
#define SIZE_VIDEOHEADER (sizeof(BITMAPINFOHEADER) + SIZE_PREHEADER)
#endif

#ifndef DWordAdd

#define ULONG_ERROR 0xffffffffUL
#define INTSAFE_E_ARITHMETIC_OVERFLOW STATUS_INTEGER_OVERFLOW

//
// ULONG addition
//
__forceinline
__checkReturn
HRESULT
ULongAdd(
    __in ULONG ulAugend,
    __in ULONG ulAddend,
    __out ULONG* pulResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pulResult = ULONG_ERROR;

    if ((unsigned long)(ulAugend + ulAddend) >= ulAugend)
    {
        *pulResult = (ulAugend + ulAddend);
        hr = S_OK;
    }

    return hr;
}

#define DWordAdd ULongAdd
#endif

#ifndef SAFE_DIBSIZE

#define INTSAFE_ULONG_MAX 0xffffffffUL

//
// ULONGLONG -> ULONG conversion
//
__forceinline
__checkReturn
HRESULT
ULongLongToULong(
    __in ULONGLONG ullOperand,
    __out ULONG* pulResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pulResult = ULONG_ERROR;

    if (ullOperand <= INTSAFE_ULONG_MAX)
    {
        *pulResult = ULONG(ullOperand);
        hr = S_OK;
    }

    return hr;
}

//
// ULONG multiplication
//
__forceinline
__checkReturn
HRESULT
ULongMult(
    __in ULONG ulMultiplicand,
    __in ULONG ulMultiplier,
    __out ULONG* pulResult)
{
    ULONGLONG ull64Result = UInt32x32To64(ulMultiplicand, ulMultiplier);

    return ULongLongToULong(ull64Result, pulResult);
}

//
// DWORD multiplication
//
#define DWordMult ULongMult

__inline HRESULT SAFE_DIBWIDTHBYTES(__in const BITMAPINFOHEADER *pbi, __out DWORD *pcbWidth)
{
    DWORD dw;
    HRESULT hr;
    if (pbi->biWidth < 0 || pbi->biBitCount <= 0) {
        return E_INVALIDARG;
    }
    //  Calculate width in bits
    hr = DWordMult(DWORD(pbi->biWidth), DWORD(pbi->biBitCount), &dw);
    if (FAILED(hr)) {
        return hr;
    }
    //  Round up to bytes
    dw = (dw & 7) ? dw / 8 + 1: dw / 8;

    //  Round up to a multiple of 4 bytes
    if (dw & 3) {
        dw += 4 - (dw & 3);
    }

    *pcbWidth = dw;
    return S_OK;
}

__inline HRESULT SAFE_DIBSIZE(__in const BITMAPINFOHEADER *pbi, __out DWORD *pcbSize)
{
    DWORD dw;
    DWORD dwWidthBytes;
    HRESULT hr;
    if (pbi->biHeight == LONG(0x80000000)) {
        return E_INVALIDARG;
    }
    hr = SAFE_DIBWIDTHBYTES(pbi, &dwWidthBytes);
    if (FAILED(hr)) {
        return hr;
    }
    dw = abs(pbi->biHeight);
    hr = DWordMult(dw, dwWidthBytes, &dw);
    if (FAILED(hr)) {
        return hr;
    }
    *pcbSize = dw;
    return S_OK;
}
#endif

#define UNUSED(x) (void)(x);

STDAPI_(BOOL) ContainsPalette(const VIDEOINFOHEADER *pVideoInfo);
STDAPI_(const RGBQUAD *) GetBitmapPalette(const VIDEOINFOHEADER *pVideoInfo);
