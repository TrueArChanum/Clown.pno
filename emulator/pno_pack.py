#!/usr/bin/env python3

# Part of Penetrizzel
# Copyright (c) 2026 ArChanum

import argparse
import struct
import sys
import zlib

HEADER_SIZE = 128
SECTION_COUNT = 3
SECTION_RECORD_SIZE = 16
RELOC_RECORD_SIZE = 12

MAGIC = b"PNO1"

R_386_32 = 1
R_386_PC32 = 2

SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4
SHT_REL = 9
SHT_NOBITS = 8

SHN_UNDEF = 0
SHN_ABS = 0xFFF1

SHF_ALLOC = 2

PNO_CAP_MEMORY = 0x00000001
PNO_CAP_LOG = 0x00000002
PNO_CAP_TIME = 0x00000004
PNO_CAP_STORAGE_READ = 0x00000008
PNO_CAP_STORAGE_WRITE = 0x00000010
PNO_CAP_VIDEO = 0x00000020
PNO_CAP_AUDIO = 0x00000040
PNO_CAP_STORAGE_SYNC = 0x00000080
PNO_CAP_VIDEO_PALETTE = 0x00000100

PNO_CAPABILITIES = (
    PNO_CAP_MEMORY |
    PNO_CAP_LOG |
    PNO_CAP_TIME |
    PNO_CAP_STORAGE_READ |
    PNO_CAP_STORAGE_WRITE |
    PNO_CAP_VIDEO |
    PNO_CAP_AUDIO |
    PNO_CAP_STORAGE_SYNC |
    PNO_CAP_VIDEO_PALETTE
)


def u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def align(value, boundary):
    return (value + boundary - 1) & ~(boundary - 1)


def cstr(data, offset):
    if offset < 0 or offset >= len(data):
        return ""

    end = data.find(b"\0", offset)

    if end < 0:
        end = len(data)

    return data[offset:end].decode("ascii", errors="replace")


def section_bytes(data, section):
    start = section["offset"]
    end = start + section["size"]

    if start < 0 or end > len(data):
        raise ValueError("section %s is outside ELF file" % section["name"])

    return data[start:end]


