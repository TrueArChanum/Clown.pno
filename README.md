# 🤡 Clown.pno

**ClownMDEmu packaged as a Penetrizzel Native Object (.pno).**

A Sega Genesis / Mega Drive emulator running as a native application inside Penetrizzel.

Special thanks to **Clownacy** for the marvelous ClownMDEmu\!

---

<details>
<summary> **Update:** Clownacy has fixed this issue in a newer version. The workaround described below is only relevant to older versions. </summary>

## 🎮 Old Demo //NOTE: this is an older video, before the existence of PNO programs.

▶ [Watch TOFU_Glitch.webm](./TOFU_Glitch.webm)

https://github.com/user-attachments/assets/0c475439-f91d-44b9-871f-6d51b6e45994

</details>

## 🎮 New Demo with a "fresh" TOFU:

https://github.com/user-attachments/assets/9150336c-2fdf-4247-87b4-70d96c295ca7

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

//TODO:
Build instructions:

## run
/emulator ⛧ ./build_clown.sh 


## This will create this folder + stuff:
/emulator/build ⛧ ls
clown.elf  clown.map  clown.pno  include  obj  unresolved.txt

## "clown.pno"
is the important file (the emulator as PNO).


## U need to copy this file into the PNO folder of Penetrizzel:
/Penetrizzel/Kernel/PNO ⛧ ls
chess.pno  clown.pno  wurm.pno
//TODO not yet available...

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

Clown.pno incorporates ClownMDEmu by Clownacy.

### License files

- [LICENCE.txt](./LICENCE.txt)
- [licence.txt](./licence.txt)
- [ClownMDEmu License](./clownmdemu-core/LICENCE.txt)
- [licence.txt](./clownmdemu-core/licence.txt)
- [Third Party Licenses](./clownmdemu-core/THIRD_PARTY_LICENSES.txt)

All original components remain subject to their respective licenses.

---

**Best regards,**

**⛧ ArChanum**
