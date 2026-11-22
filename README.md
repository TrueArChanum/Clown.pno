Writing✓

# 🤡 Clown.pno

**ClownMDEmu packaged as a Penetrizzel Native Object (.pno).**

A Sega Genesis / Mega Drive emulator running as a native application inside Penetrizzel.

Special thanks to **Clownacy** for the marvelous ClownMDEmu\!

---

## 🎮 Demo

▶ **Watch TOFU\_Glitch.webm**

---

## 💾 About Penetrizzel

Clown.pno is part of **Penetrizzel**, a self-contained retro-style gaming platform inspired by the MS-DOS floppy disk era.

**No Godot. No Unity. No Unreal. No SDL. Just freestanding C and Assembly.**

Penetrizzel is designed to boot directly from a USB stick, without requiring an HDD or SSD.

### Platform highlights

- **CPU:** 32-bit x86 protected mode
- **Languages:** Freestanding C99 + NASM Assembly
- **Boot:** Legacy BIOS and UEFI
- **Graphics:** VGA Mode X, 320×240, 8-bit framebuffer
- **Sound:** PC Beeper and HDA Audio
- **USB:** xHCI, USB Mouse, Keyboard and Mass Storage
- **Storage:** Flat-Address Storage (FAS)
- **Applications:** PNO (Penetrizzel Native Object)

### Clown.pno features

- Native Penetrizzel application.
- ClownMDEmu core integration.
- Sega Genesis / Mega Drive ROM support.
- ROM support up to 6 MiB.
- Custom platform, audio, video and input interfaces.

---

## 🛠️ Building

The project includes a build script:

bash

```bash
./build_clown.sh
```

See the script for the required toolchain and build configuration.

---

## ⚠️ Limitations

Penetrizzel is a work in progress.

- USB support is still experimental and can be unreliable.
- HDA audio support is limited to selected hardware.
- PNO is still evolving; application loading and execution have some hardcoded limitations.
- No multitasking or multithreading.
- Emulator performance depends on the available CPU and software-rendering implementation.

---

## 📜 Credits & Licensing

Clown.pno incorporates **ClownMDEmu** by Clownacy.

Please refer to the included license files:

- `LICENCE.txt`
- `licence.txt`
- `clownmdemu-core/LICENCE.txt`
- `clownmdemu-core/THIRD_PARTY_LICENSES.txt`

All original components remain subject to their respective licenses.

---

**Best regards,**

**⛧ ArChanum**
