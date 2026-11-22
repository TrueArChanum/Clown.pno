#!/usr/bin/env bash
set -euo pipefail

# Part of Penetrizzel
# Copyright (c) 2026 ArChanum

GAME="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$GAME/.." && pwd)"
CORE="$GAME/clownmdemu-core"
FRONTEND="$GAME/frontend"
BUILD="$GAME/build"
OBJDIR="$BUILD/obj"
CC="${CC:-gcc}"
NM="${NM:-nm}"
SIZE="${SIZE:-size}"
NASM="${NASM:-nasm}"
PYTHON="${PYTHON:-python3}"

for TOOL in "$CC" "$NM" "$SIZE" "$NASM" "$PYTHON"; do
    command -v "$TOOL" >/dev/null 2>&1 || { echo "ERROR: tool not found: $TOOL"; exit 1; }
done

for FILE in "$ROOT/PNO_ABI.h" "$ROOT/PNO_ABI.c" "$ROOT/libc_compat.c" "$ROOT/udiv64.c" "$ROOT/ksetjmp.h" "$ROOT/ksetjmp.asm" "$CORE/source/clownmdemu.h" "$FRONTEND/clown_audio_resampler.c" "$FRONTEND/clown_audio.c" "$FRONTEND/earbleeder.c"; do
    [[ -f "$FILE" ]] || { echo "ERROR: required source missing: $FILE"; exit 1; }
done

mkdir -p "$OBJDIR" "$BUILD/include"
cat > "$BUILD/include/string.h" <<'EOF_STRING'
#ifndef PNO_STRING_H
#define PNO_STRING_H
#include <stddef.h>
void *memset(void *dst, int value, size_t count);
void *memcpy(void *dst, const void *src, size_t count);
void *memmove(void *dst, const void *src, size_t count);
int memcmp(const void *a, const void *b, size_t count);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
#endif
EOF_STRING
cat > "$BUILD/include/assert.h" <<'EOF_ASSERT'
#ifndef PNO_ASSERT_H
#define PNO_ASSERT_H
#define assert(expression) ((void)0)
#endif
EOF_ASSERT
cat > "$BUILD/include/math.h" <<'EOF_MATH'
#ifndef PNO_MATH_H
#define PNO_MATH_H
double sin(double value);
double cos(double value);
#endif
EOF_MATH
cat > "$BUILD/include/HD_audio.h" <<'EOF_AUDIO'
#ifndef PNO_HD_AUDIO_H
#define PNO_HD_AUDIO_H
#define HDA_PCM_SAMPLE_RATE_48000 48000u
#endif
EOF_AUDIO

CFLAGS=(
    -m32 -std=c99 -ffreestanding -U__unix__ -fno-builtin -fno-stack-protector
    -fno-pic -fno-pie -fno-asynchronous-unwind-tables -fno-unwind-tables
    -ffunction-sections -fdata-sections -O2 -Wall -Wextra
    -Werror=implicit-function-declaration -mno-sse -mno-sse2 -mno-mmx -mfpmath=387
    -I"$BUILD/include" -I"$ROOT" -I"$ROOT/include" -I"$GAME" -I"$FRONTEND"
    -I"$CORE/source" -I"$CORE/libraries" -I"$CORE/libraries/clown68000/source"
    -I"$CORE/libraries/clownz80/source"
)

SOURCES=("$GAME/clown_platform.c" "$GAME/clown_pno.c" "$FRONTEND/clown_audio_resampler.c" "$FRONTEND/clown_audio.c" "$FRONTEND/earbleeder.c" "$ROOT/PNO_ABI.c" "$ROOT/libc_compat.c" "$ROOT/udiv64.c")
while IFS= read -r SOURCE; do SOURCES+=("$SOURCE"); done < <(find "$CORE/source" -maxdepth 1 -type f -name '*.c' | sort)
while IFS= read -r SOURCE; do SOURCES+=("$SOURCE"); done < <(find "$CORE/libraries" -type f -name '*.c' | sort)

OBJECTS=()
for SOURCE in "${SOURCES[@]}"; do
    RELATIVE="${SOURCE#"$ROOT"/}"
    OBJECT="$OBJDIR/${RELATIVE//\//_}"
    OBJECT="${OBJECT%.c}.o"
    echo "[CC] $RELATIVE"
    "$CC" "${CFLAGS[@]}" -MMD -MP -c "$SOURCE" -o "$OBJECT"
    OBJECTS+=("$OBJECT")
done

"$NASM" -f elf32 "$ROOT/ksetjmp.asm" -o "$OBJDIR/ksetjmp.o"
OBJECTS+=("$OBJDIR/ksetjmp.o")

"$CC" -m32 -nostdlib -static -no-pie \
    -Wl,-T,"$GAME/pno_clown.ld" -Wl,--emit-relocs -Wl,--gc-sections \
    -Wl,--build-id=none -Wl,--no-undefined -Wl,-Map,"$BUILD/clown.map" \
    "${OBJECTS[@]}" -lgcc -o "$BUILD/clown.elf"

"$NM" -u "$BUILD/clown.elf" > "$BUILD/unresolved.txt"
if [[ -s "$BUILD/unresolved.txt" ]]; then
    cat "$BUILD/unresolved.txt"
    echo "ERROR: unresolved ELF symbols"
    exit 1
fi

"$PYTHON" "$GAME/pno_pack.py" "$BUILD/clown.elf" "$BUILD/clown.pno" --name clown.pno
"$SIZE" "$BUILD/clown.elf"
printf '\nELF: %s\nPNO: %s\nMAP: %s\n' "$BUILD/clown.elf" "$BUILD/clown.pno" "$BUILD/clown.map"