def parse_elf32(data):
    if len(data) < 52 or data[:4] != b"\x7fELF":
        raise ValueError("input is not an ELF file")

    if data[4] != 1 or data[5] != 1:
        raise ValueError("expected ELF32 little-endian input")

    if u16(data, 18) != 3:
        raise ValueError("expected EM_386")

    if u16(data, 16) != 2:
        raise ValueError("expected a linked ET_EXEC image")

    entry = u32(data, 24)
    shoff = u32(data, 32)
    shentsize = u16(data, 46)
    shnum = u16(data, 48)
    shstrndx = u16(data, 50)

    if shentsize != 40 or shnum == 0 or shstrndx >= shnum:
        raise ValueError("unsupported or malformed ELF section table")

    if shoff + shentsize * shnum > len(data):
        raise ValueError("ELF section table is truncated")

    sections = []

    for i in range(shnum):
        off = shoff + i * shentsize

        sections.append({
            "name_off": u32(data, off),
            "type": u32(data, off + 4),
            "flags": u32(data, off + 8),
            "addr": u32(data, off + 12),
            "offset": u32(data, off + 16),
            "size": u32(data, off + 20),
            "link": u32(data, off + 24),
            "info": u32(data, off + 28),
            "align": u32(data, off + 32),
            "entsize": u32(data, off + 36),
        })

    shstr = sections[shstrndx]

    if shstr["type"] != SHT_STRTAB:
        raise ValueError("ELF section-name table is not a string table")

    names = section_bytes(data, shstr)

    for section in sections:
        section["name"] = cstr(names, section["name_off"])

        if section["type"] != SHT_NOBITS:
            section_bytes(data, section)

    by_name = {}

    for i, section in enumerate(sections):
        if section["name"] in by_name:
            raise ValueError("duplicate ELF section name: %s" % section["name"])

        by_name[section["name"]] = (i, section)

    for required in (".text", ".rodata", ".data"):
        if required not in by_name:
            raise ValueError("linked ELF is missing %s" % required)

    text_index, text = by_name[".text"]
    ro_index, rodata = by_name[".rodata"]
    data_index, data_section = by_name[".data"]

    if text["type"] != SHT_PROGBITS or text["size"] == 0:
        raise ValueError(".text must be non-empty PROGBITS")

    if rodata["type"] != SHT_PROGBITS or rodata["size"] == 0:
        raise ValueError(".rodata must be non-empty PROGBITS")

    if data_section["type"] != SHT_PROGBITS or data_section["size"] == 0:
        raise ValueError(".data must be non-empty PROGBITS")

    for section in (text, rodata, data_section):
        if section["addr"] & 15:
            raise ValueError("section %s is not 16-byte aligned" % section["name"])

    bss_index = None
    bss = None

    if ".bss" in by_name:
        bss_index, bss = by_name[".bss"]

        if bss["type"] != SHT_NOBITS:
            raise ValueError(".bss must be SHT_NOBITS")

    image_section_names = {".text", ".rodata", ".data", ".bss"}

    for section in sections:
        if (
            section["flags"] & SHF_ALLOC and
            section["size"] and
            section["name"] not in image_section_names
        ):
            raise ValueError("unsupported allocated ELF section: %s" % section["name"])

    image_sections = {text_index, ro_index, data_index}

    if bss_index is not None:
        image_sections.add(bss_index)

    if not (text["addr"] <= entry < text["addr"] + text["size"]):
        raise ValueError("ELF entry point is not inside .text")

    image_parts = [text, rodata, data_section]

    if bss is not None and bss["size"] != 0:
        image_parts.append(bss)

    image_parts.sort(key=lambda section: section["addr"])

    previous_end = 0

    for section in image_parts:
        start = section["addr"]
        end = start + section["size"]

        if end > 0xFFFFFFFF:
            raise ValueError("section address range overflows uint32")

        if start < previous_end:
            raise ValueError("overlapping PNO image sections")

        previous_end = end

    image_end = max(section["addr"] + section["size"] for section in image_parts)

    if image_end >= 0x80000000:
        raise ValueError("PNO image exceeds supported signed relocation range")

    bss_rva = 0
    bss_size = 0

    if bss is not None and bss["size"] != 0:
        bss_rva = bss["addr"]
        bss_size = bss["size"]

        if bss_rva + bss_size > image_end:
            raise ValueError("BSS extends beyond image")

    symtabs = {}

    for i, section in enumerate(sections):
        if section["type"] != SHT_SYMTAB:
            continue

        if section["entsize"] != 16 or section["size"] % 16 != 0:
            raise ValueError("unsupported ELF symbol table")

        if section["link"] >= len(sections):
            raise ValueError("symbol table has invalid string-table link")

        strings_section = sections[section["link"]]

        if strings_section["type"] != SHT_STRTAB:
            raise ValueError("symbol table link is not a string table")

        strings = section_bytes(data, strings_section)
        symbols = []

        for n in range(section["size"] // 16):
            off = section["offset"] + n * 16
            name_off = u32(data, off)

            symbols.append({
                "name": cstr(strings, name_off),
                "value": u32(data, off + 4),
                "size": u32(data, off + 8),
                "info": data[off + 12],
                "shndx": u16(data, off + 14),
            })

        symtabs[i] = symbols

    file_image_sections = {
        text_index: text,
        ro_index: rodata,
        data_index: data_section,
    }

    relocations = []

    for relsec in sections:
        if relsec["type"] == SHT_RELA:
            raise ValueError("ELF32 RELA relocations are not supported")

        if relsec["type"] != SHT_REL or relsec["size"] == 0:
            continue

        if relsec["entsize"] != 8 or relsec["size"] % 8 != 0:
            raise ValueError("malformed ELF REL section: %s" % relsec["name"])

        if relsec["link"] not in symtabs:
            raise ValueError("REL section has invalid symbol-table link")

        target_index = relsec["info"]

        if target_index >= len(sections):
            raise ValueError("REL section has invalid target section")

        target = sections[target_index]

        if target_index not in file_image_sections:
            if target["flags"] & SHF_ALLOC:
                raise ValueError(
                    "relocations targeting %s are unsupported" % target["name"]
                )
            continue

        symbols = symtabs[relsec["link"]]
        target_file_section = file_image_sections[target_index]

        for n in range(relsec["size"] // 8):
            off = relsec["offset"] + n * 8

            r_offset = u32(data, off)
            r_info = u32(data, off + 4)

            sym_index = r_info >> 8
            reloc_type = r_info & 0xFF

            if reloc_type not in (R_386_32, R_386_PC32):
                raise ValueError(
                    "unsupported i386 relocation type %u in %s" %
                    (reloc_type, relsec["name"])
                )

            if sym_index >= len(symbols):
                raise ValueError("relocation references invalid ELF symbol")

            symbol = symbols[sym_index]
            shndx = symbol["shndx"]

            if shndx == SHN_UNDEF:
                raise ValueError(
                    "unresolved external symbol: %s" %
                    (symbol["name"] or "<anonymous>")
                )

            if shndx != SHN_ABS and shndx not in image_sections:
                raise ValueError(
                    "relocation symbol is outside PNO image: %s" %
                    (symbol["name"] or "<anonymous>")
                )

            if reloc_type == R_386_PC32:
                continue

            if (
                r_offset < target_file_section["addr"] or
                r_offset + 4 > target_file_section["addr"] + target_file_section["size"]
            ):
                raise ValueError("relocation target is outside its output section")

            field_offset = (
                target_file_section["offset"] +
                (r_offset - target_file_section["addr"])
            )

            linked_value = u32(data, field_offset)

            if shndx == SHN_ABS and symbol["value"] >= image_end:
                continue

            if linked_value >= image_end:
                if shndx == SHN_ABS:
                    continue

                raise ValueError(
                    "ABS32 relocation value is outside PNO image: %s" %
                    (symbol["name"] or "<anonymous>")
                )

            relocations.append((
                r_offset,
                linked_value,
                0xFFFF,
                R_386_32,
            ))

    relocations.sort(key=lambda item: item[0])

    payloads = []
    records = []
    file_cursor = HEADER_SIZE

    for section in (text, rodata, data_section):
        file_cursor = align(file_cursor, 16)

        raw = section_bytes(data, section)

        records.append((
            file_cursor,
            section["addr"],
            len(raw),
            len(raw),
        ))

        payloads.append((file_cursor, raw))
        file_cursor += len(raw)

    reloc_offset = 0

    if relocations:
        reloc_offset = align(file_cursor, 4)
        file_cursor = reloc_offset + len(relocations) * RELOC_RECORD_SIZE

    if file_cursor > 0xFFFFFFFF:
        raise ValueError("PNO file exceeds uint32 size")

    output = bytearray(file_cursor)

    output[0:4] = MAGIC

    struct.pack_into(
        "<6H",
        output,
        4,
        HEADER_SIZE,
        1,
        0,
        1,
        0,
        1,
    )

    struct.pack_into(
        "<12I",
        output,
        16,
        0,
        len(output),
        image_end,
        entry,
        bss_rva,
        bss_size,
        reloc_offset,
        len(relocations),
        0,
        0,
        0,
        0,
    )

    struct.pack_into(
        "<2I",
        output,
        64,
        PNO_CAPABILITIES,
        0,
    )

    struct.pack_into("<I", output, 72, 0)

    for i, record in enumerate(records):
        struct.pack_into(
            "<4I",
            output,
            76 + i * SECTION_RECORD_SIZE,
            *record,
        )

    struct.pack_into("<I", output, 124, 0)

    for off, raw in payloads:
        output[off:off + len(raw)] = raw

    for i, relocation in enumerate(relocations):
        struct.pack_into(
            "<IiHH",
            output,
            reloc_offset + i * RELOC_RECORD_SIZE,
            *relocation,
        )

    crc = zlib.crc32(output) & 0xFFFFFFFF
    struct.pack_into("<I", output, 72, crc)

    return bytes(output), {
        "entry_rva": entry,
        "image_size": image_end,
        "bss_rva": bss_rva,
        "bss_size": bss_size,
        "relocations": len(relocations),
        "crc32": crc,
        "section_records": records,
    }


def make_fas_commands(pno_path, device_id, object_id, object_name):
    with open(pno_path, "rb") as f:
        data = f.read()

    commands = []
    offset = 0

    while offset < len(data):
        prefix = "FAS WRITE %s %s %u " % (device_id, object_id, offset)
        max_bytes = min(128, (32 - len(prefix)) // 2)

        if max_bytes < 1:
            raise ValueError("FAS WRITE command has no room for data")

        chunk = data[offset:offset + max_bytes]
        command = prefix + chunk.hex().upper()

        if len(command) > 32:
            raise ValueError("FAS WRITE command exceeds console line limit")

        commands.append(command)
        offset += len(chunk)

    verify = "FAS VERIFY %s %s" % (device_id, object_name)

    if len(verify) > 32:
        raise ValueError("FAS VERIFY command exceeds console line limit")

    commands.append(verify)

    return "\n".join(commands) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Package ELF32/i386 into PNO v1")

    parser.add_argument("elf")
    parser.add_argument("output")
    parser.add_argument("--name", default="clown.pno")
    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--object-id", type=int)
    parser.add_argument("--commands")

    args = parser.parse_args()

    if len(args.name.encode("ascii", errors="strict")) > 31:
        parser.error("FAS object name must be at most 31 ASCII bytes")

    if args.device < 0 or args.device > 0xFFFFFFFF:
        parser.error("device ID must fit uint32")

    if args.object_id is not None and (
        args.object_id < 0 or args.object_id > 0xFFFFFFFE
    ):
        parser.error("object ID must fit uint32 and cannot be invalid sentinel")

    try:
        with open(args.elf, "rb") as f:
            elf = f.read()

        pno, info = parse_elf32(elf)

        with open(args.output, "wb") as f:
            f.write(pno)

        if args.commands:
            if args.object_id is None:
                raise ValueError("--object-id is required with --commands")

            commands = make_fas_commands(
                args.output,
                args.device,
                args.object_id,
                args.name,
            )

            with open(args.commands, "w", encoding="ascii") as f:
                f.write(commands)

        print("PNO: %s (%u bytes)" % (args.output, len(pno)))
        print(
            "Entry RVA: 0x%08X; image: %u bytes" %
            (info["entry_rva"], info["image_size"])
        )
        print(
            "BSS: RVA 0x%08X; size: %u bytes" %
            (info["bss_rva"], info["bss_size"])
        )
        print("ABS32 relocations: %u" % info["relocations"])
        print("CRC32: 0x%08X" % info["crc32"])

        if args.commands:
            print("FAS commands: %s" % args.commands)

    except (OSError, ValueError, struct.error) as exc:
        print("pno_pack: %s" % exc, file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
